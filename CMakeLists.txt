cmake_minimum_required(VERSION 3.5)

if(DEFINED ENV{WIFI_SSID})
    add_compile_definitions(WIFI_SSID="$ENV{WIFI_SSID}")
    message(STATUS "WiFi SSID defined in environment variable: $ENV{WIFI_SSID}")
endif()

if(DEFINED ENV{WIFI_PASS})
    add_compile_definitions(WIFI_PASS="$ENV{WIFI_PASS}")
    message(STATUS "WiFi password defined in environment variable")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(poopal)