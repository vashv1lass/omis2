#ifndef MARKETPLACE_MARKET_INTERFACE_HPP
#define MARKETPLACE_MARKET_INTERFACE_HPP

#include "marketplace/services.hpp"

namespace marketplace {

class catalog_form {
public:
	catalog_form(catalog_service & catalog, uuid user_id);

	void
	show() const;

	std::vector<std::shared_ptr<product> >
	search_products(std::string const & query) const;

	std::shared_ptr<product>
	open_product(uuid const & product_id) const;

protected:
	catalog_service & catalog_;
	uuid		  user_id_;
};

class publication_form {
public:
	publication_form(catalog_service & catalog, content_service & content, uuid seller_id);

	void
	show() const;

	uuid
	publish_product(std::map<std::string, std::string> const & data);

	void
	upload_content(uuid const & product_id, std::string const & type, std::string const & address);

protected:
	catalog_service & catalog_;
	uuid		  seller_id_;

private:
	content_service & content_;
};

class purchase_form {
public:
	purchase_form(purchase_controller & controller, uuid buyer_id);

	void
	show() const;

	std::string
	buy_product(uuid const & product_id);

	void
	confirm_purchase(uuid const & product_id);

	std::string
	get_product(uuid const & product_id) const;

private:
	purchase_controller & controller_;
	uuid		      buyer_id_;
};

class exchange_form {
public:
	exchange_form(exchange_service & exchange, uuid user_id);

	void
	show() const;

	uuid
	propose_exchange(uuid const & owned_product_id, uuid const & desired_product_id);

	void
	respond_to_exchange(uuid const & deal_id, bool accepted);

private:
	exchange_service & exchange_;
	uuid		   user_id_;
};

} // namespace marketplace

#endif // MARKETPLACE_MARKET_INTERFACE_HPP
