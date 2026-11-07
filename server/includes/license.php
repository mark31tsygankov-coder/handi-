<?php
declare(strict_types=1);
require_once __DIR__ . '/bootstrap.php';

function err(string $code, string $msg): array { return ['ok' => false, 'error' => $code, 'message' => $msg]; }

/** Ответ для клиента. $activate=true — привязать ключ к устройству при первом вызове. */
function process_license(string $key, string $deviceId, string $deviceName, bool $activate): array {
    $pdo = db();
    $key = strtoupper(trim($key));
    if (!preg_match('/^HANDI-[A-Z0-9]{4}(-[A-Z0-9]{4}){3}$/', $key)) return err('invalid_key', 'Неверный формат ключа');
    if (!preg_match('/^[A-Za-z0-9\-]{16,128}$/', $deviceId)) return err('invalid_device', 'Неверный идентификатор устройства');
    $deviceName = mb_substr(trim($deviceName), 0, 100);

    $pdo->beginTransaction();
    try {
        $st = $pdo->prepare('SELECT * FROM licenses WHERE license_key=? FOR UPDATE');
        $st->execute([$key]);
        $l = $st->fetch();
        if (!$l) { $pdo->commit(); return err('not_found', 'Ключ не найден'); }
        if ($l['status'] === 'blocked') { $pdo->commit(); return err('blocked', 'Ключ заблокирован'); }

        if ($l['device_id'] === null) {
            if (!$activate) { $pdo->commit(); return err('not_activated', 'Ключ не активирован'); }
            $exp = $l['duration_days'] === null ? null : gmdate('Y-m-d H:i:s', time() + (int)$l['duration_days'] * 86400);
            $pdo->prepare('UPDATE licenses SET device_id=?, device_name=?, activated_at=?, expires_at=? WHERE id=?')
                ->execute([$deviceId, $deviceName, now(), $exp, $l['id']]);
            $l['expires_at'] = $exp;
        } elseif (!hash_equals((string)$l['device_id'], $deviceId)) {
            $pdo->commit(); return err('device_mismatch', 'Ключ привязан к другому устройству');
        }

        if ($l['expires_at'] !== null && strtotime($l['expires_at'] . ' UTC') <= time()) {
            $pdo->commit(); return err('expired', 'Срок действия лицензии истёк');
        }
        $pdo->prepare('UPDATE licenses SET last_check=?, last_ip=? WHERE id=?')->execute([now(), client_ip(), $l['id']]);
        $pdo->commit();
        return [
            'ok' => true,
            'status' => 'active',
            'expires_at' => $l['expires_at'] === null ? null : gmdate('c', strtotime($l['expires_at'] . ' UTC')),
            'lifetime' => $l['expires_at'] === null,
            'server_time' => gmdate('c'),
        ];
    } catch (Throwable $e) {
        if ($pdo->inTransaction()) $pdo->rollBack();
        error_log('license error: ' . $e->getMessage());
        return err('server_error', 'Ошибка сервера');
    }
}
