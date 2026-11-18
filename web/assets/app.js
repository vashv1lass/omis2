"use strict";

const app_root = document.getElementById("app");
const overlay_root = document.getElementById("overlay-root");
const toast_root = document.getElementById("toast-root");

const state = {
    user: null,
    administrator: false,
    page: "catalog",
    admin_tab: "products",
    selected_product: "",
    selected_deal: "",
    search_query: "",
    search_category: "Все категории",
    search_sort: "name_ascending",
    exchange_desired: "",
    registering: false,
    confirm_callback: null
};

const categories = ["Программы", "Шаблоны", "Книги", "Аудио", "Видео", "Графика"];

function escape_html(value) {
    return String(value ?? "").replace(/[&<>\"']/g, (character) => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        "\"": "&quot;",
        "'": "&#39;"
    })[character]);
}

function format_money(value) {
    return `${Number(value || 0).toLocaleString("ru-RU")} BYN`;
}

function category_style(category) {
    const styles = {
        "Программы": "art-programs",
        "Шаблоны": "art-templates",
        "Книги": "art-books",
        "Аудио": "art-audio",
        "Видео": "art-video",
        "Графика": "art-graphics"
    };
    return styles[category] || "art-templates";
}

function category_mark(category) {
    const marks = {
        "Программы": "APP",
        "Шаблоны": "UI",
        "Книги": "BOOK",
        "Аудио": "AUDIO",
        "Видео": "VIDEO",
        "Графика": "ART"
    };
    return marks[category] || "DIGITAL";
}

function art_markup(item, large = false) {
    const category = escape_html(item.category || "Шаблоны");
    const class_name = category_style(item.category || "Шаблоны");
    const extra_class = large ? "large-art" : "";
    return `<div class="product-art ${class_name} ${extra_class}">
        <span class="art-label">${escape_html(category_mark(item.category || "Шаблоны"))}</span>
        <span class="art-icon" aria-hidden="true"></span>
    </div>`;
}

async function request_data(resource_address, options = {}) {
    const request_options = { ...options };
    request_options.headers = {
        "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8",
        ...(options.headers || {})
    };
    const response = await fetch(resource_address, request_options);
    let result = {};
    try {
        result = await response.json();
    } catch (error) {
        result = {};
    }
    if (!response.ok) {
        throw new Error(result.error || "Не удалось выполнить запрос");
    }
    return result;
}

function form_body(values) {
    return new URLSearchParams(values).toString();
}

async function submit_data(resource_address, values) {
    return request_data(resource_address, {
        method: "POST",
        body: form_body(values)
    });
}

function notify(message, is_error = false) {
    const toast = document.createElement("div");
    toast.className = `toast${is_error ? " error" : ""}`;
    toast.textContent = message;
    toast_root.append(toast);
    window.setTimeout(() => toast.remove(), 3600);
}

function show_dialog(title, message, confirm_label, callback) {
    state.confirm_callback = callback;
    overlay_root.innerHTML = `<div class="modal-backdrop" data-action="close-dialog">
        <section class="modal-card" role="dialog" aria-modal="true">
            <h2>${escape_html(title)}</h2>
            <p>${escape_html(message)}</p>
            <div class="modal-actions">
                <button class="button button-secondary" data-action="close-dialog">Отмена</button>
                <button class="button" data-action="confirm-dialog">
                    ${escape_html(confirm_label)}
                </button>
            </div>
        </section>
    </div>`;
}

function close_dialog() {
    overlay_root.innerHTML = "";
    state.confirm_callback = null;
}

function show_message_dialog(title, message, button_label = "Понятно") {
    state.confirm_callback = null;
    overlay_root.innerHTML = `<div class="modal-backdrop" data-action="close-dialog">
        <section class="modal-card modal-card-message" role="dialog" aria-modal="true">
            <h2>${escape_html(title)}</h2>
            <p>${escape_html(message)}</p>
            <div class="modal-actions">
                <button class="button button-secondary" data-action="close-dialog">
                    ${escape_html(button_label)}
                </button>
            </div>
        </section>
    </div>`;
}

function header_markup() {
    if (!state.user) {
        return `<header class="site-header">
            <div class="brand">forma / market</div>
            <span>Цифровые товары. Новые возможности.</span>
            <span>Добро пожаловать</span>
        </header>`;
    }
    const role_label = state.administrator ? "администратор" : "покупатель и продавец";
    return `<header class="site-header">
        <div class="brand">forma / market</div>
        <span>Цифровые товары. Новые возможности.</span>
        <div class="header-user">
            <span>${escape_html(state.user.name)} · ${role_label}</span>
            <button class="button button-secondary" data-action="logout">Выйти</button>
        </div>
    </header>`;
}

function navigation_markup() {
    if (state.administrator) {
        return `<nav class="main-nav">
            <button class="nav-button active" data-action="navigate" data-page="admin">
                Администрирование
            </button>
        </nav>`;
    }
    const items = [
        ["catalog", "Каталог"],
        ["publish", "Продать товар"],
        ["exchange", "Обмен"],
        ["deals", "Мои сделки"],
        ["feedback", "Обращения"]
    ];
    return `<nav class="main-nav">${items.map(([page, label]) => `
        <button class="nav-button${state.page === page ? " active" : ""}"
            data-action="navigate" data-page="${page}">${label}</button>`).join("")}</nav>`;
}

function render_shell(content) {
    app_root.innerHTML = `${header_markup()}
        <main class="page-shell">
            ${state.user ? navigation_markup() : ""}
            ${content}
        </main>
        <footer class="site-footer">
            <span>Учебный прототип / Вариант 25</span>
            <span>Данные демонстрационные · изменения не сохраняются</span>
        </footer>`;
}

function render_auth() {
    const form_content = `<h2>Добро пожаловать</h2>
            <p>Войдите или создайте учётную запись</p>
            <form data-form="login">
                <div class="form-field">
                    <label for="login-email">Email</label>
                    <input id="login-email" name="email" type="email" value="user@demo.local"
                        autocomplete="username" required>
                </div>
                <div class="form-field">
                    <label for="login-password">Пароль</label>
                    <input id="login-password" name="password" type="password" value="demo"
                        autocomplete="current-password" required>
                </div>
                <div class="auth-actions">
                    <button class="button button-full">Войти</button>
                    <button class="button button-secondary button-full" type="button"
                        data-action="toggle-register">Создать учётную запись</button>
                </div>
            </form>
            <div class="quick-login">
                <p>Быстрый вход для демонстрации</p>
                <button class="button button-secondary button-full" data-action="quick-login"
                    data-email="user@demo.local">Покупатель и продавец · Алексей</button>
                <button class="button button-secondary button-full" data-action="quick-login"
                    data-email="admin@demo.local">Администратор · Мария</button>
                <p>Пароль для демонстрации: <strong>demo</strong></p>
            </div>`;

    render_shell(`<h1 class="page-heading">Ваши идеи начинаются здесь</h1>
        <p class="page-lead">Цифровые материалы от независимых авторов</p>
        <div class="auth-layout">
            <section class="panel auth-hero">
                <div class="auth-kicker">DIGITAL GOODS / MARKETPLACE</div>
                <div class="hero-art art-templates">
                    <span class="art-label">UI</span>
                    <span class="art-icon" aria-hidden="true"></span>
                </div>
                <div class="auth-copy">
                    <h2>Создавайте. Делитесь. Находите своё.</h2>
                    <p>Шаблоны, иллюстрации, книги и инструменты.<br>
                    Покупайте готовое или обменивайтесь своими работами.</p>
                </div>
            </section>
            <aside class="panel auth-card">${form_content}</aside>
        </div>`);

    if (state.registering)
        render_registration_dialog();
    else
        close_dialog();
}

function render_registration_dialog() {
    state.confirm_callback = null;
    overlay_root.innerHTML = `<div class="modal-backdrop" data-action="close-dialog">
        <section class="modal-card modal-card-auth" role="dialog" aria-modal="true">
            <h2>Присоединяйтесь к Forma Market</h2>
            <p>Один аккаунт для покупок и продажи своих работ</p>
            <form data-form="register">
                <div class="form-field">
                    <label for="register-name">Как к вам обращаться</label>
                    <input id="register-name" name="name" required autocomplete="name">
                </div>
                <div class="form-field">
                    <label for="register-email">Email</label>
                    <input id="register-email" name="email" type="email" required>
                </div>
                <div class="form-field">
                    <label for="register-password">Пароль</label>
                    <input id="register-password" name="password" type="password"
                        minlength="6" required>
                </div>
                <p class="modal-hint">Минимум 6 символов в пароле</p>
                <div class="auth-actions">
                    <button class="button button-full">Создать аккаунт</button>
                    <button class="button button-secondary button-full" type="button"
                        data-action="return-login">Вернуться ко входу</button>
                </div>
            </form>
        </section>
    </div>`;
}

function render_loading() {
    render_shell(`<div class="loading">Загружаем данные…</div>`);
}

function product_card(item) {
    return `<article class="panel product-card" data-action="open-product"
        data-id="${escape_html(item.id)}" tabindex="0" role="button">
        ${art_markup(item)}
        <div class="product-card-meta">
            <span class="category-pill">${escape_html(item.category)}</span>
            <span>★ ${Number(item.seller_rating || 0).toFixed(1)}</span>
        </div>
        <h3>${escape_html(item.name)}</h3>
        <p class="product-seller">${escape_html(item.seller_name)} ·
            ${escape_html(item.content_type)}</p>
        <div class="product-card-bottom">
            <p class="product-price">${format_money(item.price)}</p>
            <span class="button-link">Подробнее →</span>
        </div>
    </article>`;
}

async function render_catalog() {
    const parameters = new URLSearchParams({
        query: state.search_query,
        category: state.search_category,
        sort: state.search_sort
    });
    const products = await request_data(`/api/products?${parameters.toString()}`);
    const category_options = ["Все категории", ...categories].map((category) => `
        <option value="${escape_html(category)}"${state.search_category === category
            ? " selected" : ""}>${escape_html(category)}</option>`).join("");
    render_shell(`<h1 class="page-heading">Материалы для ваших идей</h1>
        <p class="page-lead">Откройте коллекцию цифровых товаров от независимых авторов</p>
        <form class="toolbar" data-form="search">
            <input name="query" value="${escape_html(state.search_query)}"
                placeholder="Поиск по названию, описанию или автору">
            <select name="category">${category_options}</select>
            <select name="sort">
                <option value="name_ascending"${state.search_sort === "name_ascending"
                    ? " selected" : ""}>По названию</option>
                <option value="price_ascending"${state.search_sort === "price_ascending"
                    ? " selected" : ""}>Сначала дешевле</option>
                <option value="rating_descending"${state.search_sort === "rating_descending"
                    ? " selected" : ""}>По рейтингу</option>
            </select>
        </form>
        <p class="result-count">${products.length} товаров · Доступ сразу после завершения сделки</p>
        ${products.length
            ? `<div class="product-grid">${products.map(product_card).join("")}</div>`
            : `<div class="table-empty">Ничего не найдено. Попробуйте изменить запрос.</div>`}`);
}

function render_reviews(reviews) {
    if (!reviews.length) {
        return `<p class="muted-copy">Пока нет отзывов. Поделитесь впечатлением после покупки.</p>`;
    }
    return reviews.map((item) => `<div class="review-item">
        <strong>${"★".repeat(Math.max(1, Math.min(5, item.rating)))}
            ${escape_html(item.author)}</strong>
        <span class="muted-copy"> · ${escape_html(item.date || "Недавно")}</span>
        <p>${escape_html(item.text)}</p>
    </div>`).join("");
}

async function render_product() {
    const item = await request_data(
        `/api/products/${encodeURIComponent(state.selected_product)}`);
    const is_owner = item.owned;
    const action_markup = item.purchased
        ? `<div class="callout">Покупка завершена · доступ открыт</div>
            <button class="button button-full" data-action="get-product"
                data-id="${escape_html(item.id)}">Получить товар</button>`
        : is_owner
            ? `<div class="callout">Это ваш товар. Управляйте публикацией в каталоге.</div>`
            : `<button class="button button-full" data-action="buy-product"
                data-id="${escape_html(item.id)}">
                Купить за ${format_money(item.price)}</button>
            <button class="button button-secondary button-full" data-action="prepare-exchange"
                data-id="${escape_html(item.id)}">Предложить обмен</button>`;
    render_shell(`<a class="back-link" href="#" data-action="back-catalog">← Вернуться в каталог</a>
        <h1 class="page-heading">${escape_html(item.name)}</h1>
        <p class="page-lead">${escape_html(item.category)} / ${escape_html(item.seller_name)}</p>
        <div class="details-layout">
            <section class="panel details-main">
                ${art_markup(item, true)}
                <h2>О товаре</h2>
                <p>${escape_html(item.description)}</p>
                <p class="muted-copy">Формат: ${escape_html(item.content_type)}</p>
                <h2>Отзывы покупателей</h2>
                ${render_reviews(item.reviews || [])}
            </section>
            <aside class="panel purchase-card">
                <span class="category-pill">Цифровой товар</span>
                <h2 class="price-heading">${format_money(item.price)}</h2>
                <div class="seller-line">Автор: ${escape_html(item.seller_name)}</div>
                <div class="rating-line">★ ${Number(item.seller_rating || 0).toFixed(1)}</div>
                <strong>Условия использования</strong>
                <p class="muted-copy">${escape_html(item.usage_terms)}</p>
                ${action_markup}
                <button class="button-link" data-action="report-product"
                    data-id="${escape_html(item.id)}">Подать жалобу</button>
                <div class="details-spacer"></div>
                <p class="muted-copy">Доступ выдаётся после подтверждения сделки.
                    В макете оплата имитируется.</p>
            </aside>
        </div>`);
}

function render_publish() {
    render_shell(`<h1 class="page-heading">Поделитесь своей работой</h1>
        <p class="page-lead">Создайте карточку и добавьте цифровой контент</p>
        <div class="form-layout">
            <form class="panel form-panel" data-form="publish">
                <div class="form-field">
                    <label for="product-name">Название товара</label>
                    <input id="product-name" name="name" required
                        placeholder="Например, набор иконок для мобильного приложения">
                </div>
                <div class="form-field">
                    <label for="product-category">Категория</label>
                    <select id="product-category" name="category" required>
                        ${categories.map((category) => `<option>${escape_html(category)}</option>`)
                            .join("")}
                    </select>
                </div>
                <div class="form-field">
                    <label for="product-description">Описание</label>
                    <textarea id="product-description" name="description" required
                        placeholder="Что входит в набор, кому он подойдёт и в каком формате доступен">
                    </textarea>
                </div>
                <div class="form-field">
                    <label for="product-price">Цена, BYN</label>
                    <input id="product-price" name="price" type="number" min="0" step="0.01"
                        value="15.00" required>
                </div>
                <div class="form-field">
                    <label for="usage-terms">Условия использования</label>
                    <input id="usage-terms" name="usage_terms"
                        value="Для личных проектов, без перепродажи исходников" required>
                </div>
                <div class="form-field">
                    <label for="content-type">Тип контента</label>
                    <select id="content-type" name="content_type">
                        <option>Файл</option><option>Ключ</option><option>Ссылка</option>
                    </select>
                </div>
                <div class="form-field">
                    <label for="content-address">Файл, ключ или ссылка</label>
                    <div class="upload-row">
                        <input id="content-address" name="content_address"
                            placeholder="Выберите файл или введите ключ / ссылку">
                        <label class="button button-secondary" for="content-file">Выбрать файл</label>
                        <input id="content-file" type="file" hidden>
                    </div>
                    <span id="selected-file" class="muted-copy"></span>
                </div>
                <button class="button button-full">Опубликовать товар</button>
            </form>
            <aside class="panel hint-panel">
                <h2 class="panel-title">Хорошая карточка помогает выбрать</h2>
                <div class="product-art art-graphics">
                    <span class="art-label">ART</span><span class="art-icon"></span>
                </div>
                <ul class="hint-list">
                    <li>Дайте понятное название</li>
                    <li>Опишите состав и формат</li>
                    <li>Укажите условия использования</li>
                    <li>Прикрепите файл, ключ или ссылку</li>
                </ul>
            </aside>
        </div>`);
}

function status_label(status) {
    const labels = {
        completed: "Завершена",
        waiting_reply: "Ожидает ответа",
        pending_confirmation: "Ожидает подтверждения",
        confirmed: "Подтверждена",
        cancelled: "Отменена",
        new: "Новое",
        under_review: "На рассмотрении",
        resolved: "Рассмотрено"
    };
    return labels[status] || status;
}

async function render_exchange() {
    const [offers, owned_products, products] = await Promise.all([
        request_data("/api/exchanges"),
        request_data("/api/owned-products"),
        request_data("/api/products")
    ]);
    const offered_options = owned_products.map((item) => `
        <option value="${escape_html(item.id)}">
            ${escape_html(item.name)}</option>`).join("");
    const desired_options = products.map((item) => `
        <option value="${escape_html(item.id)}"${item.id === state.exchange_desired
            ? " selected" : ""}>${escape_html(item.name)}</option>`).join("");
    const offer_rows = offers.map((item) => {
        const can_respond = item.status === "waiting_reply" &&
            item.product_owner_id === state.user.id;
        return `<tr>
            <td>${escape_html(item.proposer_name || "Пользователь")}</td>
            <td>${escape_html(item.second_product_name)}</td>
            <td>${escape_html(item.product_name)}</td>
            <td>${escape_html(status_label(item.status))}</td>
            <td>${can_respond ? `<div class="table-actions">
                <button class="button" data-action="answer-exchange" data-accepted="true"
                    data-id="${escape_html(item.id)}">Принять</button>
                <button class="button button-secondary" data-action="answer-exchange"
                    data-accepted="false" data-id="${escape_html(item.id)}">
                    Отклонить</button></div>` : "—"}</td>
        </tr>`;
    }).join("");
    render_shell(`<h1 class="page-heading">Обмен без лишних шагов</h1>
        <p class="page-lead">Предложите свою работу или рассмотрите входящие предложения</p>
        <section class="panel panel-pad exchange-create">
            <h2 class="panel-title">Новое предложение</h2>
            <form class="exchange-fields" data-form="exchange">
                <div class="form-field">
                    <label for="desired-product">Хочу получить</label>
                    <select id="desired-product" name="desired_product_id" required>
                        ${desired_options || "<option value=''>Нет товаров в каталоге</option>"}
                    </select>
                </div>
                <div class="form-field">
                    <label for="offered-product">Взамен предлагаю</label>
                    <select id="offered-product" name="offered_product_id" required>
                        ${offered_options || "<option value=''>Сначала опубликуйте свой товар</option>"}
                    </select>
                </div>
                <button class="button" ${!offered_options ? "disabled" : ""}>
                    Отправить предложение</button>
            </form>
        </section>
        <section class="exchange-table">
            <h2 class="panel-title">Входящие предложения</h2>
            <div class="table-wrap"><table>
                <thead><tr><th>Пользователь</th><th>Предлагает</th><th>За ваш товар</th>
                    <th>Статус</th><th>Действие</th></tr></thead>
                <tbody>${offer_rows || `<tr><td class="table-empty" colspan="5">
                    Предложений пока нет</td></tr>`}</tbody>
            </table></div>
        </section>
        <div class="callout" style="margin-top: 14px">
            После согласия владельца система проверит оба товара и выдаст доступ участникам.
        </div>`);
}

async function render_deals() {
    const deals = await request_data("/api/deals");
    const rows = deals.map((item) => `<tr data-action="select-deal"
        data-id="${escape_html(item.id)}">
        <td>${escape_html(item.product_name)}</td>
        <td>${item.type === "exchange" ? "Обмен" : "Покупка / продажа"}</td>
        <td>${escape_html(item.created_at || "Сегодня")}</td>
        <td>${item.price ? format_money(item.price) : "—"}</td>
        <td>${escape_html(status_label(item.status))}</td>
    </tr>`).join("");
    render_shell(`<h1 class="page-heading">Мои сделки</h1>
        <p class="page-lead">Покупки, продажи и обмены в одном месте</p>
        <div class="between-row" style="margin: 10px 0">
            <span class="badge">${deals.length} сделки</span>
        </div>
        <div class="table-wrap" style="min-height: 300px"><table>
            <thead><tr><th>Товар</th><th>Операция</th><th>Дата</th><th>Сумма</th>
                <th>Статус</th></tr></thead>
            <tbody>${rows || `<tr><td class="table-empty" colspan="5">Сделок пока нет</td></tr>`}
            </tbody>
        </table></div>
        <div class="table-actions" style="margin-top: 12px">
            <button class="button" data-action="open-selected-deal"
                ${deals.length ? "" : "disabled"}>Открыть выбранную сделку</button>
            <button class="button button-secondary" data-action="navigate" data-page="feedback">
                Отзыв или жалоба</button>
        </div>`);
}

async function render_feedback() {
    const [deals, products, complaints] = await Promise.all([
        request_data("/api/deals"),
        request_data("/api/products"),
        request_data("/api/complaints")
    ]);
    const completed_deals = deals.filter((item) => item.status === "completed");
    const deal_options = completed_deals.map((item) => `
        <option value="${escape_html(item.id)}"${item.id === state.selected_deal
            ? " selected" : ""}>${escape_html(item.product_name)} · ${escape_html(item.id)}
        </option>`).join("");
    const product_options = products.map((item) => `
        <option value="${escape_html(item.id)}">${escape_html(item.name)}</option>`).join("");
    const complaint_note = complaints.length
        ? `<div class="callout" style="margin-top: 12px">Последнее обращение:
            ${escape_html(complaints[complaints.length - 1].id)} ·
            ${escape_html(status_label(complaints[complaints.length - 1].status))}</div>`
        : "";
    render_shell(`<h1 class="page-heading">Отзывы и обращения</h1>
        <p class="page-lead">Поделитесь впечатлениями или сообщите о проблеме</p>
        <div class="feedback-layout">
            <section class="panel feedback-panel">
                <h2 class="panel-title">Оставить отзыв</h2>
                <p class="muted-copy">Завершённая покупка</p>
                <form data-form="review">
                    <div class="form-field">
                        <label for="review-deal">Сделка</label>
                        <select id="review-deal" name="deal_id" required>
                            ${deal_options || "<option value=''>Нет завершённых сделок</option>"}
                        </select>
                    </div>
                    <div class="form-field">
                        <label for="review-rating">Ваша оценка</label>
                        <select id="review-rating" name="rating">
                            <option value="5">5 — отлично</option><option value="4">4 — хорошо</option>
                            <option value="3">3 — нормально</option><option value="2">2 — плохо</option>
                            <option value="1">1 — очень плохо</option>
                        </select>
                    </div>
                    <div class="form-field">
                        <label for="review-text">Впечатления о товаре</label>
                        <textarea id="review-text" name="text" required
                            placeholder="Что понравилось? Что стоит знать другим покупателям?"></textarea>
                    </div>
                    <button class="button button-full" ${completed_deals.length ? "" : "disabled"}>
                        Опубликовать отзыв</button>
                </form>
            </section>
            <section class="panel feedback-panel">
                <h2 class="panel-title">Подать жалобу</h2>
                <p class="muted-copy">Объект жалобы</p>
                <form data-form="complaint">
                    <div class="form-field">
                        <label for="complaint-type">Тип объекта</label>
                        <select id="complaint-type" name="object_type">
                            <option value="product">Товар</option><option value="deal">Сделка</option>
                            <option value="user">Пользователь</option>
                        </select>
                    </div>
                    <div class="form-field">
                        <label for="complaint-object">Товар</label>
                        <select id="complaint-object" name="object_id">
                            ${product_options}
                        </select>
                    </div>
                    <div class="form-field">
                        <label for="complaint-reason">Причина обращения</label>
                        <textarea id="complaint-reason" name="reason" required
                            placeholder="Опишите проблему, чтобы администратор мог её проверить"></textarea>
                    </div>
                    <button class="button button-secondary button-full">Отправить жалобу</button>
                </form>
            </section>
        </div>
        ${complaint_note}`);
}

async function render_admin() {
    const tabs = [
        ["products", "Публикации"],
        ["complaints", "Жалобы"],
        ["users", "Пользователи"],
        ["reports", "Отчёты"]
    ];
    let panel_markup = "";
    if (state.admin_tab === "products") {
        const products = await request_data("/api/admin/products");
        const rows = products.map((item) => `<tr>
            <td>${escape_html(item.name)}</td><td>${escape_html(item.seller_name)}</td>
            <td>${escape_html(item.content_type)}</td><td>Опубликован</td>
            <td><button class="button" data-action="review-publication"
                data-id="${escape_html(item.id)}">Проверить</button></td>
        </tr>`).join("");
        panel_markup = `<section class="panel admin-panel">
            <h2 class="panel-title">Опубликованные материалы</h2>
            <div class="table-wrap"><table><thead><tr><th>Товар</th><th>Автор</th><th>Тип</th>
                <th>Статус</th><th>Действие</th></tr></thead><tbody>
                ${rows || `<tr><td class="table-empty" colspan="5">Нет публикаций</td></tr>`}
            </tbody></table></div>
        </section>`;
    } else if (state.admin_tab === "complaints") {
        const complaints = await request_data("/api/admin/complaints");
        const rows = complaints.map((item) => `<tr>
            <td>${escape_html(item.id)}</td><td>${escape_html(item.object_name)}</td>
            <td>${escape_html(item.reason)}</td><td>${escape_html(status_label(item.status))}</td>
            <td><select data-decision-for="${escape_html(item.id)}">
                <option value="dismiss">Отклонить жалобу</option>
                <option value="hide">Скрыть товар</option>
                <option value="block">Заблокировать пользователя</option>
                <option value="resolve">Отметить решённой</option>
            </select></td>
            <td><button class="button" data-action="resolve-complaint"
                data-id="${escape_html(item.id)}">Применить</button></td>
        </tr>`).join("");
        panel_markup = `<section class="panel admin-panel">
            <h2 class="panel-title">Очередь обращений</h2>
            <div class="table-wrap"><table><thead><tr><th>Обращение</th><th>Объект</th>
                <th>Причина</th><th>Статус</th><th>Решение</th><th></th></tr></thead>
                <tbody>${rows || `<tr><td class="table-empty" colspan="6">Жалоб нет</td></tr>`}
                </tbody></table></div>
        </section>`;
    } else if (state.admin_tab === "users") {
        const users = await request_data("/api/admin/users");
        const rows = users.map((item) => `<tr>
            <td>${escape_html(item.name)}</td><td>${escape_html(item.email)}</td>
            <td>${escape_html(item.roles)}</td><td>${item.blocked ? "Заблокирован" : "Активен"}</td>
            <td>${item.blocked ? "—" : `<button class="button button-secondary"
                data-action="block-user" data-id="${escape_html(item.id)}">
                Заблокировать</button>`}</td>
        </tr>`).join("");
        panel_markup = `<section class="panel admin-panel">
            <h2 class="panel-title">Учётные записи</h2>
            <div class="table-wrap"><table><thead><tr><th>Пользователь</th><th>Email</th>
                <th>Роли</th><th>Статус</th><th>Действие</th></tr></thead>
                <tbody>${rows}</tbody></table></div>
        </section>`;
    } else {
        panel_markup = `<section class="panel admin-panel report-panel">
            <h2 class="panel-title">Отчёт за период</h2>
            <form data-form="report">
                <div class="form-field"><label for="report-start">Начало периода</label>
                    <input id="report-start" name="start" type="date" value="2026-10-01"></div>
                <div class="form-field"><label for="report-end">Конец периода</label>
                    <input id="report-end" name="end" type="date" value="2026-10-31"></div>
                <button class="button button-full">Сформировать отчёт</button>
            </form>
            <div id="report-result" class="callout report-result">
                Сформируйте отчёт, чтобы посмотреть сводку по сделкам и продажам.
            </div>
        </section>`;
    }

    render_shell(`<span class="badge">Администрирование</span>
        <h1 class="page-heading" style="margin-top: 10px">Рабочее место администратора</h1>
        <p class="page-lead">Публикации, обращения пользователей и отчёт о работе платформы</p>
        <nav class="admin-nav admin-tabs">${tabs.map(([tab, label]) => `
            <button class="tab-button${state.admin_tab === tab ? " active" : ""}"
                data-action="admin-tab" data-tab="${tab}">${label}</button>`).join("")}</nav>
        ${panel_markup}`);
}

async function render_page() {
    if (!state.user) {
        render_auth();
        return;
    }
    if (state.administrator) {
        state.page = "admin";
        await render_admin();
        return;
    }

    try {
        if (state.page === "catalog")
            await render_catalog();
        else if (state.page === "product")
            await render_product();
        else if (state.page === "publish")
            render_publish();
        else if (state.page === "exchange")
            await render_exchange();
        else if (state.page === "deals")
            await render_deals();
        else if (state.page === "feedback")
            await render_feedback();
        else
            await render_catalog();
    } catch (error) {
        render_shell(`<div class="callout">${escape_html(error.message)}</div>`);
    }
}

async function login(email, password) {
    let result;
    try {
        result = await submit_data("/api/login", { email, password });
    } catch (error) {
        show_message_dialog("Не удалось войти",
            "Используйте user@demo.local или admin@demo.local, пароль demo.");
        return false;
    }
    state.user = result.user;
    state.administrator = result.administrator;
    state.page = state.administrator ? "admin" : "catalog";
    state.registering = false;
    close_dialog();
    await render_page();
    return true;
}

async function handle_click(event) {
    const target = event.target.closest("[data-action]");
    if (!target)
        return;
    const action = target.dataset.action;

    if (action === "navigate") {
        state.page = target.dataset.page;
        state.selected_product = "";
        await render_page();
    } else if (action === "toggle-register") {
        state.registering = !state.registering;
        render_auth();
    } else if (action === "quick-login") {
        await login(target.dataset.email, "demo");
    } else if (action === "logout") {
        await submit_data("/api/logout", {});
        state.user = null;
        state.administrator = false;
        state.page = "catalog";
        render_auth();
    } else if (action === "open-product") {
        state.selected_product = target.dataset.id;
        state.page = "product";
        await render_page();
    } else if (action === "back-catalog") {
        event.preventDefault();
        state.page = "catalog";
        await render_page();
    } else if (action === "buy-product") {
        const product_id = target.dataset.id;
        show_dialog("Подтвердить покупку?", "Это демонстрация: реальные платежи не выполняются.",
            "Подтвердить", async () => {
                try {
                    await submit_data("/api/purchase", {
                        product_id
                    });
                    close_dialog();
                    await render_page();
                    show_message_dialog("Пример успешной покупки",
                        "Права проверены, покупка подтверждена, доступ открыт. " +
                        "История и тестовые данные не изменяются. " +
                        "В макете оплата не выполняется.");
                } catch (error) {
                    close_dialog();
                    notify(error.message, true);
                }
            });
    } else if (action === "get-product") {
        try {
            const result = await submit_data("/api/access", {
                product_id: target.dataset.id
            });
            show_message_dialog("Получение товара",
                `Файл готов к скачиванию. Формат: ${result.type}. ` +
                "В этом макете передача файла не выполняется.");
        } catch (error) {
            notify(error.message, true);
        }
    } else if (action === "prepare-exchange") {
        state.exchange_desired = target.dataset.id;
        state.page = "exchange";
        await render_page();
    } else if (action === "report-product") {
        state.selected_product = target.dataset.id;
        state.page = "feedback";
        await render_page();
        const object_select = document.getElementById("complaint-object");
        const type_select = document.getElementById("complaint-type");
        if (object_select)
            object_select.value = state.selected_product;
        if (type_select)
            type_select.value = "product";
    } else if (action === "answer-exchange") {
        try {
            await submit_data("/api/exchanges/respond", {
                deal_id: target.dataset.id,
                accepted: target.dataset.accepted
            });
            notify(target.dataset.accepted === "true"
                ? "Обмен завершён, доступ выдан участникам."
                : "Предложение отклонено.");
            await render_page();
        } catch (error) {
            notify(error.message, true);
        }
    } else if (action === "select-deal") {
        document.querySelectorAll("tbody tr[data-action='select-deal']")
            .forEach((row) => row.classList.remove("row-selected"));
        target.classList.add("row-selected");
        state.selected_deal = target.dataset.id;
    } else if (action === "open-selected-deal") {
        if (!state.selected_deal)
            notify("Выберите сделку в таблице.", true);
        else {
            state.page = "feedback";
            await render_page();
            const deal_select = document.getElementById("review-deal");
            if (deal_select)
                deal_select.value = state.selected_deal;
        }
    } else if (action === "admin-tab") {
        state.admin_tab = target.dataset.tab;
        await render_page();
    } else if (action === "review-publication") {
        await submit_data("/api/admin/publication", {
            product_id: target.dataset.id
        });
        notify("Публикация проверена.");
    } else if (action === "resolve-complaint") {
        const decision_select = document.querySelector(
            `[data-decision-for="${CSS.escape(target.dataset.id)}"]`);
        await submit_data("/api/admin/complaint", {
            complaint_id: target.dataset.id,
            decision: decision_select ? decision_select.value : "resolve"
        });
        notify("Решение по обращению применено.");
        await render_page();
    } else if (action === "block-user") {
        const reason = window.prompt("Укажите причину блокировки:", "Нарушение правил платформы");
        if (reason === null)
            return;
        await submit_data("/api/admin/block", {
            user_id: target.dataset.id,
            reason
        });
        notify("Учётная запись заблокирована.");
        await render_page();
        } else if (action === "close-dialog") {
        close_dialog();
    } else if (action === "confirm-dialog") {
        if (state.confirm_callback)
            await state.confirm_callback();
    }
}

async function handle_submit(event) {
    const form = event.target.closest("form[data-form]");
    if (!form)
        return;
    event.preventDefault();
    const form_data = new FormData(form);
    const values = Object.fromEntries(form_data.entries());

    try {
        if (form.dataset.form === "login") {
            await login(values.email, values.password);
        } else if (form.dataset.form === "register") {
            const result = await submit_data("/api/register", values);
            state.user = result.user;
            state.administrator = false;
            state.page = "catalog";
            state.registering = false;
            close_dialog();
            notify("Учётная запись создана.");
            await render_page();
        } else if (form.dataset.form === "search") {
            state.search_query = values.query || "";
            state.search_category = values.category || "Все категории";
            state.search_sort = values.sort || "name_ascending";
            await render_page();
        } else if (form.dataset.form === "publish") {
            const selected_file = document.getElementById("content-file").files[0];
            if (selected_file && !values.content_address)
                values.content_address = `demo://${selected_file.name}`;
            await submit_data("/api/products", values);
            notify("Карточка товара опубликована.");
            state.page = "catalog";
            state.search_query = "";
            await render_page();
        } else if (form.dataset.form === "exchange") {
            await submit_data("/api/exchanges", values);
            notify("Предложение обмена отправлено.");
            state.exchange_desired = "";
            await render_page();
        } else if (form.dataset.form === "review") {
            await submit_data("/api/reviews", values);
            notify("Спасибо, отзыв опубликован.");
            form.reset();
        } else if (form.dataset.form === "complaint") {
            await submit_data("/api/complaints", values);
            notify("Жалоба отправлена администратору.");
            form.reset();
        } else if (form.dataset.form === "report") {
            const parameters = new URLSearchParams(values);
            const result = await request_data(`/api/admin/report?${parameters.toString()}`);
            const output = document.getElementById("report-result");
            if (output)
                output.textContent = result.report;
        }
    } catch (error) {
        notify(error.message, true);
    }
}

app_root.addEventListener("click", (event) => {
    handle_click(event).catch((error) => notify(error.message, true));
});

app_root.addEventListener("submit", (event) => {
    handle_submit(event).catch((error) => notify(error.message, true));
});

overlay_root.addEventListener("submit", (event) => {
    handle_submit(event).catch((error) => notify(error.message, true));
});

app_root.addEventListener("change", (event) => {
    if (event.target.id === "content-file") {
        const file_label = document.getElementById("selected-file");
        if (file_label)
            file_label.textContent = event.target.files[0]
                ? `Выбран файл: ${event.target.files[0].name}` : "";
    }
});

overlay_root.addEventListener("click", (event) => {
    const action_target = event.target.closest("[data-action]");
    if (!action_target)
        return;
    if (action_target.dataset.action === "close-dialog") {
        if (action_target.classList.contains("modal-backdrop") &&
            event.target !== action_target)
            return;
        const account_dialog_open = overlay_root.querySelector(".modal-card-auth") !== null;
        if (account_dialog_open) {
            state.registering = false;
            render_auth();
        } else {
            close_dialog();
            if (state.registering)
                render_registration_dialog();
        }
    } else if (action_target.dataset.action === "return-login") {
        state.registering = false;
        close_dialog();
        render_auth();
    } else if (action_target.dataset.action === "confirm-dialog" &&
               state.confirm_callback) {
        Promise.resolve(state.confirm_callback())
            .catch((error) => notify(error.message, true));
    }
});

async function start_application() {
    try {
        const result = await request_data("/api/session");
        if (result.authenticated) {
            state.user = result.user;
            state.administrator = result.administrator;
            state.page = state.administrator ? "admin" : "catalog";
        }
    } catch (error) {
        notify(error.message, true);
    }
    await render_page();
}

start_application();
