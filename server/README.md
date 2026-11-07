# HANDI — сервер лицензий (PHP 8.1+ / MySQL)

## Установка на H1Cloud
1. В панели хостинга создайте базу MySQL и пользователя, запомните host/имя/логин/пароль.
2. Откройте phpMyAdmin → ваша база → «Импорт» → загрузите `schema.sql`.
3. Скопируйте `config.example.php` в `config.php` и впишите данные БД.
4. Загрузите всё содержимое папки в корень сайта (`public_html`) по FTP/файловому менеджеру.
5. Включите HTTPS (бесплатный сертификат Let's Encrypt в панели) и редирект с http на https.
6. Откройте `https://ВАШ-ДОМЕН/admin/setup.php`, создайте администратора (пароль от 12 символов).
7. **Сразу удалите `admin/setup.php`.**
8. Войдите в `https://ВАШ-ДОМЕН/admin/`.

## API (для приложения)
`POST https://ВАШ-ДОМЕН/api/activate.php` и `/api/check.php`, тело JSON:
```json
{"key":"HANDI-XXXX-XXXX-XXXX-XXXX","device_id":"<hash устройства>","device_name":"PC-NAME"}
```
Ответ успеха: `{"ok":true,"status":"active","expires_at":"2026-12-06T10:00:00+00:00","lifetime":false,"server_time":"..."}`
Ошибки: `{"ok":false,"error":"blocked|expired|not_found|device_mismatch|not_activated|invalid_key|rate_limit","message":"..."}`

- `activate` привязывает ключ к устройству при первом вызове и запускает отсчёт срока.
- `check` только проверяет (клиент вызывает при запуске и периодически).

## Безопасность
PDO с подготовленными запросами, `password_hash`, CSRF-токены, сессии httponly/SameSite=Strict, лимит попыток входа и запросов к API, CSP, запрет доступа к `config.php`/`includes` через `.htaccess`.
Сроки считаются по UTC на сервере. Если H1Cloud работает не на Apache, закройте доступ к `config.php`, `includes/`, `schema.sql` средствами сервера.
