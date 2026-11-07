<?php
declare(strict_types=1);
require_once __DIR__ . '/layout.php';
secure_session();
$pdo = db();
$msg = ''; $isErr = false;

if (isset($_GET['logout'])) { $_SESSION = []; session_destroy(); header('Location: index.php'); exit; }

// ---- Вход ----
if (empty($_SESSION['admin_id'])) {
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && csrf_ok()) {
        if (rate_limited('login', (int)cfg('login_rate_limit'), (int)cfg('login_rate_window'))) {
            $msg = 'Слишком много попыток. Подождите 15 минут.'; $isErr = true;
        } else {
            $st = $pdo->prepare('SELECT id,password_hash FROM admins WHERE username=?');
            $st->execute([trim((string)($_POST['username'] ?? ''))]);
            $a = $st->fetch();
            $hash = $a['password_hash'] ?? '$2y$10$abcdefghijklmnopqrstuuABCDEFGHIJKLMNOPQRSTUVWXYZ0123456';
            $ok = password_verify((string)($_POST['password'] ?? ''), $hash);
            if ($a && $ok) {
                session_regenerate_id(true);
                $_SESSION['admin_id'] = (int)$a['id'];
                $_SESSION['csrf'] = bin2hex(random_bytes(32));
                header('Location: index.php'); exit;
            }
            $msg = 'Неверный логин или пароль'; $isErr = true;
        }
    }
    page_start('Вход');
    echo '<h1><b>HANDI</b> — вход</h1><div class="card">';
    if ($msg) echo '<div class="msg e">' . h($msg) . '</div>';
    ?><form method="post" class="row"><input type="hidden" name="csrf" value="<?= h(csrf_token()) ?>">
    <input name="username" placeholder="Логин" required autocomplete="username">
    <input name="password" type="password" placeholder="Пароль" required autocomplete="current-password">
    <button>Войти</button></form></div><?php
    page_end(); exit;
}

// ---- Действия ----
$newKeys = [];
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (!csrf_ok()) { $msg = 'Ошибка CSRF, обновите страницу'; $isErr = true; }
    else {
        $act = (string)($_POST['action'] ?? '');
        $id = (int)($_POST['id'] ?? 0);
        try {
            if ($act === 'create') {
                $d = (string)($_POST['days'] ?? '30');
                $allowed = ['7', '30', '90', '180', 'forever'];
                if (!in_array($d, $allowed, true)) throw new RuntimeException('Неверный срок');
                $count = max(1, min(100, (int)($_POST['count'] ?? 1)));
                $note = mb_substr(trim((string)($_POST['note'] ?? '')), 0, 190);
                $days = $d === 'forever' ? null : (int)$d;
                $ins = $pdo->prepare('INSERT INTO licenses (license_key,duration_days,note,created_at) VALUES (?,?,?,?)');
                for ($i = 0; $i < $count; $i++) {
                    for ($try = 0; $try < 5; $try++) {
                        $k = generate_key();
                        try { $ins->execute([$k, $days, $note, now()]); $newKeys[] = $k; break; }
                        catch (PDOException $e) { if ($e->getCode() !== '23000') throw $e; }
                    }
                }
                $msg = 'Создано ключей: ' . count($newKeys);
            } elseif ($act === 'block' || $act === 'unblock') {
                $pdo->prepare('UPDATE licenses SET status=? WHERE id=?')->execute([$act === 'block' ? 'blocked' : 'active', $id]);
                $msg = $act === 'block' ? 'Ключ заблокирован' : 'Ключ разблокирован';
            } elseif ($act === 'delete') {
                $pdo->prepare('DELETE FROM licenses WHERE id=?')->execute([$id]);
                $msg = 'Ключ удалён';
            } elseif ($act === 'reset_device') {
                $pdo->prepare('UPDATE licenses SET device_id=NULL, device_name=NULL WHERE id=?')->execute([$id]);
                $msg = 'Привязка к устройству сброшена';
            } elseif ($act === 'extend') {
                $add = max(1, min(3650, (int)($_POST['add_days'] ?? 0)));
                $st = $pdo->prepare('SELECT * FROM licenses WHERE id=?'); $st->execute([$id]); $l = $st->fetch();
                if (!$l) throw new RuntimeException('Ключ не найден');
                if ($l['duration_days'] === null) throw new RuntimeException('Лицензия уже бессрочная');
                if ($l['activated_at'] === null) {
                    $pdo->prepare('UPDATE licenses SET duration_days=duration_days+? WHERE id=?')->execute([$add, $id]);
                } else {
                    $base = max(time(), strtotime($l['expires_at'] . ' UTC'));
                    $pdo->prepare('UPDATE licenses SET expires_at=? WHERE id=?')->execute([gmdate('Y-m-d H:i:s', $base + $add * 86400), $id]);
                }
                $msg = "Продлено на $add дн.";
            } elseif ($act === 'make_forever') {
                $pdo->prepare('UPDATE licenses SET duration_days=NULL, expires_at=NULL WHERE id=?')->execute([$id]);
                $msg = 'Лицензия стала бессрочной';
            }
        } catch (Throwable $e) { $msg = $e instanceof RuntimeException ? $e->getMessage() : 'Ошибка базы данных'; $isErr = true; error_log($e->getMessage()); }
    }
}

// ---- Список ----
$q = trim((string)($_GET['q'] ?? ''));
$sql = 'SELECT * FROM licenses';
$par = [];
if ($q !== '') { $sql .= ' WHERE license_key LIKE ? OR device_name LIKE ? OR note LIKE ?'; $like = '%' . addcslashes($q, '%_\\') . '%'; $par = [$like, $like, $like]; }
$sql .= ' ORDER BY id DESC LIMIT 300';
$st = $pdo->prepare($sql); $st->execute($par); $rows = $st->fetchAll();
$stats = $pdo->query("SELECT COUNT(*) t, SUM(status='blocked') b, SUM(device_id IS NOT NULL) a FROM licenses")->fetch();

function state(array $l): array {
    if ($l['status'] === 'blocked') return ['Заблокирован', 'bad'];
    if ($l['device_id'] === null) return ['Не активирован', 'wait'];
    if ($l['expires_at'] !== null && strtotime($l['expires_at'] . ' UTC') <= time()) return ['Истёк', 'bad'];
    return ['Активен', 'ok'];
}
function act_form(int $id, string $a, string $label, string $cls = 'sec', string $confirm = ''): string {
    return '<form method="post" class="inl"' . ($confirm ? ' data-confirm="' . h($confirm) . '"' : '') . '>'
        . '<input type="hidden" name="csrf" value="' . h(csrf_token()) . '"><input type="hidden" name="action" value="' . $a . '">'
        . '<input type="hidden" name="id" value="' . $id . '"><button class="' . $cls . '">' . h($label) . '</button></form> ';
}

page_start('Панель');
?>
<div class="row" style="justify-content:space-between"><h1><b>HANDI</b> — лицензии</h1><a style="color:#9a9aa2" href="?logout=1">Выйти</a></div>
<?php if ($msg) echo '<div class="msg' . ($isErr ? ' e' : '') . '">' . h($msg) . '</div>'; ?>
<?php if ($newKeys): ?><div class="card"><b>Новые ключи (скопируйте):</b><pre style="color:#ff6a00;user-select:all"><?= h(implode("\n", $newKeys)) ?></pre></div><?php endif; ?>
<div class="card row">
  <span>Всего: <b><?= (int)$stats['t'] ?></b></span><span>Активировано: <b><?= (int)$stats['a'] ?></b></span><span>Заблокировано: <b><?= (int)$stats['b'] ?></b></span>
</div>
<div class="card"><form method="post" class="row"><input type="hidden" name="csrf" value="<?= h(csrf_token()) ?>"><input type="hidden" name="action" value="create">
  <select name="days"><option value="7">7 дней</option><option value="30" selected>30 дней</option><option value="90">90 дней</option><option value="180">180 дней</option><option value="forever">Навсегда</option></select>
  <input type="number" name="count" value="1" min="1" max="100" style="width:80px" title="Количество">
  <input name="note" placeholder="Заметка (необязательно)" maxlength="190">
  <button>Создать ключ</button></form></div>
<div class="card"><form method="get" class="row"><input name="q" value="<?= h($q) ?>" placeholder="Поиск: ключ, устройство, заметка"><button class="sec">Найти</button></form></div>
<div class="card tbl"><table><tr><th>Ключ</th><th>Статус</th><th>Срок</th><th>Устройство</th><th>Действия</th></tr>
<?php foreach ($rows as $l): [$sl, $sc] = state($l); $id = (int)$l['id']; ?>
<tr><td><code><?= h($l['license_key']) ?></code><br><span style="color:#9a9aa2"><?= h($l['note']) ?></span></td>
<td><span class="tag <?= $sc ?>"><?= $sl ?></span></td>
<td><?php
 if ($l['duration_days'] === null) echo 'Навсегда';
 elseif ($l['activated_at'] === null) echo h((string)$l['duration_days']) . ' дн. (после активации)';
 else echo 'до ' . h($l['expires_at']) . ' UTC';
?></td>
<td><?= $l['device_id'] ? h($l['device_name'] ?: '—') . '<br><span style="color:#9a9aa2;font-size:11px">' . h(substr($l['device_id'], 0, 16)) . '…<br>акт. ' . h($l['activated_at']) . '<br>проверка ' . h($l['last_check'] ?? '—') . '</span>' : '—' ?></td>
<td><?php
 echo $l['status'] === 'blocked' ? act_form($id, 'unblock', 'Разблокировать') : act_form($id, 'block', 'Блок', 'sec', 'Заблокировать ключ?');
 if ($l['device_id']) echo act_form($id, 'reset_device', 'Сбросить ПК', 'sec', 'Сбросить привязку к устройству?');
 if ($l['duration_days'] !== null) { ?>
 <form method="post" class="inl"><input type="hidden" name="csrf" value="<?= h(csrf_token()) ?>"><input type="hidden" name="action" value="extend"><input type="hidden" name="id" value="<?= $id ?>">
 <input type="number" name="add_days" value="30" min="1" max="3650" style="width:70px"><button class="sec">Продлить</button></form>
 <?php echo act_form($id, 'make_forever', '∞', 'sec', 'Сделать бессрочной?'); }
 echo act_form($id, 'delete', 'Удалить', 'dng', 'Удалить ключ навсегда?');
?></td></tr>
<?php endforeach; if (!$rows) echo '<tr><td colspan="5" style="color:#9a9aa2">Ничего нет</td></tr>'; ?>
</table></div>
<?php page_end();
