#include <cstdlib>
#include <iostream>

#include <boost/log/trivial.hpp>

#include <streaming_lib.hpp>

int main(int argc, char *argv[]) {
    try {
        std::string path{"./"};
        uint16_t port_number{554};

        if ((argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0)) || argc > 3) {
            BOOST_LOG_TRIVIAL(fatal) << "Usage: " << argv[0]
                                     << " [<port_number>] | [<portnumber> <ressource_path>]\n";
            return EXIT_FAILURE;
        }

        if (argc > 1) {
            port_number = std::stoul(argv[1]);
        }
        if (argc > 2) {
            path = argv[2];
        }

        rtsp::rtsp_server_pimpl server{path, port_number, [](auto &e) { BOOST_LOG_TRIVIAL(error) << e.what(); }};

        BOOST_LOG_TRIVIAL(info) << "Using \"" << path << "\" as ressource path";
        BOOST_LOG_TRIVIAL(info) << "rtsp_server listing on port: " << port_number;

        for (std::string input; std::getline(std::cin, input);) {
            if (input == "quit") {
                std::cout << "Quitting" << std::endl;
                server.graceful_shutdown();
                break;
            }
            std::cout << "Enter quit to exit" << std::endl;
        };
    } catch (std::exception &e) {
        BOOST_LOG_TRIVIAL(fatal) << "Exception in main: " << e.what();
        throw;
    }
}
