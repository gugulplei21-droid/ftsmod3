#!/usr/bin/env bash
# Esqueleto de build — NO PROBADO. Necesitas: apktool, JDK, Android SDK (android.jar, d8,
# zipalign, apksigner), Android NDK, baksmali, y Dobby compilado (ver native/CMakeLists.txt).
set -euo pipefail

APK_IN="${1:?uso: ./build.sh original.apk}"
ANDROID_JAR="${ANDROID_JAR:?ruta a android.jar (platforms/android-XX)}"
BUILD_TOOLS="${BUILD_TOOLS:?ruta a build-tools/XX.Y.Z}"
NDK="${NDK:?ruta al NDK}"
BAKSMALI="${BAKSMALI:?ruta a baksmali.jar}"
KEYSTORE="${KEYSTORE:-debug.keystore}"       # keytool -genkey -v -keystore debug.keystore -alias k -keyalg RSA -validity 10000

rm -rf work build && mkdir -p build/classes build/dex

echo "[1/6] Decodificar APK"
apktool d -f "$APK_IN" -o work

echo "[2/6] Compilar clases Java del mod -> dex -> smali (en smali_classes2)"
javac --release 8 -cp "$ANDROID_JAR" -d build/classes java/com/ftsmod/*.java
"$BUILD_TOOLS/d8" --release --min-api 21 --lib "$ANDROID_JAR" \
    --output build/dex $(find build/classes -name '*.class')
java -jar "$BAKSMALI" d build/dex/classes.dex -o work/smali_classes2

echo "[3/6] Compilar libftsmod.so (solo ABIs que trae el juego)"
for ABI in armeabi-v7a x86; do
  cmake -S native -B build/native-$ABI \
        -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=$ABI -DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static \
        -DCMAKE_BUILD_TYPE=Release
  cmake --build build/native-$ABI
  cp build/native-$ABI/libftsmod.so work/lib/$ABI/
done

echo "[4/6] PARCHE MANUAL pendiente: MainActivity.smali (ver README, sección 'Parche smali')."
echo "      Edita work/smali*/com/firsttouchgames/fts15/MainActivity.smali y pulsa Enter."
read -r _

echo "[5/6] Reempaquetar"
apktool b work -o build/mod_unsigned.apk

echo "[6/6] Alinear y firmar"
"$BUILD_TOOLS/zipalign" -f 4 build/mod_unsigned.apk build/mod_aligned.apk
"$BUILD_TOOLS/apksigner" sign --ks "$KEYSTORE" --out build/fts15_mod.apk build/mod_aligned.apk
echo "Listo: build/fts15_mod.apk  (desinstala el original antes: la firma es distinta)"
