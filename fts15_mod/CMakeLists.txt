cmake_minimum_required(VERSION 3.18)
project(ftsmod CXX)

# ---- Estándar ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Flags comunes ----
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O2 -fvisibility=hidden -Wall -Wextra")

# ---- Dobby (usa su propio CMakeLists) ----
if(NOT TARGET dobby)
    add_subdirectory(dobby_src EXCLUDE_FROM_ALL)
endif()

# ---- Mod principal ----
add_library(ftsmod SHARED
    mod.cpp
)

# Incluir headers de Dobby solo en nuestro target (no global)
target_include_directories(ftsmod PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby_src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby_src/source
)

# Enlazado
target_link_libraries(ftsmod PRIVATE
    dobby
    log
    dl
)

# Stripped en release
set_target_properties(ftsmod PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)
