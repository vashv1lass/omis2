#include "marketplace/account_moderation.hpp"

#include <algorithm>
#include <utility>

namespace marketplace {

account_form::account_form(user_service & users) : users_(users) {}

void
account_form::show() const
{ static_cast<void>(users_); }

uuid
account_form::register_account(std::string const & email, std::string const & password, std::string const & name)
{
	user_id_ = users_.register_user(email, name, password);
	return user_id_;
}

uuid
account_form::login(std::string const & email, std::string const & password)
{
	user_id_ = users_.authenticate(email, password);
	return user_id_;
}

std::vector<std::shared_ptr<deal> >
account_form::open_history() const
{ return users_.get_history(user_id_); }

feedback_form::feedback_form(marketplace_store & store, uuid user_id) : store_(store), user_id_(user_id) {}

void
feedback_form::show() const
{ static_cast<void>(store_); }

uuid
feedback_form::leave_review(uuid const & deal_id, std::int32_t rating, std::string const & text)
{
	auto const transaction = store_.find_deal(deal_id);
	auto const author      = store_.find_user(user_id_);
	if (transaction == nullptr || author == nullptr || transaction->status() != "completed" ||
	    (transaction->participants().first != author && transaction->participants().second != author) ||
	    text.empty() || transaction->products().empty())
		return {};

	auto const customer_review  = std::make_shared<review>();
	customer_review->id	    = uuid::generate();
	customer_review->rating	    = std::clamp<std::int32_t>(rating, 1, 5);
	customer_review->text	    = text;
	customer_review->created_at = std::chrono::system_clock::now();
	customer_review->deal	    = transaction;
	customer_review->product    = transaction->products().front();
	customer_review->author	    = author;
	store_.reviews.push_back(customer_review);
	return customer_review->id;
}

uuid
feedback_form::submit_complaint(std::string const & object_type, uuid const & object_id, std::string const & reason)
{
	if (object_type.empty() || object_id.empty() || reason.empty()) return {};

	auto const author = store_.find_user(user_id_);
	if (author == nullptr) return {};

	if ((object_type == "product" && store_.find_product(object_id) == nullptr) ||
	    (object_type == "user" && store_.find_user(object_id) == nullptr) ||
	    (object_type == "deal" && store_.find_deal(object_id) == nullptr) ||
	    (object_type != "product" && object_type != "user" && object_type != "deal"))
		return {};

	auto const appeal   = std::make_shared<complaint>();
	appeal->id	    = uuid::generate();
	appeal->object_type = object_type;
	appeal->object_id   = object_id;
	appeal->reason	    = reason;
	appeal->status	    = "new";
	appeal->author	    = author;
	store_.complaints.push_back(appeal);
	return appeal->id;
}

administration_form::administration_form(moderation_service & moderation, uuid admin_id)
    : moderation_(moderation), admin_id_(admin_id)
{}

void
administration_form::show() const
{ static_cast<void>(moderation_); }

void
administration_form::review_publication(uuid const & product_id)
{ moderation_.review_publication(product_id); }

void
administration_form::process_complaint(uuid const & complaint_id, std::string const & decision)
{ moderation_.review_complaint(admin_id_, complaint_id, decision); }

void
administration_form::block_user(uuid const & user_id, std::string const & reason)
{ moderation_.block_user(user_id, reason); }

std::string
administration_form::generate_report(
    std::chrono::system_clock::time_point const & start, std::chrono::system_clock::time_point const & end) const
{ return moderation_.generate_report(start, end); }

} // namespace marketplace
