#include "marketplace/domain.hpp"

#include <algorithm>
#include <random>
#include <utility>

#include "marketplace/strategies.hpp"

namespace marketplace {

uuid
uuid::generate()
{
	static std::random_device device;
	static std::mt19937_64	  generator(device());

	uuid id;
	for (int i = 0; i < 16; i += 8) {
		std::uint64_t const value = generator();
		for (int j = 0; j < 8; ++j) id.bytes_[i + j] = static_cast<std::uint8_t>(value >> (j * 8));
	}

	id.bytes_[6] = static_cast<std::uint8_t>((id.bytes_[6] & 0x0f) | 0x40);
	id.bytes_[8] = static_cast<std::uint8_t>((id.bytes_[8] & 0x3f) | 0x80);
	return id;
}

uuid
uuid::parse(std::string const & value)
{
	uuid	     id;
	std::size_t  byte = 0;
	std::int32_t high = -1;

	for (char c : value) {
		if (c == '-') continue;

		std::int32_t digit = -1;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'a' && c <= 'f')
			digit = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			digit = c - 'A' + 10;
		else
			return {};

		if (high < 0) {
			high = digit;
		} else {
			if (byte >= id.bytes_.size()) return {};
			id.bytes_[byte++] = static_cast<std::uint8_t>(high * 16 + digit);
			high		  = -1;
		}
	}

	if (byte != id.bytes_.size() || high >= 0) return {};
	return id;
}

std::string
uuid::str() const
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string	      value;
	value.reserve(36);

	for (int i = 0; i < static_cast<int>(bytes_.size()); ++i) {
		if (i == 4 || i == 6 || i == 8 || i == 10) value.push_back('-');
		value.push_back(digits[bytes_[i] >> 4]);
		value.push_back(digits[bytes_[i] & 0x0f]);
	}

	return value;
}

bool
uuid::empty() const
{
	return std::all_of(bytes_.begin(), bytes_.end(), [](std::uint8_t byte) { return byte == 0; });
}

bool
uuid::operator==(uuid const & other) const
{ return bytes_ == other.bytes_; }

bool
uuid::operator!=(uuid const & other) const
{ return !(*this == other); }

bool
uuid::operator<(uuid const & other) const
{ return bytes_ < other.bytes_; }

user::user(
    uuid id, std::string email, std::string password_hash, std::string name, std::set<std::string> roles, double rating)
    : id(id), email(std::move(email)), password_hash(std::move(password_hash)), name(std::move(name)),
      roles(std::move(roles)), rating(rating)
{}

void
user::block()
{ blocked = true; }

product::product(
    uuid		  id,
    std::string		  name,
    std::string		  description,
    std::string		  category,
    std::shared_ptr<user> seller,
    double		  price)
    : id_(id), name_(std::move(name)), description_(std::move(description)), category_(std::move(category)),
      seller_(std::move(seller)), sales_rule_(std::make_shared<fixed_price>(price))
{}

std::map<std::string, std::string>
product::purchase_terms(uuid const & customer_id) const
{
	if (sales_rule_ != nullptr) return sales_rule_->terms(customer_id);
	return {};
}

bool
product::can_purchase(uuid const & customer_id) const
{
	if (!published_ || seller_ == nullptr || seller_->id == customer_id || !hidden_reason_.empty()) return false;
	return sales_rule_ == nullptr || sales_rule_->allow(customer_id);
}

void
product::change_rules(std::shared_ptr<sales_rule> new_rules)
{ sales_rule_ = std::move(new_rules); }

std::shared_ptr<sales_rule>
product::rules() const
{ return sales_rule_; }

void
product::hide(std::string const & reason)
{
	published_     = false;
	hidden_reason_ = reason;
}

double
product::price() const
{ return sales_rule_ == nullptr ? 0.0 : sales_rule_->price(); }

std::string
product::hidden_reason() const
{ return hidden_reason_; }

uuid
product::id() const
{ return id_; }

void
product::set_id(uuid id)
{ id_ = id; }

std::string const &
product::name() const
{ return name_; }

void
product::set_name(std::string name)
{ name_ = std::move(name); }

std::string const &
product::description() const
{ return description_; }

void
product::set_description(std::string description)
{ description_ = std::move(description); }

std::string const &
product::category() const
{ return category_; }

void
product::set_category(std::string category)
{ category_ = std::move(category); }

bool
product::published() const
{ return published_; }

void
product::set_published(bool published)
{ published_ = published; }

std::shared_ptr<user>
product::seller() const
{ return seller_; }

void
product::set_seller(std::shared_ptr<user> seller)
{ seller_ = std::move(seller); }

std::shared_ptr<content>
product::material() const
{ return material_; }

void
product::set_material(std::shared_ptr<content> material)
{ material_ = std::move(material); }

std::string const &
product::usage_terms() const
{ return usage_terms_; }

void
product::set_usage_terms(std::string terms)
{ usage_terms_ = std::move(terms); }

uuid
deal::record_purchase(std::pair<std::shared_ptr<user>, std::shared_ptr<user> > participants, double price)
{
	id_	      = uuid::generate();
	type_	      = "purchase";
	status_	      = "pending_confirmation";
	price_	      = price;
	created_at_   = std::chrono::system_clock::now();
	participants_ = participants;
	products_.clear();
	return id_;
}

void
deal::confirm_purchase(uuid const & deal_id)
{
	if (id_ == deal_id && status_ == "pending_confirmation") status_ = "confirmed";
}

uuid
deal::propose_exchange(std::pair<std::shared_ptr<user>, std::shared_ptr<user> > participants)
{
	id_	      = uuid::generate();
	type_	      = "exchange";
	status_	      = "waiting_reply";
	price_	      = 0.0;
	created_at_   = std::chrono::system_clock::now();
	participants_ = participants;
	products_.clear();
	return id_;
}

void
deal::complete_exchange()
{
	if (type_ == "exchange" && status_ == "waiting_reply") complete_transaction();
}

void
deal::complete_transaction()
{
	if (status_ == "confirmed" || status_ == "waiting_reply") status_ = "completed";
}

void
deal::cancel(std::string const & reason)
{
	if (status_ != "completed") {
		status_		     = "cancelled";
		cancellation_reason_ = reason;
	}
}

uuid
deal::id() const
{ return id_; }

std::string
deal::type() const
{ return type_; }

std::string
deal::status() const
{ return status_; }

double
deal::price() const
{ return price_; }

std::chrono::system_clock::time_point
deal::created_at() const
{ return created_at_; }

std::pair<std::shared_ptr<user>, std::shared_ptr<user> > const &
deal::participants() const
{ return participants_; }

std::vector<std::shared_ptr<product> > const &
deal::products() const
{ return products_; }

void
deal::set_products(std::vector<std::shared_ptr<product> > products)
{ products_ = std::move(products); }

} // namespace marketplace
