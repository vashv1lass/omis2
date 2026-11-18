#ifndef MARKETPLACE_SERVICES_HPP
#define MARKETPLACE_SERVICES_HPP

#include <memory>
#include <string>
#include <vector>

#include "marketplace/store.hpp"
#include "marketplace/strategies.hpp"

namespace marketplace {

class catalog_service {
public:
	catalog_service(marketplace_store & store, product_selection const & selection);

	std::vector<std::shared_ptr<product> >
	search(uuid const & user_id, std::string const & query) const;

	std::shared_ptr<product>
	get_product(uuid const & product_id) const;

	std::vector<std::shared_ptr<review> >
	get_reviews(uuid const & product_id) const;

	uuid
	publish(uuid const & user_id, std::shared_ptr<product> const & item);

	void
	change_rules(uuid const & product_id, std::shared_ptr<sales_rule> rules);

	void
	hide(uuid const & product_id, std::string const & reason);

private:
	marketplace_store &	  store_;
	product_selection const & selection_;
};

class content_service {
public:
	content_service(marketplace_store & store, delivery_strategy const & delivery);

	void
	upload(uuid const & product_id, std::string const & type, std::string const & address);

	bool
	check_availability(uuid const & product_id, uuid const & seller_id) const;

	bool
	grant_access(uuid const & customer_id, uuid const & product_id, uuid const & deal_id);

	std::string
	get_content_for_access(uuid const & customer_id, uuid const & product_id) const;

private:
	marketplace_store &	  store_;
	delivery_strategy const & delivery_;
};

class user_service {
public:
	explicit user_service(marketplace_store & store);

	uuid
	register_user(std::string const & email, std::string const & name, std::string const & password);

	uuid
	authenticate(std::string const & email, std::string const & password) const;

	void
	update_profile(uuid const & id, std::string const & name);

	void
	rate_user(uuid const & id, double rating);

	std::vector<std::shared_ptr<deal> >
	get_history(uuid const & user_id) const;

	std::shared_ptr<user>
	get_user(uuid const & id) const;

	void
	block(uuid const & id, std::string const & reason);

private:
	marketplace_store & store_;
};

class purchase_controller {
public:
	purchase_controller(
	    marketplace_store & store, catalog_service & catalog, content_service & content, user_service & users);

	std::map<std::string, std::string>
	get_purchase_terms(uuid const & product_id) const;

	std::string
	make_purchase(uuid const & buyer_id, uuid const & product_id);

	std::string
	get_purchased_product(uuid const & buyer_id, uuid const & product_id) const;

private:
	marketplace_store & store_;
	catalog_service &   catalog_;
	content_service &   content_;
	user_service &	    users_;
};

class exchange_service {
public:
	exchange_service(marketplace_store & store, content_service & content);

	uuid
	propose(uuid const & proposer_id, uuid const & desired_product_id, uuid const & offered_product_id);

	void
	accept(uuid const & deal_id, uuid const & seller_id);

	void
	reject(uuid const & deal_id);

private:
	marketplace_store & store_;
	content_service &   content_;
};

class moderation_service {
public:
	moderation_service(marketplace_store & store, catalog_service & catalog, user_service & users);

	void
	review_publication(uuid const & product_id);

	void
	resolve_complaint(uuid const & complaint_id, std::string const & decision);

	void
	review_complaint(uuid const & admin_id, uuid const & complaint_id, std::string const & decision);

	void
	hide_product(uuid const & product_id, std::string const & reason);

	void
	block_user(uuid const & user_id, std::string const & reason);

	std::string
	generate_report(
	    std::chrono::system_clock::time_point const & start,
	    std::chrono::system_clock::time_point const & end) const;

private:
	marketplace_store & store_;
	catalog_service &   catalog_;
	user_service &	    users_;
};

} // namespace marketplace

#endif // MARKETPLACE_SERVICES_HPP
