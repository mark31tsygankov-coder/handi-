<?php
declare(strict_types=1);
require_once __DIR__ . '/../includes/license.php';

function handle_api(bool $activate): void {
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') { http_response_code(405); echo json_encode(err('method', 'Только POST')); return; }
    try {
        if (rate_limited($activate ? 'activate' : 'check', (int)cfg('api_rate_limit'), (int)cfg('api_rate_window'))) {
            http_response_code(429); echo json_encode(err('rate_limit', 'Слишком много запросов')); return;
        }
        $raw = file_get_contents('php://input', false, null, 0, 4096);
        $d = json_decode($raw ?: '', true);
        if (!is_array($d)) { http_response_code(400); echo json_encode(err('bad_request', 'Неверный JSON')); return; }
        $res = process_license((string)($d['key'] ?? ''), (string)($d['device_id'] ?? ''), (string)($d['device_name'] ?? ''), $activate);
        if (!$res['ok'] && $res['error'] === 'server_error') http_response_code(500);
        echo json_encode($res, JSON_UNESCAPED_UNICODE);
    } catch (Throwable $e) {
        error_log('api error: ' . $e->getMessage());
        http_response_code(500);
        echo json_encode(err('server_error', 'Ошибка сервера'));
    }
}
