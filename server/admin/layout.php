<?php
declare(strict_types=1);
require_once __DIR__ . '/../includes/bootstrap.php';

function secure_session(): void {
    $https = !empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off';
    session_name('handi_admin');
    session_set_cookie_params(['lifetime' => 0, 'path' => '/', 'secure' => $https, 'httponly' => true, 'samesite' => 'Strict']);
    session_start();
    header('X-Frame-Options: DENY');
    header('Cache-Control: no-store');
    header("Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'unsafe-inline'; img-src 'self' data:; form-action 'self'");
}

function csrf_token(): string {
    if (empty($_SESSION['csrf'])) $_SESSION['csrf'] = bin2hex(random_bytes(32));
    return $_SESSION['csrf'];
}
function csrf_ok(): bool {
    return isset($_POST['csrf'], $_SESSION['csrf']) && hash_equals($_SESSION['csrf'], (string)$_POST['csrf']);
}

function page_start(string $title): void { ?>
<!doctype html><html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="robots" content="noindex,nofollow">
<title><?= h($title) ?> — HANDI</title>
<style>
:root{--bg:#0a0a0a;--card:#1a1a1c;--line:#2a2a2e;--tx:#f4f4f5;--mut:#9a9aa2;--or:#ff6a00;--red:#e5484d;--grn:#30a46c}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--tx);font:15px/1.5 system-ui,Segoe UI,sans-serif;padding:16px}
.wrap{max-width:1100px;margin:0 auto}h1{font-size:22px}h1 b{color:var(--or)}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:16px}
input,select{background:#0f0f10;color:var(--tx);border:1px solid var(--line);border-radius:10px;padding:9px 12px;font:inherit}
input:focus,select:focus{outline:none;border-color:var(--or)}
button{background:var(--or);color:#000;border:0;border-radius:10px;padding:9px 14px;font:inherit;font-weight:600;cursor:pointer}
button.sec{background:#2a2a2e;color:var(--tx)}button.dng{background:var(--red);color:#fff}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
table{width:100%;border-collapse:collapse;font-size:13px}th,td{text-align:left;padding:8px;border-bottom:1px solid var(--line);vertical-align:top}
th{color:var(--mut);font-weight:500}.tbl{overflow-x:auto}
.tag{padding:2px 8px;border-radius:99px;font-size:12px}.ok{background:#12352a;color:var(--grn)}.bad{background:#3a1618;color:var(--red)}.wait{background:#2a2a2e;color:var(--mut)}
code{background:#0f0f10;padding:2px 6px;border-radius:6px;color:var(--or)}.msg{padding:10px 14px;border-radius:10px;margin-bottom:12px;background:#1f2a1f}.msg.e{background:#3a1618}
form.inl{display:inline}
</style><script src="app.js" defer></script></head><body><div class="wrap"><?php }
function page_end(): void { echo '</div></body></html>'; }
