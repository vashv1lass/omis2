#include "marketplace/web_server.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include "marketplace/account_moderation.hpp"

namespace marketplace {

namespace {

std::string
json_escape(std::string const & text)
{
	std::ostringstream out;
	for (unsigned char c : text) {
		switch (c) {
		case '"':
			out << "\\\"";
			break;
		case '\\':
			out << "\\\\";
			break;
		case '\b':
			out << "\\b";
			break;
		case '\f':
			out << "\\f";
			break;
		case '\n':
			out << "\\n";
			break;
		case '\r':
			out << "\\r";
			break;
		case '\t':
			out << "\\t";
			break;
		default:
			if (c < 0x20)
				out << '?';
			else
				out << static_cast<char>(c);
		}
	}
	return out.str();
}

std::int32_t
hex_value(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

std::string
decode(std::string const & text)
{
	std::string out;
	for (std::size_t i = 0; i < text.size(); ++i) {
		if (text[i] == '+') {
			out.push_back(' ');
		} else if (text[i] == '%' && i + 2 < text.size()) {
			std::int32_t const hi = hex_value(text[i + 1]);
			std::int32_t const lo = hex_value(text[i + 2]);
			if (hi >= 0 && lo >= 0) {
				out.push_back(static_cast<char>(hi * 16 + lo));
				i += 2;
			} else {
				out.push_back(text[i]);
			}
		} else {
			out.push_back(text[i]);
		}
	}
	return out;
}

std::map<std::string, std::string>
parse_fields(std::string const & text)
{
	std::map<std::string, std::string> fields;
	std::size_t			   begin = 0;
	while (begin <= text.size()) {
		std::size_t const end	= text.find('&', begin);
		std::string const part	= text.substr(begin, end - begin);
		std::size_t const split = part.find('=');
		std::string const key	= decode(part.substr(0, split));
		if (!key.empty()) {
			std::string const value =
			    split == std::string::npos ? std::string{} : decode(part.substr(split + 1));
			fields[key] = value;
		}
		if (end == std::string::npos) break;
		begin = end + 1;
	}
	return fields;
}

std::string
field(std::map<std::string, std::string> const & fields, std::string const & key)
{
	auto const it = fields.find(key);
	return it == fields.end() ? std::string{} : it->second;
}

std::string
number(double value)
{
	std::ostringstream out;
	out << value;
	return out.str();
}

std::string
format_time(std::chrono::system_clock::time_point const & time)
{
	if (time.time_since_epoch().count() == 0) return {};

	std::time_t const value = std::chrono::system_clock::to_time_t(time);
	std::tm		  tm{};
	localtime_r(&value, &tm);
	std::ostringstream out;
	out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return out.str();
}

std::chrono::system_clock::time_point
parse_time(std::string const & text, bool end_of_day)
{
	std::tm		   tm{};
	std::istringstream in(text);
	in >> std::get_time(&tm, "%Y-%m-%d");
	if (in.fail()) return {};
	if (end_of_day) {
		tm.tm_hour = 23;
		tm.tm_min  = 59;
		tm.tm_sec  = 59;
	}
	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string
mime_type(std::filesystem::path const & path)
{
	std::string const ext = path.extension().string();
	if (ext == ".css") return "text/css; charset=utf-8";
	if (ext == ".js") return "application/javascript; charset=utf-8";
	if (ext == ".svg") return "image/svg+xml";
	if (ext == ".png") return "image/png";
	return "text/html; charset=utf-8";
}

bool
send_all(int fd, std::string const & text)
{
	std::size_t sent = 0;
	while (sent < text.size()) {
		ssize_t const count = send(fd, text.data() + sent, text.size() - sent, MSG_NOSIGNAL);
		if (count <= 0) return false;
		sent += static_cast<std::size_t>(count);
	}
	return true;
}

bool
read_request(int fd, request_message & req)
{
	std::string received;
	char	    buf[4096];
	std::size_t header_end = std::string::npos;
	while ((header_end = received.find("\r\n\r\n")) == std::string::npos) {
		ssize_t const count = recv(fd, buf, sizeof(buf), 0);
		if (count <= 0 || received.size() > 65536) return false;
		received.append(buf, static_cast<std::size_t>(count));
	}

	std::istringstream header(received.substr(0, header_end));
	std::string	   line;
	std::getline(header, line);
	std::istringstream first_line(line);
	first_line >> req.method >> req.path;

	std::size_t length = 0;
	while (std::getline(header, line)) {
		std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		std::size_t const split = line.find(':');
		if (split != std::string::npos && line.substr(0, split) == "content-length") {
			try {
				length = static_cast<std::size_t>(std::stoul(line.substr(split + 1)));
			} catch (std::exception const &) {
				return false;
			}
		}
	}

	std::size_t const body_begin = header_end + 4;
	if (received.size() > body_begin) req.body = received.substr(body_begin);
	while (req.body.size() < length) {
		ssize_t const count = recv(fd, buf, sizeof(buf), 0);
		if (count <= 0 || req.body.size() + static_cast<std::size_t>(count) > 1048576) return false;
		req.body.append(buf, static_cast<std::size_t>(count));
	}
	req.body.resize(length);
	return !req.method.empty() && !req.path.empty();
}

std::string
status_text(std::uint16_t status)
{
	switch (status) {
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	default:
		return "Internal Server Error";
	}
}

bool
has_role(std::shared_ptr<user> const & u, std::string const & role)
{ return u != nullptr && u->roles.count(role) != 0; }

} // namespace

web_server::web_server(
    std::string		  web_root,
    marketplace_store &	  store,
    catalog_service &	  catalog,
    content_service &	  content,
    user_service &	  users,
    purchase_controller & purchases,
    exchange_service &	  exchanges,
    moderation_service &  moderation)
    : web_root_(std::move(web_root)), store_(store), catalog_(catalog), content_(content), users_(users),
      purchases_(purchases), exchanges_(exchanges), moderation_(moderation)
{}

int
web_server::run(std::uint16_t port, bool open_browser_window)
{
	stopping_.store(false);
	int const server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		std::cerr << "Could not create the local web server socket.\n";
		return 1;
	}

	int reuse = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	sockaddr_in addr{};
	addr.sin_family	     = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port	     = htons(port);
	if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
		std::cerr << "Could not bind to port " << port << ". Try another port with --port.\n";
		close(server_fd);
		return 1;
	}
	if (listen(server_fd, 16) < 0) {
		std::cerr << "Could not start listening for browser connections.\n";
		close(server_fd);
		return 1;
	}
	server_socket_.store(server_fd);

	std::string const address = "http://127.0.0.1:" + std::to_string(port);
	std::cout << "Forma Market is available at " << address << std::endl;
	if (open_browser_window) {
		char const *	  browser = std::getenv("BROWSER");
		std::string const command = browser == nullptr || std::string(browser).empty() ? "xdg-open" : browser;
		std::system((command + " " + address + " >/dev/null 2>&1 &").c_str());
	}

	while (!stopping_.load()) {
		int const client_fd = accept(server_fd, nullptr, nullptr);
		if (client_fd < 0) {
			if (stopping_.load()) break;
			continue;
		}

		request_message	 req;
		response_message res;
		if (read_request(client_fd, req)) {
			res = route_(req);
		} else {
			res.status = 400;
			res.body   = "{\"error\":\"Некорректный запрос\"}";
		}

		std::ostringstream header;
		header << "HTTP/1.1 " << res.status << ' ' << status_text(res.status)
		       << "\r\nContent-Type: " << res.content_type << "\r\nContent-Length: " << res.body.size()
		       << "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n";
		send_all(client_fd, header.str());
		send_all(client_fd, res.body);
		close(client_fd);
	}

	close(server_fd);
	server_socket_.store(-1);
	return 0;
}

void
web_server::stop()
{
	stopping_.store(true);
	int const fd = server_socket_.load();
	if (fd >= 0) shutdown(fd, SHUT_RDWR);
}

bool
web_server::is_running() const
{ return server_socket_.load() >= 0; }

response_message
web_server::route_(request_message const & req)
{
	if (req.path.rfind("/api/", 0) == 0) return route_api_(req);
	return serve_file_(req.path);
}

response_message
web_server::serve_file_(std::string const & resource) const
{
	std::string path = resource.substr(0, resource.find('?'));
	if (path == "/") path = "/index.html";
	if (path.find("..") != std::string::npos) return { 404, "text/plain; charset=utf-8", "Not found" };

	std::filesystem::path const relative(path.substr(1));
	std::filesystem::path const file_path = std::filesystem::path(web_root_) / relative;
	std::ifstream		    file(file_path, std::ios::binary);
	if (!file) return { 404, "text/plain; charset=utf-8", "Not found" };

	std::ostringstream contents;
	contents << file.rdbuf();
	return { 200, mime_type(file_path), contents.str() };
}

std::string
web_server::json_product_(std::shared_ptr<product> const & p) const
{
	if (p == nullptr) return "null";
	auto const u	     = store_.find_user(active_user_id_);
	bool	   purchased = false;
	for (auto const & rule : store_.rights) {
		if (rule != nullptr && rule->active && rule->recipient == u && rule->product == p) {
			purchased = true;
			break;
		}
	}

	std::ostringstream out;
	out << "{\"id\":\"" << p->id().str() << "\",\"name\":\"" << json_escape(p->name()) << "\",\"description\":\""
	    << json_escape(p->description()) << "\",\"category\":\"" << json_escape(p->category())
	    << "\",\"price\":" << number(p->price()) << ",\"seller_id\":\""
	    << (p->seller() == nullptr ? "" : p->seller()->id.str()) << "\",\"seller_name\":\""
	    << json_escape(p->seller() == nullptr ? "" : p->seller()->name)
	    << "\",\"seller_rating\":" << number(p->seller() == nullptr ? 0.0 : p->seller()->rating)
	    << ",\"content_type\":\"" << json_escape(p->material() == nullptr ? "" : p->material()->type)
	    << "\",\"usage_terms\":\"" << json_escape(p->usage_terms())
	    << "\",\"published\":" << (p->published() ? "true" : "false")
	    << ",\"purchased\":" << (purchased ? "true" : "false")
	    << ",\"owned\":" << (p->seller() == u ? "true" : "false") << ",\"reviews\":[";

	auto const reviews = catalog_.get_reviews(p->id());
	for (std::size_t i = 0; i < reviews.size(); ++i) {
		auto const & r = reviews[i];
		if (i != 0) out << ',';
		out << "{\"rating\":" << r->rating << ",\"text\":\"" << json_escape(r->text) << "\",\"author\":\""
		    << json_escape(r->author == nullptr ? "Покупатель" : r->author->name) << "\",\"date\":\""
		    << json_escape(format_time(r->created_at)) << "\"}";
	}
	out << "]}";
	return out.str();
}

std::string
web_server::json_deal_(std::shared_ptr<deal> const & d) const
{
	if (d == nullptr) return "null";
	std::string  name;
	std::string  second_name;
	std::string  owner_id;
	std::string  proposer;
	auto const & products = d->products();
	if (!products.empty() && products[0] != nullptr) {
		name = products[0]->name();
		if (products[0]->seller() != nullptr) owner_id = products[0]->seller()->id.str();
	}
	if (products.size() > 1 && products[1] != nullptr) {
		second_name = products[1]->name();
		if (products[1]->seller() != nullptr) proposer = products[1]->seller()->name;
	}

	std::ostringstream out;
	out << "{\"id\":\"" << d->id().str() << "\",\"type\":\"" << json_escape(d->type()) << "\",\"status\":\""
	    << json_escape(d->status()) << "\",\"price\":" << number(d->price()) << ",\"created_at\":\""
	    << json_escape(format_time(d->created_at())) << "\",\"product_name\":\"" << json_escape(name)
	    << "\",\"second_product_name\":\"" << json_escape(second_name) << "\",\"product_owner_id\":\"" << owner_id
	    << "\",\"proposer_name\":\"" << json_escape(proposer) << "\"}";
	return out.str();
}

std::string
web_server::json_complaint_(std::shared_ptr<complaint> const & c) const
{
	if (c == nullptr) return "null";
	std::string name = c->object_id.str();
	if (c->object_type == "product") {
		auto const p = store_.find_product(c->object_id);
		if (p != nullptr) name = p->name();
	} else if (c->object_type == "user") {
		auto const u = store_.find_user(c->object_id);
		if (u != nullptr) name = u->name;
	} else if (c->object_type == "deal") {
		auto const d = store_.find_deal(c->object_id);
		if (d != nullptr) name = "Сделка " + d->id().str();
	}

	std::ostringstream out;
	out << "{\"id\":\"" << c->id.str() << "\",\"object_type\":\"" << json_escape(c->object_type)
	    << "\",\"object_id\":\"" << c->object_id.str() << "\",\"object_name\":\"" << json_escape(name)
	    << "\",\"reason\":\"" << json_escape(c->reason) << "\",\"status\":\"" << json_escape(c->status)
	    << "\",\"decision\":\"" << json_escape(c->decision) << "\"}";
	return out.str();
}

std::string
web_server::json_user_(std::shared_ptr<user> const & u) const
{
	if (u == nullptr) return "null";
	std::ostringstream roles;
	bool		   first = true;
	for (std::string const & role : u->roles) {
		if (!first) roles << ", ";
		first = false;
		roles << role;
	}

	std::ostringstream out;
	out << "{\"id\":\"" << u->id.str() << "\",\"name\":\"" << json_escape(u->name) << "\",\"email\":\""
	    << json_escape(u->email) << "\",\"roles\":\"" << json_escape(roles.str())
	    << "\",\"rating\":" << number(u->rating) << ",\"blocked\":" << (u->blocked ? "true" : "false") << "}";
	return out.str();
}

response_message
web_server::route_api_(request_message const & req)
{
	std::size_t const qpos = req.path.find('?');
	std::string const path = req.path.substr(0, qpos);
	auto const	  query =
	    qpos == std::string::npos ? std::map<std::string, std::string>{} : parse_fields(req.path.substr(qpos + 1));
	auto const fields = parse_fields(req.body);
	auto const json	  = [](std::string const & body, std::uint16_t status = 200) {
		return response_message{ status, "application/json; charset=utf-8", body };
	};
	auto const need_user = [this, &json]() {
		if (active_user_id_.empty()) return json("{\"error\":\"Войдите в аккаунт\"}", 401);
		return response_message{};
	};

	if (path == "/api/session" && req.method == "GET") {
		auto const u = users_.get_user(active_user_id_);
		if (u == nullptr) return json("{\"authenticated\":false}");
		return json(
		    "{\"authenticated\":true,\"user\":" + json_user_(u) +
		    ",\"administrator\":" + (has_role(u, "administrator") ? "true}" : "false}"));
	}

	if (path == "/api/login" && req.method == "POST") {
		active_user_id_ = users_.authenticate(field(fields, "email"), field(fields, "password"));
		auto const u	= users_.get_user(active_user_id_);
		if (u == nullptr) return json("{\"error\":\"Не удалось войти. Проверьте данные.\"}", 401);
		return json(
		    "{\"ok\":true,\"user\":" + json_user_(u) +
		    ",\"administrator\":" + (has_role(u, "administrator") ? "true}" : "false}"));
	}

	if (path == "/api/register" && req.method == "POST") {
		active_user_id_ =
		    users_.register_user(field(fields, "email"), field(fields, "name"), field(fields, "password"));
		auto const u = users_.get_user(active_user_id_);
		if (u == nullptr) return json("{\"error\":\"Укажите имя, email и пароль от 6 символов.\"}", 400);
		return json("{\"ok\":true,\"user\":" + json_user_(u) + ",\"administrator\":false}", 201);
	}

	if (path == "/api/logout" && req.method == "POST") {
		active_user_id_ = {};
		return json("{\"ok\":true}");
	}

	if (path == "/api/products" && req.method == "GET") {
		auto		  products = catalog_.search(active_user_id_, field(query, "query"));
		std::string const category = field(query, "category");
		if (!category.empty() && category != "Все категории") {
			products.erase(
			    std::remove_if(
				products.begin(),
				products.end(),
				[&category](auto const & p) { return p->category() != category; }),
			    products.end());
		}
		std::string const sort = field(query, "sort");
		if (sort == "price_ascending") {
			std::sort(products.begin(), products.end(), [](auto const & a, auto const & b) {
				return a->price() < b->price();
			});
		} else if (sort == "rating_descending") {
			std::sort(products.begin(), products.end(), [](auto const & a, auto const & b) {
				double const ar = a->seller() == nullptr ? 0.0 : a->seller()->rating;
				double const br = b->seller() == nullptr ? 0.0 : b->seller()->rating;
				return ar > br;
			});
		}

		std::ostringstream out;
		out << '[';
		for (std::size_t i = 0; i < products.size(); ++i) {
			if (i != 0) out << ',';
			out << json_product_(products[i]);
		}
		out << ']';
		return json(out.str());
	}

	if (path.rfind("/api/products/", 0) == 0 && req.method == "GET") {
		uuid const id = uuid::parse(path.substr(14));
		auto const p  = store_.find_product(id);
		if (p == nullptr) return json("{\"error\":\"Товар не найден\"}", 404);
		return json(json_product_(p));
	}

	if (path == "/api/products" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		double price = 0.0;
		try {
			price = std::stod(field(fields, "price"));
		} catch (std::exception const &) {
			return json("{\"error\":\"Укажите корректную цену\"}", 400);
		}
		std::string const name	      = field(fields, "name");
		std::string const description = field(fields, "description");
		std::string const address     = field(fields, "content_address");
		if (name.empty() || description.empty() || address.empty())
			return json("{\"error\":\"Заполните карточку и добавьте контент\"}", 400);

		auto const p = std::make_shared<product>();
		p->set_id(uuid::generate());
		p->set_name(name);
		p->set_description(description);
		p->set_category(field(fields, "category"));
		p->set_usage_terms(field(fields, "usage_terms"));
		p->change_rules(std::make_shared<fixed_price>(price));
		uuid const id = catalog_.publish(active_user_id_, p);
		if (id.empty()) return json("{\"error\":\"Не удалось опубликовать товар\"}", 400);
		content_.upload(id, field(fields, "content_type"), address);
		return json("{\"ok\":true,\"id\":\"" + id.str() + "\"}", 201);
	}

	if (path == "/api/purchase" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		std::string const id =
		    purchases_.make_purchase(active_user_id_, uuid::parse(field(fields, "product_id")));
		if (id.empty()) return json("{\"error\":\"Покупка не выполнена. Проверьте доступность товара.\"}", 400);
		return json("{\"ok\":true,\"deal_id\":\"" + id + "\"}");
	}

	if (path == "/api/access" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		uuid const	  id	  = uuid::parse(field(fields, "product_id"));
		std::string const address = purchases_.get_purchased_product(active_user_id_, id);
		if (address.empty()) return json("{\"error\":\"Доступ не найден\"}", 403);
		auto const p = store_.find_product(id);
		return json(
		    "{\"ok\":true,\"address\":\"" + json_escape(address) + "\",\"type\":\"" +
		    json_escape(p == nullptr || p->material() == nullptr ? "" : p->material()->type) + "\"}");
	}

	if (path == "/api/exchanges" && req.method == "GET") {
		auto const access = need_user();
		if (access.status != 200) return access;
		auto const	   u = users_.get_user(active_user_id_);
		std::ostringstream out;
		out << '[';
		bool first = true;
		for (auto const & d : store_.deals) {
			auto const & participants = d->participants();
			if (d->type() != "exchange" || (participants.first != u && participants.second != u)) continue;
			if (!first) out << ',';
			first = false;
			out << json_deal_(d);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/exchanges" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		uuid const id = exchanges_.propose(
		    active_user_id_,
		    uuid::parse(field(fields, "desired_product_id")),
		    uuid::parse(field(fields, "offered_product_id")));
		if (id.empty()) return json("{\"error\":\"Не удалось создать предложение обмена\"}", 400);
		return json("{\"ok\":true,\"deal_id\":\"" + id.str() + "\"}", 201);
	}

	if (path == "/api/exchanges/respond" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		uuid const id = uuid::parse(field(fields, "deal_id"));
		if (field(fields, "accepted") == "true")
			exchanges_.accept(id, active_user_id_);
		else
			exchanges_.reject(id);
		return json("{\"ok\":true}");
	}

	if (path == "/api/deals" && req.method == "GET") {
		auto const access = need_user();
		if (access.status != 200) return access;
		auto const	   history = users_.get_history(active_user_id_);
		std::ostringstream out;
		out << '[';
		for (std::size_t i = 0; i < history.size(); ++i) {
			if (i != 0) out << ',';
			out << json_deal_(history[i]);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/reviews" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		std::int32_t rating = 0;
		try {
			rating = static_cast<std::int32_t>(std::stoi(field(fields, "rating")));
		} catch (std::exception const &) {
			return json("{\"error\":\"Выберите оценку\"}", 400);
		}
		std::string const text = field(fields, "text");
		feedback_form	  form(store_, active_user_id_);
		if (form.leave_review(uuid::parse(field(fields, "deal_id")), rating, text).empty())
			return json("{\"error\":\"Отзыв доступен после завершённой сделки\"}", 400);
		return json("{\"ok\":true}", 201);
	}

	if (path == "/api/complaints" && req.method == "GET") {
		auto const access = need_user();
		if (access.status != 200) return access;
		std::ostringstream out;
		out << '[';
		bool first = true;
		for (auto const & c : store_.complaints) {
			if (c->author == nullptr || c->author->id != active_user_id_) continue;
			if (!first) out << ',';
			first = false;
			out << json_complaint_(c);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/complaints" && req.method == "POST") {
		auto const access = need_user();
		if (access.status != 200) return access;
		std::string const type	 = field(fields, "object_type");
		uuid const	  oid	 = uuid::parse(field(fields, "object_id"));
		std::string const reason = field(fields, "reason");
		if (type.empty() || oid.empty() || reason.empty())
			return json("{\"error\":\"Заполните причину обращения\"}", 400);
		feedback_form form(store_, active_user_id_);
		if (form.submit_complaint(type, oid, reason).empty())
			return json("{\"error\":\"Указанный объект не найден\"}", 400);
		return json("{\"ok\":true}", 201);
	}

	if (path == "/api/owned-products" && req.method == "GET") {
		auto const access = need_user();
		if (access.status != 200) return access;
		std::ostringstream out;
		out << '[';
		bool first = true;
		for (auto const & p : store_.products) {
			if (p->seller() == nullptr || p->seller()->id != active_user_id_) continue;
			if (!first) out << ',';
			first = false;
			out << json_product_(p);
		}
		out << ']';
		return json(out.str());
	}

	auto const u = users_.get_user(active_user_id_);
	if (path.rfind("/api/admin/", 0) == 0 && !has_role(u, "administrator"))
		return json("{\"error\":\"Недостаточно прав\"}", 403);

	if (path == "/api/admin/products" && req.method == "GET") {
		std::ostringstream out;
		out << '[';
		bool first = true;
		for (auto const & p : store_.products) {
			if (!p->published()) continue;
			if (!first) out << ',';
			first = false;
			out << json_product_(p);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/admin/complaints" && req.method == "GET") {
		std::ostringstream out;
		out << '[';
		for (std::size_t i = 0; i < store_.complaints.size(); ++i) {
			if (i != 0) out << ',';
			out << json_complaint_(store_.complaints[i]);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/admin/users" && req.method == "GET") {
		std::ostringstream out;
		out << '[';
		for (std::size_t i = 0; i < store_.users.size(); ++i) {
			if (i != 0) out << ',';
			out << json_user_(store_.users[i]);
		}
		out << ']';
		return json(out.str());
	}

	if (path == "/api/admin/report" && req.method == "GET") {
		std::string const report = moderation_.generate_report(
		    parse_time(field(query, "start"), false), parse_time(field(query, "end"), true));
		return json("{\"report\":\"" + json_escape(report) + "\"}");
	}

	if (path == "/api/admin/publication" && req.method == "POST") {
		moderation_.review_publication(uuid::parse(field(fields, "product_id")));
		return json("{\"ok\":true}");
	}

	if (path == "/api/admin/complaint" && req.method == "POST") {
		moderation_.review_complaint(
		    active_user_id_, uuid::parse(field(fields, "complaint_id")), field(fields, "decision"));
		return json("{\"ok\":true}");
	}

	if (path == "/api/admin/block" && req.method == "POST") {
		moderation_.block_user(uuid::parse(field(fields, "user_id")), field(fields, "reason"));
		return json("{\"ok\":true}");
	}

	return json("{\"error\":\"Маршрут не найден\"}", 404);
}

} // namespace marketplace
