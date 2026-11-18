#ifndef MARKETPLACE_WEB_SERVER_HPP
#define MARKETPLACE_WEB_SERVER_HPP

#include <atomic>
#include <cstdint>
#include <string>

#include "marketplace/services.hpp"

namespace marketplace {

struct request_message {
	std::string method;
	std::string path;
	std::string body;
};

struct response_message {
	std::uint16_t status	   = 200;
	std::string   content_type = "application/json; charset=utf-8";
	std::string   body;
};

class web_server {
      public:
	web_server(
	    std::string		  web_root,
	    marketplace_store &	  store,
	    catalog_service &	  catalog,
	    content_service &	  content,
	    user_service &	  users,
	    purchase_controller & purchases,
	    exchange_service &	  exchanges,
	    moderation_service &  moderation);

	int
	run(std::uint16_t port, bool open_browser_window);

	void
	stop();

	bool
	is_running() const;

      private:
	response_message
	route_(request_message const & request);

	response_message
	route_api_(request_message const & request);

	response_message
	serve_file_(std::string const & resource_address) const;

	std::string
	json_product_(std::shared_ptr<product> const & item) const;

	std::string
	json_deal_(std::shared_ptr<deal> const & item) const;

	std::string
	json_complaint_(std::shared_ptr<complaint> const & item) const;

	std::string
	json_user_(std::shared_ptr<user> const & item) const;

	std::string	      web_root_;
	marketplace_store &   store_;
	catalog_service &     catalog_;
	content_service &     content_;
	user_service &	      users_;
	purchase_controller & purchases_;
	exchange_service &    exchanges_;
	moderation_service &  moderation_;
	uuid		      active_user_id_;
	std::atomic<bool>     stopping_{ false };
	std::atomic<int>      server_socket_{ -1 };
};

} // namespace marketplace

#endif // MARKETPLACE_WEB_SERVER_HPP
