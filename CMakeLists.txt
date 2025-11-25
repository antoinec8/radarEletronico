cmake_minimum_required(VERSION 3.20.0)

# Adiciona o módulo camera_service como EXTRA_MODULE (pasta interna)
set(ZEPHYR_EXTRA_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/camera_service/camera_service")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(radar_eletronico)

# Adiciona diretório de includes
target_include_directories(app PRIVATE src)

# Adiciona todos os arquivos fonte
target_sources(app PRIVATE 
    src/main.c
    src/threads/sensor_thread.c
    src/threads/display_thread.c
    src/threads/camera_thread.c
)
