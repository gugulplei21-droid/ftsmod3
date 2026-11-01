cmake_minimum_required(VERSION 3.10)
project(ftsmod)

# Compilar tu mod.cpp y el código fuente de Dobby juntos en una sola librería compartida (.so)
add_library(ftsmod SHARED 
    mod.cpp
    dobby/dobby.cpp
)

# Indicarle a CMake dónde buscar tus archivos de cabecera (.h) como game.h y los de Dobby
target_include_directories(ftsmod PRIVATE 
    .
    dobby
)
