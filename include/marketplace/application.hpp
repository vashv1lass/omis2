#ifndef MARKETPLACE_APPLICATION_HPP
#define MARKETPLACE_APPLICATION_HPP

#include <memory>
#include <string>

#include "marketplace/platform_factory.hpp"
#include "marketplace/web_server.hpp"

namespace marketplace {

class application {
public:
	explicit application(std::string web_root);

	int
	run(int argc, char ** argv);

private:
	std::string		    web_root_;
	platform_factory	    factory_;
	std::unique_ptr<web_server> server_;
};

} // namespace marketplace

#endif // MARKETPLACE_APPLICATION_HPP
