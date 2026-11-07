cmake_minimum_required(VERSION 3.21)
project(HANDI VERSION 1.0.0 LANGUAGES CXX)
enable_language(RC)

if(NOT WIN32)
  message(FATAL_ERROR "HANDI собирается только под Windows 10/11")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(MSVC)
  add_compile_options(/utf-8 /W3)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Sql Widgets)

set(HANDI_LICENSE_API_URL "https://your-domain.example/api" CACHE STRING
    "HTTPS-адрес папки api вашего сервера лицензий (без / на конце)")

add_library(handi_core STATIC
  src/macro/Macro.cpp
  src/macro/KeyNames.cpp
  src/hotkeys/Hotkey.cpp
  src/hotkeys/HotkeyManager.cpp
  src/engine/InputSender.cpp
  src/engine/MacroEngine.cpp
  src/settings/Database.cpp
  src/settings/AppSettings.cpp
  src/profiles/ProfileManager.cpp
  src/licensing/LicenseManager.cpp
)
target_include_directories(handi_core PUBLIC src)
target_compile_definitions(handi_core PUBLIC NOMINMAX
  HANDI_LICENSE_API_URL="${HANDI_LICENSE_API_URL}")
target_link_libraries(handi_core PUBLIC Qt6::Core Qt6::Network Qt6::Sql user32 crypt32)

add_executable(handi_selftest tools/selftest.cpp)
target_link_libraries(handi_selftest PRIVATE handi_core)

add_executable(handi WIN32
  src/main.cpp
  src/ui/Theme.cpp
  src/ui/HotkeyEdit.cpp
  src/ui/ActivationDialog.cpp
  src/ui/MacroEditorDialog.cpp
  src/ui/MainWindow.cpp
  resources/resources.qrc
  resources/app.rc
)
target_link_libraries(handi PRIVATE handi_core Qt6::Widgets)
set_target_properties(handi PROPERTIES OUTPUT_NAME "HANDI")
if(MSVC)
  # точка входа main() для GUI-приложения без консоли
  target_link_options(handi PRIVATE /ENTRY:mainCRTStartup)
endif()
