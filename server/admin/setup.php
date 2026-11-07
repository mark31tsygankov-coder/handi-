<?php
declare(strict_types=1);
require_once __DIR__ . '/layout.php';
secure_session();
$pdo = db();
$exists = (int)$pdo->query('SELECT COUNT(*) FROM admins')->fetchColumn() > 0;
$msg = ''; $isErr = false;
if (!$exists && $_SERVER['REQUEST_METHOD'] === 'POST' && csrf_ok()) {
    $u = trim((string)($_POST['username'] ?? ''));
    $p = (string)($_POST['password'] ?? '');
    if (!preg_match('/^[A-Za-z0-9_.-]{3,64}$/', $u)) { $msg = 'Логин: 3–64 символа (латиница, цифры, _ . -)'; $isErr = true; }
    elseif (strlen($p) < 12) { $msg = 'Пароль должен быть не короче 12 символов'; $isErr = true; }
    else {
        $pdo->prepare('INSERT INTO admins (username,password_hash,created_at) VALUES (?,?,?)')
            ->execute([$u, password_hash($p, PASSWORD_DEFAULT), now()]);
        $exists = true; $msg = 'Администратор создан. УДАЛИТЕ файл admin/setup.php с сервера!';
    }
}
page_start('Установка');
echo '<h1><b>HANDI</b> — первичная настройка</h1><div class="card">';
if ($msg) echo '<div class="msg' . ($isErr ? ' e' : '') . '">' . h($msg) . '</div>';
if ($exists) {
    echo '<p>Администратор уже создан. Удалите <code>admin/setup.php</code> и перейдите в <a style="color:#ff6a00" href="index.php">панель</a>.</p>';
} else { ?>
<form method="post" class="row"><input type="hidden" name="csrf" value="<?= h(csrf_token()) ?>">
<input name="username" placeholder="Логин" required autocomplete="username">
<input name="password" type="password" placeholder="Пароль (от 12 символов)" required autocomplete="new-password">
<button>Создать</button></form>
<?php }
echo '</div>'; page_end();
