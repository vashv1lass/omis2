#include "marketplace/services.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace marketplace {

namespace {

std::string
password_hash(std::string const & password)
{
	std::ostringstream value;
	value << std::hex << std::hash<std::string>{}(password);
	return value.str();
}

} // namespace

catalog_service::catalog_service(marketplace_store & store, product_selection const & selection)
    : store_(store), selection_(selection)
{}

std::vector<std::shared_ptr<product> >
catalog_service::search(uuid const & user_id, std::string const & query) const
{ return selection_.select(user_id, query, store_.products); }

std::shared_ptr<product>
catalog_service::get_product(uuid const & product_id) const
{ return store_.find_product(product_id); }

std::vector<std::shared_ptr<review> >
catalog_service::get_reviews(uuid const & product_id) const
{
	std::vector<std::shared_ptr<review> > result;
	for (auto const & r : store_.reviews) {
		if (r->product != nullptr && r->product->id() == product_id) result.push_back(r);
	}
	return result;
}

uuid
catalog_service::publish(uuid const & user_id, std::shared_ptr<product> const & item)
{
	auto const seller = store_.find_user(user_id);
	if (seller == nullptr || seller->blocked || item == nullptr) return {};

	if (item->id().empty()) item->set_id(uuid::generate());
	item->set_seller(seller);
	item->set_published(true);
	if (item->rules() == nullptr) item->change_rules(std::make_shared<fixed_price>(0.0));
	store_.products.push_back(item);
	return item->id();
}

void
catalog_service::change_rules(uuid const & product_id, std::shared_ptr<sales_rule> rules)
{
	auto const item = store_.find_product(product_id);
	if (item != nullptr) item->change_rules(std::move(rules));
}

void
catalog_service::hide(uuid const & product_id, std::string const & reason)
{
	auto const item = store_.find_product(product_id);
	if (item != nullptr) item->hide(reason);
}

content_service::content_service(marketplace_store & store, delivery_strategy const & delivery)
    : store_(store), delivery_(delivery)
{}

void
content_service::upload(uuid const & product_id, std::string const & type, std::string const & address)
{
	auto const item = store_.find_product(product_id);
	if (item == nullptr) return;

	auto const stored_content	= std::make_shared<content>();
	stored_content->id		= uuid::generate();
	stored_content->type		= type;
	stored_content->storage_address = address;
	stored_content->checksum	= "demo-checksum";
	item->set_material(stored_content);
	store_.contents.push_back(stored_content);
}

bool
content_service::check_availability(uuid const & product_id, uuid const & seller_id) const
{
	auto const item = store_.find_product(product_id);
	if (item == nullptr || item->seller() == nullptr || item->seller()->id != seller_id ||
	    item->seller()->blocked || item->material() == nullptr)
		return false;
	return !item->material()->storage_address.empty();
}

bool
content_service::grant_access(uuid const & customer_id, uuid const & product_id, uuid const & deal_id)
{
	auto const customer    = store_.find_user(customer_id);
	auto const item	       = store_.find_product(product_id);
	auto const transaction = store_.find_deal(deal_id);
	if (customer == nullptr || item == nullptr || item->seller() == nullptr || transaction == nullptr ||
	    transaction->status() != "completed" || !check_availability(product_id, item->seller()->id) ||
	    (transaction->participants().first != customer && transaction->participants().second != customer))
		return false;

	auto const & products = transaction->products();
	if (std::find(products.begin(), products.end(), item) == products.end()) return false;

	auto const rule = std::make_shared<access_rights>();
	rule->id	= uuid::generate();
	rule->issued_at = std::chrono::system_clock::now();
	rule->basis	= transaction;
	rule->product	= item;
	rule->recipient = customer;
	store_.rights.push_back(rule);
	return true;
}

std::string
content_service::get_content_for_access(uuid const & customer_id, uuid const & product_id) const
{
	auto const customer = store_.find_user(customer_id);
	auto const item	    = store_.find_product(product_id);
	if (customer == nullptr || item == nullptr || item->material() == nullptr) return {};

	for (auto const & rule : store_.rights) {
		if (rule != nullptr && rule->recipient == customer && rule->product == item &&
		    delivery_.check(*rule, "download"))
			return delivery_.deliver(*item->material(), *rule);
	}
	return {};
}

user_service::user_service(marketplace_store & store) : store_(store)
{
	for (auto const & u : store_.users) u->password_hash = password_hash("demo");
}

uuid
user_service::register_user(std::string const & email, std::string const & name, std::string const & password)
{
	if (email.empty() || name.empty() || password.size() < 6) return {};
	for (auto const & u : store_.users) {
		if (u->email == email) return {};
	}

	uuid const id = uuid::generate();
	auto const u  = std::make_shared<user>(
	    id, email, password_hash(password), name, std::set<std::string>{ "buyer", "seller" });
	store_.users.push_back(u);
	return id;
}

uuid
user_service::authenticate(std::string const & email, std::string const & password) const
{
	std::string const hash = password_hash(password);
	for (auto const & u : store_.users) {
		if (u->email == email && u->password_hash == hash && !u->blocked) return u->id;
	}
	return {};
}

void
user_service::update_profile(uuid const & id, std::string const & name)
{
	auto const u = store_.find_user(id);
	if (u != nullptr) u->name = name;
}

void
user_service::rate_user(uuid const & id, double rating)
{
	auto const u = store_.find_user(id);
	if (u != nullptr) u->rating = std::clamp(rating, 0.0, 5.0);
}

std::vector<std::shared_ptr<deal> >
user_service::get_history(uuid const & user_id) const
{
	auto const u = store_.find_user(user_id);
	if (u == nullptr) return {};

	std::vector<std::shared_ptr<deal> > history;
	for (auto const & transaction : store_.deals) {
		if (transaction->participants().first == u || transaction->participants().second == u)
			history.push_back(transaction);
	}
	return history;
}

std::shared_ptr<user>
user_service::get_user(uuid const & id) const
{ return store_.find_user(id); }

void
user_service::block(uuid const & id, std::string const & reason)
{
	static_cast<void>(reason);
	auto const u = store_.find_user(id);
	if (u != nullptr) u->block();
}

purchase_controller::purchase_controller(
    marketplace_store & store, catalog_service & catalog, content_service & content, user_service & users)
    : store_(store), catalog_(catalog), content_(content), users_(users)
{}

std::map<std::string, std::string>
purchase_controller::get_purchase_terms(uuid const & product_id) const
{
	auto const item = catalog_.get_product(product_id);
	return item == nullptr ? std::map<std::string, std::string>{} : item->purchase_terms({});
}

std::string
purchase_controller::make_purchase(uuid const & buyer_id, uuid const & product_id)
{
	auto const buyer = users_.get_user(buyer_id);
	auto const item	 = catalog_.get_product(product_id);
	if (buyer == nullptr || buyer->blocked || item == nullptr || item->seller() == nullptr ||
	    !item->can_purchase(buyer_id) || !content_.check_availability(product_id, item->seller()->id))
		return {};

	auto const transaction = std::make_shared<deal>();
	uuid const deal_id     = transaction->record_purchase({ buyer, item->seller() }, item->price());
	transaction->set_products({ item });
	transaction->confirm_purchase(deal_id);
	transaction->complete_transaction();
	store_.deals.push_back(transaction);

	if (!content_.grant_access(buyer_id, product_id, deal_id)) {
		transaction->cancel("Content access could not be issued");
		return {};
	}
	return deal_id.str();
}

std::string
purchase_controller::get_purchased_product(uuid const & buyer_id, uuid const & product_id) const
{ return content_.get_content_for_access(buyer_id, product_id); }

exchange_service::exchange_service(marketplace_store & store, content_service & content)
    : store_(store), content_(content)
{}

uuid
exchange_service::propose(uuid const & proposer_id, uuid const & desired_product_id, uuid const & offered_product_id)
{
	auto const proposer = store_.find_user(proposer_id);
	auto const desired  = store_.find_product(desired_product_id);
	auto const offered  = store_.find_product(offered_product_id);
	if (proposer == nullptr || desired == nullptr || offered == nullptr || desired->seller() == nullptr ||
	    desired->seller() == proposer || offered->seller() != proposer ||
	    !content_.check_availability(desired_product_id, desired->seller()->id) ||
	    !content_.check_availability(offered_product_id, proposer->id))
		return {};

	auto const transaction = std::make_shared<deal>();
	uuid const id	       = transaction->propose_exchange({ proposer, desired->seller() });
	transaction->set_products({ desired, offered });
	store_.deals.push_back(transaction);
	return id;
}

void
exchange_service::accept(uuid const & deal_id, uuid const & seller_id)
{
	auto const transaction = store_.find_deal(deal_id);
	auto const seller      = store_.find_user(seller_id);
	if (transaction == nullptr || seller == nullptr || transaction->type() != "exchange" ||
	    transaction->status() != "waiting_reply" ||
	    (transaction->participants().first != seller && transaction->participants().second != seller))
		return;

	auto const & products = transaction->products();
	if (products.size() != 2 || products[0]->seller() != seller ||
	    !content_.check_availability(products[0]->id(), products[0]->seller()->id) ||
	    !content_.check_availability(products[1]->id(), products[1]->seller()->id))
		return;

	transaction->complete_exchange();
	auto const grant = [this, deal_id, &products](std::shared_ptr<user> const & participant) {
		auto const received = participant == products[0]->seller() ? products[1] : products[0];
		content_.grant_access(participant->id, received->id(), deal_id);
	};
	grant(transaction->participants().first);
	grant(transaction->participants().second);
}

void
exchange_service::reject(uuid const & deal_id)
{
	auto const transaction = store_.find_deal(deal_id);
	if (transaction != nullptr && transaction->type() == "exchange")
		transaction->cancel("Exchange proposal was declined");
}

moderation_service::moderation_service(marketplace_store & store, catalog_service & catalog, user_service & users)
    : store_(store), catalog_(catalog), users_(users)
{}

void
moderation_service::review_publication(uuid const & product_id)
{
	auto const item = store_.find_product(product_id);
	if (item != nullptr) item->set_published(true);
}

void
moderation_service::resolve_complaint(uuid const & complaint_id, std::string const & decision)
{
	auto const c = store_.find_complaint(complaint_id);
	if (c == nullptr) return;

	c->decision = decision;
	c->status   = "resolved";
	if (decision == "hide" && c->object_type == "product")
		catalog_.hide(c->object_id, c->reason);
	else if (decision == "block" && c->object_type == "user")
		users_.block(c->object_id, c->reason);
}

void
moderation_service::review_complaint(uuid const & admin_id, uuid const & complaint_id, std::string const & decision)
{
	auto const admin = users_.get_user(admin_id);
	if (admin != nullptr && admin->roles.count("administrator") != 0) resolve_complaint(complaint_id, decision);
}

void
moderation_service::hide_product(uuid const & product_id, std::string const & reason)
{ catalog_.hide(product_id, reason); }

void
moderation_service::block_user(uuid const & user_id, std::string const & reason)
{ users_.block(user_id, reason); }

std::string
moderation_service::generate_report(
    std::chrono::system_clock::time_point const & start, std::chrono::system_clock::time_point const & end) const
{
	static_cast<void>(start);
	static_cast<void>(end);
	std::size_t completed = 0;
	std::size_t pending   = 0;
	double	    sales     = 0.0;

	for (auto const & transaction : store_.deals) {
		if (transaction->status() == "completed") {
			++completed;
			sales += transaction->price();
		} else if (transaction->status() == "waiting_reply") {
			++pending;
		}
	}

	std::ostringstream report;
	report << "Сделок: " << store_.deals.size() << "; завершено: " << completed << "; ожидает ответа: " << pending
	       << "; продажи: " << sales << " BYN";
	return report.str();
}

} // namespace marketplace
