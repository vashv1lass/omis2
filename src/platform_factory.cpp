#include "marketplace/platform_factory.hpp"

#include <stdexcept>

namespace marketplace {

namespace {

uuid
demo_user_id(std::string const & tail)
{
	std::string const suffix = std::string(12 - tail.size(), '0') + tail;
	return uuid::parse("00000000-0000-4000-8000-" + suffix);
}

} // namespace

platform_factory::platform_factory() = default;

platform_factory::~platform_factory() = default;

void
platform_factory::build()
{
	if (built_) return;

	store_	    = std::make_unique<marketplace_store>();
	selection_  = create_filter();
	delivery_   = create_delivery();
	catalog_    = std::make_unique<catalog_service>(*store_, *selection_);
	content_    = std::make_unique<content_service>(*store_, *delivery_);
	users_	    = std::make_unique<user_service>(*store_);
	purchases_  = std::make_unique<purchase_controller>(*store_, *catalog_, *content_, *users_);
	exchanges_  = std::make_unique<exchange_service>(*store_, *content_);
	moderation_ = std::make_unique<moderation_service>(*store_, *catalog_, *users_);

	catalog_form_	     = create_catalog_form(demo_user_id("01"));
	publication_form_    = create_publication_form(demo_user_id("01"));
	purchase_form_	     = create_purchase_form(demo_user_id("01"));
	exchange_form_	     = create_exchange_form(demo_user_id("01"));
	account_form_	     = create_account_form();
	feedback_form_	     = create_feedback_form(demo_user_id("01"));
	administration_form_ = create_administration_form(demo_user_id("06"));
	built_		     = true;
}

std::unique_ptr<product_selection>
platform_factory::create_filter() const
{ return std::make_unique<filter_selection>(); }

std::unique_ptr<delivery_strategy>
platform_factory::create_delivery() const
{ return std::make_unique<direct_delivery>(); }

std::unique_ptr<catalog_form>
platform_factory::create_catalog_form(uuid user_id) const
{
	if (!catalog_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<catalog_form>(*catalog_, user_id);
}

std::unique_ptr<publication_form>
platform_factory::create_publication_form(uuid seller_id) const
{
	if (!catalog_ || !content_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<publication_form>(*catalog_, *content_, seller_id);
}

std::unique_ptr<purchase_form>
platform_factory::create_purchase_form(uuid buyer_id) const
{
	if (!purchases_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<purchase_form>(*purchases_, buyer_id);
}

std::unique_ptr<exchange_form>
platform_factory::create_exchange_form(uuid user_id) const
{
	if (!exchanges_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<exchange_form>(*exchanges_, user_id);
}

std::unique_ptr<account_form>
platform_factory::create_account_form() const
{
	if (!users_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<account_form>(*users_);
}

std::unique_ptr<feedback_form>
platform_factory::create_feedback_form(uuid user_id) const
{
	if (!store_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<feedback_form>(*store_, user_id);
}

std::unique_ptr<administration_form>
platform_factory::create_administration_form(uuid admin_id) const
{
	if (!moderation_) throw std::logic_error("Build the platform before creating forms");
	return std::make_unique<administration_form>(*moderation_, admin_id);
}

marketplace_store &
platform_factory::store()
{ return *store_; }

catalog_service &
platform_factory::catalog()
{ return *catalog_; }

content_service &
platform_factory::content()
{ return *content_; }

user_service &
platform_factory::users()
{ return *users_; }

purchase_controller &
platform_factory::purchases()
{ return *purchases_; }

exchange_service &
platform_factory::exchanges()
{ return *exchanges_; }

moderation_service &
platform_factory::moderation()
{ return *moderation_; }

} // namespace marketplace
