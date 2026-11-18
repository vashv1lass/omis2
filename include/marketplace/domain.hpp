#ifndef MARKETPLACE_DOMAIN_HPP
#define MARKETPLACE_DOMAIN_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace marketplace {

class uuid {
public:
	uuid() = default;

	static uuid
	generate();

	static uuid
	parse(std::string const & value);

	std::string
	str() const;

	bool
	empty() const;

	bool
	operator==(uuid const & other) const;

	bool
	operator!=(uuid const & other) const;

	bool
	operator<(uuid const & other) const;

private:
	std::array<std::uint8_t, 16> bytes_{};
};

class user;
class content;
class access_rights;
class review;
class complaint;
class product;
class deal;
class sales_rule;

class user {
public:
	user() = default;
	user(
	    uuid		  id,
	    std::string		  email,
	    std::string		  password_hash,
	    std::string		  name,
	    std::set<std::string> roles,
	    double		  rating = 0.0);

	void
	block();

	uuid		      id;
	std::string	      email;
	std::string	      password_hash;
	std::string	      name;
	std::set<std::string> roles;
	double		      rating  = 0.0;
	bool		      blocked = false;
};

class content {
public:
	uuid	    id;
	std::string type;
	std::string storage_address;
	std::string checksum;
};

class access_rights {
public:
	uuid					 id;
	std::chrono::system_clock::time_point	 issued_at{};
	bool					 active = true;
	std::shared_ptr<deal>			 basis;
	std::shared_ptr< ::marketplace::product> product;
	std::shared_ptr<user>			 recipient;
};

class review {
public:
	uuid					 id;
	std::int32_t				 rating = 0;
	std::string				 text;
	std::chrono::system_clock::time_point	 created_at{};
	std::shared_ptr< ::marketplace::deal>	 deal;
	std::shared_ptr< ::marketplace::product> product;
	std::shared_ptr<user>			 author;
};

class complaint {
public:
	uuid		      id;
	std::string	      object_type;
	uuid		      object_id;
	std::string	      reason;
	std::string	      status;
	std::string	      decision;
	std::shared_ptr<user> author;
};

class product {
public:
	product() = default;
	product(
	    uuid		  id,
	    std::string		  name,
	    std::string		  description,
	    std::string		  category,
	    std::shared_ptr<user> seller,
	    double		  price);

	std::map<std::string, std::string>
	purchase_terms(uuid const & customer_id) const;

	bool
	can_purchase(uuid const & customer_id) const;

	void
	change_rules(std::shared_ptr<sales_rule> rules);

	std::shared_ptr<sales_rule>
	rules() const;

	void
	hide(std::string const & reason);

	double
	price() const;

	std::string
	hidden_reason() const;

	uuid
	id() const;

	void
	set_id(uuid id);

	std::string const &
	name() const;

	void
	set_name(std::string name);

	std::string const &
	description() const;

	void
	set_description(std::string description);

	std::string const &
	category() const;

	void
	set_category(std::string category);

	bool
	published() const;

	void
	set_published(bool published);

	std::shared_ptr<user>
	seller() const;

	void
	set_seller(std::shared_ptr<user> seller);

	std::shared_ptr<content>
	material() const;

	void
	set_material(std::shared_ptr<content> material);

	std::string const &
	usage_terms() const;

	void
	set_usage_terms(std::string terms);

private:
	uuid			    id_;
	std::string		    name_;
	std::string		    description_;
	std::string		    category_;
	bool			    published_ = false;
	std::shared_ptr<user>	    seller_;
	std::shared_ptr<content>    material_;
	std::shared_ptr<sales_rule> sales_rule_;
	std::string		    usage_terms_;
	std::string		    hidden_reason_;
};

class deal {
public:
	deal() = default;

	uuid
	record_purchase(std::pair<std::shared_ptr<user>, std::shared_ptr<user> > participants, double price);

	void
	confirm_purchase(uuid const & deal_id);

	uuid
	propose_exchange(std::pair<std::shared_ptr<user>, std::shared_ptr<user> > participants);

	void
	complete_exchange();

	void
	complete_transaction();

	void
	cancel(std::string const & reason);

	uuid
	id() const;

	std::string
	type() const;

	std::string
	status() const;

	double
	price() const;

	std::chrono::system_clock::time_point
	created_at() const;

	std::pair<std::shared_ptr<user>, std::shared_ptr<user> > const &
	participants() const;

	std::vector<std::shared_ptr<product> > const &
	products() const;

	void
	set_products(std::vector<std::shared_ptr<product> > products);

private:
	uuid							 id_;
	std::string						 type_;
	std::string						 status_;
	double							 price_ = 0.0;
	std::chrono::system_clock::time_point			 created_at_{};
	std::pair<std::shared_ptr<user>, std::shared_ptr<user> > participants_;
	std::vector<std::shared_ptr<product> >			 products_;
	std::string						 cancellation_reason_;
};

} // namespace marketplace

#endif // MARKETPLACE_DOMAIN_HPP
