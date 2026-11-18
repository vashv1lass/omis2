#include "marketplace/strategies.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace marketplace {

namespace {

std::string
lowercase(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

} // namespace

fixed_price::fixed_price(double price) : price_(price) {}

std::map<std::string, std::string>
fixed_price::terms(uuid const & customer_id) const
{
	std::ostringstream value;
	value << std::fixed << std::setprecision(2) << price_;
	return {
		{ "price",	 value.str()	   },
		 { "currency",	  "BYN"		    },
		       { "customer_id", customer_id.str() }
	};
}

bool
fixed_price::allow(uuid const & customer_id) const
{ return !customer_id.empty() && price_ >= 0.0; }

double
fixed_price::price() const
{ return price_; }

bool
direct_delivery::check(access_rights const & access, std::string const & action) const
{ return access.active && access.recipient != nullptr && access.product != nullptr && !action.empty(); }

std::string
direct_delivery::deliver(content const & item, access_rights const & access) const
{ return check(access, "deliver") ? item.storage_address : std::string{}; }

std::vector<std::shared_ptr<product> >
filter_selection::select(
    uuid const & user_id, std::string const & query, std::vector<std::shared_ptr<product> > const & products) const
{
	static_cast<void>(user_id);
	std::string const		       q = lowercase(query);
	std::vector<std::shared_ptr<product> > result;

	for (auto const & p : products) {
		if (p == nullptr || !p->published()) continue;

		std::string const text = lowercase(
		    p->name() + " " + p->description() + " " + p->category() + " " +
		    (p->seller() == nullptr ? "" : p->seller()->name));
		if (q.empty() || text.find(q) != std::string::npos) result.push_back(p);
	}

	return result;
}

} // namespace marketplace
