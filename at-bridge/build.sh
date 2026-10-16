#!/bin/bash
# ============================================================================
#  GsmTools AT Bridge - Build Script
#  بناء ملف AT لأجهزة سامسونج
# ============================================================================
#
#  متطلبات:
#    - Android NDK (r21+ recommended)
#    - تحميل من: https://developer.android.com/ndk/downloads
#
#  الاستخدام:
#    chmod +x build.sh
#    ./build.sh                    # بناء عادي
#    ./build.sh clean              # تنظيف
#    ./build.sh deploy             # بناء + رفع للجهاز
#    ./build.sh deploy-run         # بناء + رفع + تشغيل
#
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
OUTPUT_DIR="$PROJECT_DIR/libs/arm64-v8a"
BINARY_NAME="AT"

# ============ البحث عن NDK ============

find_ndk() {
    # البحث في المتغيرات
    if [ -n "$ANDROID_NDK_HOME" ] && [ -d "$ANDROID_NDK_HOME" ]; then
        echo "$ANDROID_NDK_HOME"
        return 0
    fi
    if [ -n "$ANDROID_NDK_ROOT" ] && [ -d "$ANDROID_NDK_ROOT" ]; then
        echo "$ANDROID_NDK_ROOT"
        return 0
    fi
    if [ -n "$NDK_ROOT" ] && [ -d "$NDK_ROOT" ]; then
        echo "$NDK_ROOT"
        return 0
    fi

    # البحث في المسارات الشائعة
    local COMMON_PATHS=(
        "$HOME/Android/Sdk/ndk"
        "$HOME/Library/Android/sdk/ndk"
        "/usr/local/android-ndk"
        "/opt/android-ndk"
        "$HOME/android-ndk"
    )
    for base in "${COMMON_PATHS[@]}"; do
        if [ -d "$base" ]; then
            # اختيار أحدث نسخة
            local latest=$(ls -d "$base"/*/ 2>/dev/null | sort -V | tail -1)
            if [ -n "$latest" ] && [ -f "$latest/ndk-build" ]; then
                echo "$latest"
                return 0
            fi
            if [ -f "$base/ndk-build" ]; then
                echo "$base"
                return 0
            fi
        fi
    done

    # البحث في PATH
    if command -v ndk-build &>/dev/null; then
        echo "$(dirname $(which ndk-build))"
        return 0
    fi

    return 1
}

NDK_PATH=$(find_ndk) || {
    echo "========================================"
    echo " ERROR: Android NDK not found!"
    echo "========================================"
    echo ""
    echo " Download NDK from:"
    echo "   https://developer.android.com/ndk/downloads"
    echo ""
    echo " Then set environment variable:"
    echo "   export ANDROID_NDK_HOME=/path/to/ndk"
    echo ""
    echo " Or place it in: ~/Android/Sdk/ndk/"
    echo "========================================"
    exit 1
}

NDK_BUILD="$NDK_PATH/ndk-build"
echo "[*] Using NDK: $NDK_PATH"

# ============ الأوامر ============

case "${1:-build}" in
    clean)
        echo "[*] Cleaning..."
        "$NDK_BUILD" NDK_PROJECT_PATH="$PROJECT_DIR" \
                     APP_BUILD_SCRIPT="$PROJECT_DIR/jni/Android.mk" \
                     NDK_APPLICATION_MK="$PROJECT_DIR/jni/Application.mk" \
                     clean
        rm -rf "$PROJECT_DIR/obj" "$PROJECT_DIR/libs"
        echo "[OK] Clean done"
        ;;

    build)
        echo "[*] Building AT Bridge..."
        "$NDK_BUILD" NDK_PROJECT_PATH="$PROJECT_DIR" \
                     APP_BUILD_SCRIPT="$PROJECT_DIR/jni/Android.mk" \
                     NDK_APPLICATION_MK="$PROJECT_DIR/jni/Application.mk" \
                     -j$(nproc 2>/dev/null || echo 4)
        echo ""
        echo "[OK] Build successful!"
        echo "[*] Binary: $OUTPUT_DIR/$BINARY_NAME"
        ls -la "$OUTPUT_DIR/$BINARY_NAME" 2>/dev/null
        file "$OUTPUT_DIR/$BINARY_NAME" 2>/dev/null
        ;;

    deploy)
        echo "[*] Building and deploying..."
        "$0" build
        echo ""
        echo "[*] Pushing to device..."
        adb push "$OUTPUT_DIR/$BINARY_NAME" /data/local/tmp/AT
        adb shell chmod 755 /data/local/tmp/AT
        echo "[OK] Deployed to /data/local/tmp/AT"
        ;;

    deploy-run)
        "$0" deploy
        echo ""
        echo "[*] Running on device..."
        echo "========================================"
        adb shell /data/local/tmp/AT
        ;;

    *)
        echo "Usage: $0 {build|clean|deploy|deploy-run}"
        exit 1
        ;;
esac
