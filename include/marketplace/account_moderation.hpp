#ifndef MARKETPLACE_ACCOUNT_MODERATION_HPP
#define MARKETPLACE_ACCOUNT_MODERATION_HPP

#include "marketplace/services.hpp"

namespace marketplace {

class account_form {
public:
	explicit account_form(user_service & users);

	void
	show() const;

	uuid
	register_account(std::string const & email, std::string const & password, std::string const & name);

	uuid
	login(std::string const & email, std::string const & password);

	std::vector<std::shared_ptr<deal> >
	open_history() const;

private:
	user_service & users_;
	uuid	       user_id_;
};

class feedback_form {
public:
	feedback_form(marketplace_store & store, uuid user_id);

	void
	show() const;

	uuid
	leave_review(uuid const & deal_id, std::int32_t rating, std::string const & text);

	uuid
	submit_complaint(std::string const & object_type, uuid const & object_id, std::string const & reason);

private:
	marketplace_store & store_;
	uuid		    user_id_;
};

class administration_form {
public:
	administration_form(moderation_service & moderation, uuid admin_id);

	void
	show() const;

	void
	review_publication(uuid const & product_id);

	void
	process_complaint(uuid const & complaint_id, std::string const & decision);

	void
	block_user(uuid const & user_id, std::string const & reason);

	std::string
	generate_report(
	    std::chrono::system_clock::time_point const & start,
	    std::chrono::system_clock::time_point const & end) const;

private:
	moderation_service & moderation_;
	uuid		     admin_id_;
};

} // namespace marketplace

#endif // MARKETPLACE_ACCOUNT_MODERATION_HPP
