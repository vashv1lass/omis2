#include "marketplace/application.hpp"

int
main(int argc, char ** argv)
{
	marketplace::application application(MARKETPLACE_WEB_ROOT);
	return application.run(argc, argv);
}
