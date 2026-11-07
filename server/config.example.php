<?php
// Скопируйте в config.php и заполните данные из панели H1Cloud.
return [
    'db_host' => 'localhost',
    'db_name' => 'handi',
    'db_user' => 'handi_user',
    'db_pass' => 'CHANGE_ME',
    'api_rate_limit'   => 30,   // запросов к API с одного IP
    'api_rate_window'  => 600,  // за N секунд
    'login_rate_limit' => 5,    // попыток входа в админку
    'login_rate_window'=> 900,
];
