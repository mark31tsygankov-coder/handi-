@echo off
chcp 65001 > nul
setlocal
cd /d "%~dp0"

if exist build_config.bat call build_config.bat
if "%QT_DIR%"=="" (echo [ОШИБКА] Не задан QT_DIR. Скопируйте build_config.example.bat в build_config.bat и заполните. & exit /b 1)
if "%HANDI_LICENSE_API_URL%"=="" (echo [ОШИБКА] Не задан HANDI_LICENSE_API_URL. & exit /b 1)
echo %HANDI_LICENSE_API_URL% | findstr /b /i "https://" > nul || (echo [ОШИБКА] Адрес API должен начинаться с https:// & exit /b 1)
where cmake > nul 2>&1 || (echo [ОШИБКА] CMake не найден в PATH. Запустите из "x64 Native Tools Command Prompt for VS 2022". & exit /b 1)

echo === Конфигурация ===
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_DIR%" -DHANDI_LICENSE_API_URL=%HANDI_LICENSE_API_URL%
if errorlevel 1 exit /b 1

echo === Сборка ===
cmake --build build --config Release
if errorlevel 1 exit /b 1

echo === Самопроверка ядра ===
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations build\Release\handi_selftest.exe > nul
build\Release\handi_selftest.exe
if errorlevel 1 (echo [ОШИБКА] Самопроверка не пройдена & exit /b 1)

echo === Подготовка папки dist ===
if exist dist rmdir /s /q dist
mkdir dist\HANDI
copy /y build\Release\HANDI.exe dist\HANDI\ > nul
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler dist\HANDI\HANDI.exe
if errorlevel 1 exit /b 1
copy /y ..\README.md dist\HANDI\README.md > nul

echo === Создание ZIP ===
powershell -NoProfile -Command "Compress-Archive -Path 'dist\HANDI' -DestinationPath 'dist\HANDI-1.0.0-win64.zip' -Force"
if errorlevel 1 exit /b 1

echo.
echo Готово: %CD%\dist\HANDI-1.0.0-win64.zip
endlocal
