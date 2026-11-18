#include "marketplace/market_interface.hpp"

#include <utility>

#include "marketplace/strategies.hpp"

namespace marketplace {

catalog_form::catalog_form(catalog_service & catalog, uuid user_id) : catalog_(catalog), user_id_(user_id) {}

void
catalog_form::show() const
{ static_cast<void>(catalog_); }

std::vector<std::shared_ptr<product> >
catalog_form::search_products(std::string const & query) const
{ return catalog_.search(user_id_, query); }

std::shared_ptr<product>
catalog_form::open_product(uuid const & product_id) const
{ return catalog_.get_product(product_id); }

publication_form::publication_form(catalog_service & catalog, content_service & content, uuid seller_id)
    : catalog_(catalog), seller_id_(seller_id), content_(content)
{}

void
publication_form::show() const
{ static_cast<void>(catalog_); }

uuid
publication_form::publish_product(std::map<std::string, std::string> const & data)
{
	auto const value = [&data](std::string const & key) {
		auto const it = data.find(key);
		return it == data.end() ? std::string{} : it->second;
	};

	double price = 0.0;
	try {
		price = std::stod(value("price"));
	} catch (std::exception const &) {
		return {};
	}

	auto const item = std::make_shared<product>();
	item->set_id(uuid::generate());
	item->set_name(value("name"));
	item->set_description(value("description"));
	item->set_category(value("category"));
	item->set_usage_terms(value("terms"));
	item->change_rules(std::make_shared<fixed_price>(price));

	uuid const id = catalog_.publish(seller_id_, item);
	if (!id.empty()) content_.upload(id, value("content_type"), value("address"));
	return id;
}

void
publication_form::upload_content(uuid const & product_id, std::string const & type, std::string const & address)
{ content_.upload(product_id, type, address); }

purchase_form::purchase_form(purchase_controller & controller, uuid buyer_id)
    : controller_(controller), buyer_id_(buyer_id)
{}

void
purchase_form::show() const
{ static_cast<void>(controller_); }

std::string
purchase_form::buy_product(uuid const & product_id)
{ return controller_.make_purchase(buyer_id_, product_id); }

void
purchase_form::confirm_purchase(uuid const & product_id)
{ static_cast<void>(product_id); }

std::string
purchase_form::get_product(uuid const & product_id) const
{ return controller_.get_purchased_product(buyer_id_, product_id); }

exchange_form::exchange_form(exchange_service & exchange, uuid user_id) : exchange_(exchange), user_id_(user_id) {}

void
exchange_form::show() const
{ static_cast<void>(exchange_); }

uuid
exchange_form::propose_exchange(uuid const & owned_product_id, uuid const & desired_product_id)
{ return exchange_.propose(user_id_, desired_product_id, owned_product_id); }

void
exchange_form::respond_to_exchange(uuid const & deal_id, bool accepted)
{
	if (accepted)
		exchange_.accept(deal_id, user_id_);
	else
		exchange_.reject(deal_id);
}

} // namespace marketplace
