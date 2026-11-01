cmake_minimum_required(VERSION 3.10)
project(ftsmod)

# Configurar C++ moderno
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilar únicamente tu mod.cpp (game.h se incluye solo con #include desde el código)
add_library(ftsmod SHARED 
    mod.cpp
)

target_include_directories(ftsmod PRIVATE 
    .
)
