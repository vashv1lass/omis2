#ifndef MARKETPLACE_PLATFORM_FACTORY_HPP
#define MARKETPLACE_PLATFORM_FACTORY_HPP

#include <memory>

#include "marketplace/account_moderation.hpp"
#include "marketplace/market_interface.hpp"

namespace marketplace {

class platform_factory {
public:
	platform_factory();
	~platform_factory();

	void
	build();

	std::unique_ptr<product_selection>
	create_filter() const;

	std::unique_ptr<delivery_strategy>
	create_delivery() const;

	marketplace_store &
	store();

	catalog_service &
	catalog();

	content_service &
	content();

	user_service &
	users();

	purchase_controller &
	purchases();

	exchange_service &
	exchanges();

	moderation_service &
	moderation();

protected:
	std::unique_ptr<catalog_form>
	create_catalog_form(uuid user_id) const;

	std::unique_ptr<publication_form>
	create_publication_form(uuid seller_id) const;

	std::unique_ptr<purchase_form>
	create_purchase_form(uuid buyer_id) const;

	std::unique_ptr<exchange_form>
	create_exchange_form(uuid user_id) const;

	std::unique_ptr<account_form>
	create_account_form() const;

	std::unique_ptr<feedback_form>
	create_feedback_form(uuid user_id) const;

	std::unique_ptr<administration_form>
	create_administration_form(uuid admin_id) const;

private:
	std::unique_ptr<marketplace_store>   store_;
	std::unique_ptr<product_selection>   selection_;
	std::unique_ptr<delivery_strategy>   delivery_;
	std::unique_ptr<catalog_service>     catalog_;
	std::unique_ptr<content_service>     content_;
	std::unique_ptr<user_service>	     users_;
	std::unique_ptr<purchase_controller> purchases_;
	std::unique_ptr<exchange_service>    exchanges_;
	std::unique_ptr<moderation_service>  moderation_;
	std::unique_ptr<catalog_form>	     catalog_form_;
	std::unique_ptr<publication_form>    publication_form_;
	std::unique_ptr<purchase_form>	     purchase_form_;
	std::unique_ptr<exchange_form>	     exchange_form_;
	std::unique_ptr<account_form>	     account_form_;
	std::unique_ptr<feedback_form>	     feedback_form_;
	std::unique_ptr<administration_form> administration_form_;
	bool				     built_ = false;
};

} // namespace marketplace

#endif // MARKETPLACE_PLATFORM_FACTORY_HPP
