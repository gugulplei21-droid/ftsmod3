cmake_minimum_required(VERSION 3.10)
project(ftsmod CXX)
set(CMAKE_CXX_STANDARD 17)

# Compilar tu mod principal
add_library(ftsmod SHARED 
    mod.cpp
)

# Indicarle a CMake dónde encontrar la librería estática de Dobby para Android (armeabi-v7a)
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES
  IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/dobby/lib/${ANDROID_ABI}/libdobby.a
)

# Incluir las cabeceras (.h) y vincular Dobby, logs y dl
target_include_directories(ftsmod PRIVATE 
    .
    dobby/include
)

target_link_libraries(ftsmod dobby log dl)
