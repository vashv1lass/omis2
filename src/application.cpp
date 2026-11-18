#include "marketplace/application.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

namespace marketplace {

application::application(std::string web_root) : web_root_(std::move(web_root)) {}

int
application::run(int argc, char ** argv)
{
	std::uint32_t port = 8008;

	for (int i = 1; i < argc; i++) {
		std::string const arg(argv[i]);
		if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
			try {
				port = static_cast<std::uint32_t>(std::stoul(argv[++i]));
			} catch (std::exception const &) {
				std::cerr << "The port must be a number between 1 and 65535.\n";
				return 1;
			}
		} else if (arg == "--help" || arg == "-h") {
			std::cout << "Usage: digital_goods_marketplace [--port PORT] \n";
			return 0;
		} else {
			std::cerr << "Unknown argument: " << arg << '\n'
				  << "Usage: digital_goods_marketplace [--port PORT] \n";
			return 1;
		}
	}

	if (port == 0 || port > std::numeric_limits<std::uint16_t>::max()) {
		std::cerr << "The port must be a number between 1 and 65535.\n";
		return 1;
	}

	factory_.build();
	server_ = std::make_unique<web_server>(
	    web_root_,
	    factory_.store(),
	    factory_.catalog(),
	    factory_.content(),
	    factory_.users(),
	    factory_.purchases(),
	    factory_.exchanges(),
	    factory_.moderation());

	return server_->run(static_cast<std::uint16_t>(port), true);
}

} // namespace marketplace
