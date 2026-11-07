#include "Metrics.h"
#include "Server.h"

#include <charconv>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

int main(int argc, char* argv[])
{
    try
    {
        std::uint16_t port = 6000;
        if (argc > 1)
        {
            const std::string_view text = argv[1];
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), port);
            if (error != std::errc{} || end != text.data() + text.size() || port == 0)
            {
                throw std::invalid_argument("Port must be an integer from 1 to 65535.");
            }
        }
        if (argc > 3 || (argc == 3 && std::string_view(argv[2]) != "--trace"))
        {
            throw std::invalid_argument("Usage: NetLibTest_Echo [port] [--trace]");
        }

        const auto server = std::make_unique<psh::Server>(port, argc == 3);

        auto metricsConfig = psh::metrics::ConfigFromEnv("echo");
        metricsConfig.useConsoleMonitor = true;
        if (auto result = psh::metrics::Init(metricsConfig); !result)
            std::cerr << "metrics disabled: " << result.Error() << std::endl;

        server->Start();

        std::cout << "Echo server listening on port " << port << ". Press Ctrl+C to exit.\n";
        server->Wait();
        psh::metrics::Shutdown();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
