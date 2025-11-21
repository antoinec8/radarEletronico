# CMakeLists.txt para testes unitários

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(radar_tests)

# Inclui diretório de headers
target_include_directories(app PRIVATE ../src)

# Adiciona arquivos de teste
target_sources(app PRIVATE 
    test_calculations.c
    test_plate_validator.c
)
