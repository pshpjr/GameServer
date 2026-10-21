#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include "TracyWorker.hpp"
#include "TracyFileRead.hpp"
#include "TracyFileWrite.hpp"

namespace fs = std::filesystem;

int64_t Time(const std::string& text) {
    // Seconds, mm:ss, or hh:mm:ss. Decimals are allowed only in seconds.
    long double value = 0;
    size_t offset = 0;
    unsigned fields = 0;
    for (;;) {
        auto colon = text.find(':', offset);
        auto part = text.substr(offset, colon == std::string::npos ? colon : colon - offset);
        if (part.empty() || part.find_first_not_of("0123456789.") != std::string::npos)
            throw std::runtime_error("Invalid time: " + text);
        size_t consumed;
        long double n = std::stold(part, &consumed);
        if (consumed != part.size() || !std::isfinite(n) || n < 0 ||
            (fields && n >= 60) || (colon != std::string::npos && part.find('.') != std::string::npos))
            throw std::runtime_error("Invalid time: " + text);
        value = value * 60 + n;
        if (++fields > 3 || value >= (1ULL << 46) / 1e9L)
            throw std::runtime_error("Time exceeds the supported trace range");
        if (colon == std::string::npos) break;
        offset = colon + 1;
    }
    return int64_t(std::round(value * 1e9L));
}

std::unique_ptr<tracy::FileRead> Reader(const fs::path& path) {
    auto f = std::unique_ptr<tracy::FileRead>(tracy::FileRead::Open(path.string().c_str()));
    if (!f) throw std::runtime_error("Cannot open input: " + path.string());
    return f;
}

int main(int argc, char** argv) {
    fs::path temporary;
    bool created = false;
    try {
        if (argc != 5) {
            std::cout << "Usage: tracy-crop INPUT.tracy OUTPUT.tracy START END\n"
                "Time: seconds, mm:ss, or hh:mm:ss, relative to original trace zero.\n"
                "Boundary-overlapping events are kept whole; output may include earlier/later times.\n"
                "Frame intervals, plot neighbours, allocation lifetimes and lock-state history may extend it.\n"
                "Original timestamps are preserved. Existing files are never overwritten.\n"
                "CPU traces only: GPU zone captures are rejected explicitly.\n";
            return argc == 2 && std::string(argv[1]) == "--help" ? 0 : 2;
        }
        const auto begin = Time(argv[3]), end = Time(argv[4]);
        if (end <= begin) throw std::runtime_error("END must be greater than START");
        const fs::path input = fs::absolute(argv[1]), output = fs::absolute(argv[2]);
        if (fs::exists(output)) throw std::runtime_error("Output already exists; choose a new filename");
        if (!fs::is_regular_file(input)) throw std::runtime_error("Input file does not exist");
        if (!fs::is_directory(output.parent_path())) throw std::runtime_error("Output directory does not exist");
        temporary = output;
        temporary += ".partial";
        if (fs::exists(temporary)) throw std::runtime_error("Partial output already exists; choose a new filename");
        std::cerr << "주의: 지정 시간과 겹치는 이벤트는 전체를 보존합니다. 요청한 시간만 정확히 잘리는 것은 보장하지 않으며, 이전·이후 시간이 포함될 수 있습니다.\n"
            "프레임 경계, 그래프의 인접 표본, 메모리 할당 수명, 락 상태 이력도 범위를 넓힐 수 있습니다.\n"
            "Requested: " << begin / 1e9 << " .. " << end / 1e9 << " seconds. Loading...\n";
        {
            auto f = Reader(input);
            tracy::Worker worker(*f, tracy::EventType::All, false);
            worker.Crop(begin, end);
            std::cerr << "Writing subset...\n";
            auto w = std::unique_ptr<tracy::FileWrite>(tracy::FileWrite::Open(temporary.string().c_str(), tracy::FileCompression::Zstd, 3, 4));
            if (!w) throw std::runtime_error("Cannot create output");
            created = true;
            worker.Write(*w, false);
            w->Finish();
        }
        if (fs::exists(output)) throw std::runtime_error("Output appeared while processing; refusing to overwrite");
        fs::rename(temporary, output);
        temporary.clear();
        std::cout << "Saved: " << output.string() << '\n';
        return 0;
    } catch (const tracy::UnsupportedVersion&) { std::cerr << "Unsupported newer Tracy version\n";
    } catch (const tracy::LegacyVersion&) { std::cerr << "Unsupported legacy Tracy version\n";
    } catch (const tracy::NotTracyDump&) { std::cerr << "Input is not a Tracy capture\n";
    } catch (const tracy::FileReadError&) { std::cerr << "Unable to read Tracy capture\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
    if (created && !temporary.empty()) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
    }
    return 1;
}
