"""Generate an isolated extension of Tracy's reader/writer; never edit vcpkg sources."""
from pathlib import Path
import re
import sys

source, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
server = source / "server"
version = (source / "public/common/TracyVersion.hpp").read_text(encoding="utf-8")
for field, expected in (("Major", 0), ("Minor", 14), ("Patch", 1)):
    if not re.search(rf"constexpr int {field}\s*=\s*{expected}\s*;", version):
        raise SystemExit("This extension requires Tracy 0.14.1 sources")

def write_changed(path, text):
    if not path.exists() or path.read_text(encoding="utf-8") != text:
        path.write_text(text, encoding="utf-8")

def includes(text, directory):
    def replace(match):
        name = match[1]
        path = output / name if name == "TracyWorker.hpp" else directory / name
        return f'#include "{path.resolve().as_posix()}"'
    return re.sub(r'#include "([^"]+)"', replace, text)

header = (server / "TracyWorker.hpp").read_text(encoding="utf-8")
needle = "    void Write( FileWrite& f, bool fiDict );"
if header.count(needle) != 1:
    raise SystemExit("Unsupported Tracy source: expected 0.14.1 Worker::Write declaration")
header = header.replace(needle, "    void Crop(int64_t begin, int64_t end);\n" + needle)
write_changed(output / "TracyWorker.hpp", includes(header, server))
cpp = includes((server / "TracyWorker.cpp").read_text(encoding="utf-8"), server)
cpp = cpp.replace('#include <capstone/capstone.h>', '')
start = cpp.index('void Worker::AddSymbolCode(')
end = cpp.index('void Worker::AddSourceCode(', start)
cpp = cpp[:start] + '''void Worker::AddSymbolCode(uint64_t, const char*, size_t) {
    throw std::runtime_error("Live symbol-code ingestion is disabled in the offline crop tool");
}
\n''' + cpp[end:]
cpp += f'\n#include "{Path(__file__).with_name("crop_impl.hpp").resolve().as_posix()}"\n'
write_changed(output / "TracyWorker.cpp", cpp)
