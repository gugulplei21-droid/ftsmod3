cmake_minimum_required(VERSION 3.10)
project(ftsmod CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(ftsmod SHARED src/mod.cpp)

# Dobby (hooking inline) NO viene incluido. Compílalo para cada ABI y coloca:
#   native/third_party/dobby/include/dobby.h
#   native/third_party/dobby/lib/<ABI>/libdobby.a      (armeabi-v7a, x86)
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES
  IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/third_party/dobby/lib/${ANDROID_ABI}/libdobby.a)

target_include_directories(ftsmod PRIVATE third_party/dobby/include)
target_link_libraries(ftsmod dobby log dl)
