cmake_minimum_required(VERSION 3.10)
project(ftsmod CXX)
set(CMAKE_CXX_STANDARD 17)

# Descargar Dobby automáticamente desde su repositorio oficial de GitHub
include(FetchContent)
FetchContent_Declare(
  dobby
  GIT_REPOSITORY https://github.com/jmpews/Dobby.git
  GIT_TAG master
)
FetchContent_MakeAvailable(dobby)

# Compilar tu mod principal (game.h se incluye por #include dentro de mod.cpp)
add_library(ftsmod SHARED 
    mod.cpp
)

# Incluir las cabeceras de Dobby y la raíz de tu proyecto
target_include_directories(ftsmod PRIVATE 
    .
    ${dobby_SOURCE_DIR}/include
)

# Vincular Dobby, el log de Android y dlopen
target_link_libraries(ftsmod dobby log dl)
