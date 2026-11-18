#ifndef MARKETPLACE_STRATEGIES_HPP
#define MARKETPLACE_STRATEGIES_HPP

#include <memory>
#include <vector>

#include "marketplace/domain.hpp"

namespace marketplace {

class sales_rule {
public:
	virtual ~sales_rule() = default;

	virtual std::map<std::string, std::string>
	terms(uuid const & customer_id) const = 0;

	virtual bool
	allow(uuid const & customer_id) const = 0;

	virtual double
	price() const = 0;
};

class fixed_price : public sales_rule {
public:
	explicit fixed_price(double price);

	std::map<std::string, std::string>
	terms(uuid const & customer_id) const override;

	bool
	allow(uuid const & customer_id) const override;

	double
	price() const override;

private:
	double price_;
};

class delivery_strategy {
public:
	virtual ~delivery_strategy() = default;

	virtual bool
	check(access_rights const & access, std::string const & action) const = 0;

	virtual std::string
	deliver(content const & item, access_rights const & access) const = 0;
};

class direct_delivery : public delivery_strategy {
public:
	bool
	check(access_rights const & access, std::string const & action) const override;

	std::string
	deliver(content const & item, access_rights const & access) const override;
};

class product_selection {
public:
	virtual ~product_selection() = default;

	virtual std::vector<std::shared_ptr<product> >
	select(uuid const & user_id, std::string const & query, std::vector<std::shared_ptr<product> > const & products)
	    const = 0;
};

class filter_selection : public product_selection {
public:
	std::vector<std::shared_ptr<product> >
	select(uuid const & user_id, std::string const & query, std::vector<std::shared_ptr<product> > const & products)
	    const override;
};

} // namespace marketplace

#endif // MARKETPLACE_STRATEGIES_HPP
