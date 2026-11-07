<?php
declare(strict_types=1);
date_default_timezone_set('UTC');

function cfg(string $k) {
    static $c;
    if ($c === null) {
        $f = __DIR__ . '/../config.php';
        if (!is_file($f)) { http_response_code(500); exit('config.php не найден'); }
        $c = require $f;
    }
    return $c[$k] ?? null;
}

function db(): PDO {
    static $pdo;
    if ($pdo === null) {
        $dsn = 'mysql:host=' . cfg('db_host') . ';dbname=' . cfg('db_name') . ';charset=utf8mb4';
        $pdo = new PDO($dsn, cfg('db_user'), cfg('db_pass'), [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
        ]);
    }
    return $pdo;
}

function now(): string { return gmdate('Y-m-d H:i:s'); }

function client_ip(): string {
    return substr((string)($_SERVER['REMOTE_ADDR'] ?? '0.0.0.0'), 0, 45);
}

/** true = лимит превышен. Каждый вызов записывает попытку. */
function rate_limited(string $bucket, int $max, int $window): bool {
    $ip = client_ip();
    $pdo = db();
    $pdo->prepare('DELETE FROM rate_limit WHERE created_at < ?')
        ->execute([gmdate('Y-m-d H:i:s', time() - 86400)]);
    $st = $pdo->prepare('SELECT COUNT(*) FROM rate_limit WHERE ip=? AND bucket=? AND created_at >= ?');
    $st->execute([$ip, $bucket, gmdate('Y-m-d H:i:s', time() - $window)]);
    if ((int)$st->fetchColumn() >= $max) return true;
    $pdo->prepare('INSERT INTO rate_limit (ip,bucket,created_at) VALUES (?,?,?)')
        ->execute([$ip, $bucket, now()]);
    return false;
}

function generate_key(): string {
    $alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
    $parts = [];
    for ($i = 0; $i < 4; $i++) {
        $p = '';
        for ($j = 0; $j < 4; $j++) $p .= $alphabet[random_int(0, strlen($alphabet) - 1)];
        $parts[] = $p;
    }
    return 'HANDI-' . implode('-', $parts);
}

function h(?string $s): string { return htmlspecialchars((string)$s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); }
