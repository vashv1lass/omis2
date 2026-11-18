#ifndef MARKETPLACE_STORE_HPP
#define MARKETPLACE_STORE_HPP

#include <memory>
#include <vector>

#include "marketplace/domain.hpp"

namespace marketplace {

class marketplace_store {
      public:
	marketplace_store();

	std::shared_ptr<user>
	find_user(uuid const & id) const;

	std::shared_ptr<product>
	find_product(uuid const & id) const;

	std::shared_ptr<deal>
	find_deal(uuid const & id) const;

	std::shared_ptr<complaint>
	find_complaint(uuid const & id) const;

	std::vector<std::shared_ptr<user> >	     users;
	std::vector<std::shared_ptr<product> >	     products;
	std::vector<std::shared_ptr<content> >	     contents;
	std::vector<std::shared_ptr<deal> >	     deals;
	std::vector<std::shared_ptr<review> >	     reviews;
	std::vector<std::shared_ptr<complaint> >     complaints;
	std::vector<std::shared_ptr<access_rights> > rights;

      private:
	void
	add_product_(
	    uuid const &		  id,
	    std::string const &		  name,
	    std::string const &		  description,
	    std::string const &		  category,
	    std::shared_ptr<user> const & seller,
	    double			  price,
	    std::string const &		  type,
	    bool			  published);
};

} // namespace marketplace

#endif // MARKETPLACE_STORE_HPP
