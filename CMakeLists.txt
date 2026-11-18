cmake_minimum_required(VERSION 3.15)
project(digital_goods_marketplace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(COPY web DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(digital_goods_marketplace
    src/application.cpp
    src/account_moderation.cpp
    src/domain.cpp
    src/market_interface.cpp
    src/main.cpp
    src/platform_factory.cpp
    src/services.cpp
    src/store.cpp
    src/strategies.cpp
    src/web_server.cpp
)

target_include_directories(digital_goods_marketplace PRIVATE include)
target_compile_definitions(digital_goods_marketplace PRIVATE
    MARKETPLACE_WEB_ROOT="${CMAKE_CURRENT_BINARY_DIR}/web"
)
target_compile_options(digital_goods_marketplace PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_custom_command(TARGET digital_goods_marketplace POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/web ${CMAKE_CURRENT_BINARY_DIR}/web
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(WEBKIT_41 QUIET IMPORTED_TARGET webkit2gtk-4.1)
  pkg_check_modules(WEBKIT_40 QUIET IMPORTED_TARGET webkit2gtk-4.0)
  if(WEBKIT_41_FOUND)
    target_link_libraries(digital_goods_marketplace
                          PRIVATE PkgConfig::WEBKIT_41)
    target_compile_definitions(digital_goods_marketplace
                               PRIVATE MARKETPLACE_HAS_WEBKIT)
  elseif(WEBKIT_40_FOUND)
    target_link_libraries(digital_goods_marketplace
                          PRIVATE PkgConfig::WEBKIT_40)
    target_compile_definitions(digital_goods_marketplace
                               PRIVATE MARKETPLACE_HAS_WEBKIT)
  endif()
endif()
