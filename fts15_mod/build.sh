#!/usr/bin/env bash
set -e

echo "=========================================="
echo " Starting FTS15 Mod Compilation with Dobby "
echo "=========================================="

# 1. Crear y entrar a la carpeta de compilación
mkdir -p build
cd build

# 2. Configurar CMake con las herramientas del NDK
echo "[+] Configuring CMake project..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release

# 3. Compilar usando todos los núcleos del sistema
echo "[+] Building binary..."
make -j$(nproc)

echo "=========================================="
echo " Compilation completed successfully! "
echo "=========================================="
