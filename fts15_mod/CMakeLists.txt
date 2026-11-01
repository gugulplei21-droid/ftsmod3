cmake_minimum_required(VERSION 3.10)
project(ftsmod)

# Compilar tu mod.cpp junto con el código fuente de Dobby
add_library(ftsmod SHARED 
    mod.cpp
    dobby/dobby.cpp   # (Asegúrate de que este archivo sí exista dentro de tu carpeta dobby)
)

# Indicarle a CMake dónde buscar tus headers (.h) como game.h y los de Dobby
target_include_directories(ftsmod PRIVATE 
    .
    dobby
)
