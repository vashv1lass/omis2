#include "marketplace/store.hpp"

#include <memory>
#include <utility>

namespace marketplace {

namespace {

uuid
demo_id(std::string const & tail)
{
	std::string const suffix = std::string(12 - tail.size(), '0') + tail;
	return uuid::parse("00000000-0000-4000-8000-" + suffix);
}

} // namespace

marketplace_store::marketplace_store()
{
	auto const alex = std::make_shared<user>(
	    demo_id("01"), "user@demo.local", "", "Алексей", std::set<std::string>{ "buyer", "seller" }, 4.9);
	auto const studio = std::make_shared<user>(
	    demo_id("02"), "north@demo.local", "", "Studio North", std::set<std::string>{ "seller" }, 4.9);
	auto const anna = std::make_shared<user>(
	    demo_id("03"), "anna@demo.local", "", "Anna Visual", std::set<std::string>{ "seller" }, 4.8);
	auto const code = std::make_shared<user>(
	    demo_id("04"), "code@demo.local", "", "Code Notes", std::set<std::string>{ "seller" }, 4.9);
	auto const sound = std::make_shared<user>(
	    demo_id("05"), "sound@demo.local", "", "Sound Room", std::set<std::string>{ "seller" }, 4.7);
	auto const maria = std::make_shared<user>(
	    demo_id("06"), "admin@demo.local", "", "Мария", std::set<std::string>{ "administrator" }, 5.0);
	users = { alex, studio, anna, code, sound, maria };

	add_product_(
	    demo_id("101"),
	    "Focus Tools",
	    "Набор небольших приложений для Windows и Linux; ключ входит в комплект.",
	    "Программы",
	    studio,
	    35.0,
	    "Ключ",
	    true);
	add_product_(
	    demo_id("102"),
	    "Forma — UI kit",
	    "Соберите интерфейс своего следующего проекта. 84 компонента, 12 готовых "
	    "экранов и аккуратная система стилей для Figma.",
	    "Шаблоны",
	    studio,
	    29.0,
	    "Файл",
	    true);
	add_product_(
	    demo_id("103"),
	    "Python. От идеи к коду",
	    "Практическая книга для тех, кто начинает программировать. PDF, 210 страниц.",
	    "Книги",
	    code,
	    24.0,
	    "Файл",
	    true);
	add_product_(
	    demo_id("104"),
	    "Slow mornings",
	    "Набор спокойных музыкальных композиций. 16 аудиотреков в формате WAV.",
	    "Аудио",
	    sound,
	    12.0,
	    "Файл",
	    true);
	add_product_(
	    demo_id("105"),
	    "Монтаж: первый проект",
	    "Видеокурс по монтажу и цветокоррекции: 12 уроков, 3 часа.",
	    "Видео",
	    studio,
	    42.0,
	    "Ссылка",
	    true);
	add_product_(
	    demo_id("106"),
	    "Тихие пространства",
	    "Коллекция из 24 иллюстраций для личных и коммерческих проектов.",
	    "Графика",
	    anna,
	    18.0,
	    "Файл",
	    true);
	add_product_(
	    demo_id("107"),
	    "Мой набор иконок",
	    "Набор SVG-иконок пользователя для демонстрации обмена.",
	    "Графика",
	    alex,
	    15.0,
	    "Файл",
	    false);

	auto const ui_kit    = find_product(demo_id("102"));
	auto const spaces    = find_product(demo_id("106"));
	auto const own_icons = find_product(demo_id("107"));

	auto const purchase    = std::make_shared<deal>();
	uuid const purchase_id = purchase->record_purchase({ alex, studio }, 29.0);
	purchase->set_products({ ui_kit });
	purchase->confirm_purchase(purchase_id);
	purchase->complete_transaction();
	deals.push_back(purchase);

	auto const initial_access = std::make_shared<access_rights>();
	initial_access->id	  = uuid::generate();
	initial_access->issued_at = purchase->created_at();
	initial_access->basis	  = purchase;
	initial_access->product	  = ui_kit;
	initial_access->recipient = alex;
	rights.push_back(initial_access);

	auto const exchange = std::make_shared<deal>();
	exchange->propose_exchange({ alex, anna });
	exchange->set_products({ own_icons, spaces });
	deals.push_back(exchange);

	auto const sale	   = std::make_shared<deal>();
	uuid const sale_id = sale->record_purchase({ alex, studio }, 15.0);
	sale->set_products({ own_icons });
	sale->confirm_purchase(sale_id);
	sale->complete_transaction();
	deals.push_back(sale);

	auto const customer_review  = std::make_shared<review>();
	customer_review->id	    = uuid::generate();
	customer_review->rating	    = 5;
	customer_review->text	    = "Удобные материалы, всё соответствует описанию.";
	customer_review->created_at = std::chrono::system_clock::now();
	customer_review->deal	    = purchase;
	customer_review->product    = ui_kit;
	customer_review->author	    = alex;
	reviews.push_back(customer_review);

	auto const first_complaint   = std::make_shared<complaint>();
	first_complaint->id	     = uuid::generate();
	first_complaint->object_type = "product";
	first_complaint->object_id   = ui_kit->id();
	first_complaint->reason	     = "Нарушение условий";
	first_complaint->status	     = "new";
	first_complaint->author	     = alex;
	complaints.push_back(first_complaint);

	auto const second_complaint   = std::make_shared<complaint>();
	second_complaint->id	      = uuid::generate();
	second_complaint->object_type = "deal";
	second_complaint->object_id   = purchase->id();
	second_complaint->reason      = "Нет доступа";
	second_complaint->status      = "under_review";
	second_complaint->author      = alex;
	complaints.push_back(second_complaint);
}

std::shared_ptr<user>
marketplace_store::find_user(uuid const & id) const
{
	for (auto const & u : users) {
		if (u != nullptr && u->id == id) return u;
	}
	return nullptr;
}

std::shared_ptr<product>
marketplace_store::find_product(uuid const & id) const
{
	for (auto const & p : products) {
		if (p != nullptr && p->id() == id) return p;
	}
	return nullptr;
}

std::shared_ptr<deal>
marketplace_store::find_deal(uuid const & id) const
{
	for (auto const & d : deals) {
		if (d != nullptr && d->id() == id) return d;
	}
	return nullptr;
}

std::shared_ptr<complaint>
marketplace_store::find_complaint(uuid const & id) const
{
	for (auto const & c : complaints) {
		if (c != nullptr && c->id == id) return c;
	}
	return nullptr;
}

void
marketplace_store::add_product_(
    uuid const &		  id,
    std::string const &		  name,
    std::string const &		  description,
    std::string const &		  category,
    std::shared_ptr<user> const & seller,
    double			  price,
    std::string const &		  type,
    bool			  published)
{
	auto const item = std::make_shared<product>(id, name, description, category, seller, price);
	item->set_published(published);
	item->set_usage_terms("Для личных проектов. Перепродажа исходных материалов не разрешена.");

	auto const stored_content	= std::make_shared<content>();
	stored_content->id		= uuid::generate();
	stored_content->type		= type;
	stored_content->storage_address = "demo://" + id.str();
	stored_content->checksum	= "demo-checksum";
	item->set_material(stored_content);

	products.push_back(item);
	contents.push_back(stored_content);
}

} // namespace marketplace
