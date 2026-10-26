#!/bin/bash
# ============================================================================
#  GsmTools AT Bridge — build script (v4.0)
# ============================================================================
#  Produces:
#    payloads/bin/arm64-v8a/AT       static, Android 5.0+ (API 21), 64-bit
#    payloads/bin/armeabi-v7a/AT_32  static, Android 5.0+ (API 21), 32-bit
#    payloads/bin/host-linux/AT      host build for protocol testing (gcc)
#    payloads/AT, payloads/AT_32     copies of the primary phone binaries
#
#  Usage:
#    ./build.sh                build all ABIs
#    ./build.sh host           host test build only
#    ./build.sh clean          remove build outputs
#    ./build.sh deploy         build + push arm64 to device (/data/local/tmp/AT)
#
#  NDK discovery order: $ANDROID_NDK_HOME > $ANDROID_NDK_ROOT > $NDK_ROOT
#                        > ~/Android/Sdk/ndk/<latest> > ndk-build on PATH
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
JNI_DIR="$SCRIPT_DIR/jni"
OUT_ROOT="$REPO_DIR/payloads/bin"

ANDROID_API_64=21   # widest device support (Android 5.0+)
ANDROID_API_32=21

log()  { printf '[*] %s\n' "$1"; }
ok()   { printf '[OK] %s\n' "$1"; }
die()  { printf '[ERROR] %s\n' "$1" >&2; exit 1; }

find_ndk() {
    for v in ANDROID_NDK_HOME ANDROID_NDK_ROOT NDK_ROOT; do
        [ -n "${!v:-}" ] && [ -d "${!v}" ] && { echo "${!v}"; return 0; }
    done
    for base in "$HOME/Android/Sdk/ndk" "$HOME/Library/Android/sdk/ndk" \
                /opt/android-ndk /usr/local/android-ndk; do
        if [ -d "$base" ]; then
            latest="$(ls -d "$base"/*/ 2>/dev/null | sort -V | tail -1 || true)"
            [ -n "$latest" ] && [ -x "$latest/ndk-build" ] && { echo "${latest%/}"; return 0; }
            [ -x "$base/ndk-build" ] && { echo "$base"; return 0; }
        fi
    done
    command -v ndk-build >/dev/null 2>&1 && { dirname "$(command -v ndk-build)"; return 0; }
    return 1
}

build_host() {
    command -v gcc >/dev/null 2>&1 || die "gcc not found (needed for host build)"
    mkdir -p "$OUT_ROOT/host-linux"
    log "Building host-linux test binary (gcc)..."
    gcc -Wall -Wextra -O2 -o "$OUT_ROOT/host-linux/AT" "$JNI_DIR/at_bridge.c"
    ok "host-linux: $OUT_ROOT/host-linux/AT"
}

build_android() {
    NDK_PATH="$(find_ndk)" || {
        cat >&2 <<'EOF'
[ERROR] Android NDK not found.
  Install from https://developer.android.com/ndk/downloads then:
    export ANDROID_NDK_HOME=/path/to/ndk
  (or run './build.sh host' for the host test build only)
EOF
        exit 1
    }
    log "Using NDK: $NDK_PATH"
    TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/linux-x86_64/bin"
    [ -d "$TOOLCHAIN" ] || TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/darwin-x86_64/bin"
    [ -d "$TOOLCHAIN" ] || TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/windows-x86_64/bin"
    [ -d "$TOOLCHAIN" ] || die "unsupported host OS for NDK toolchain"

    mkdir -p "$OUT_ROOT/arm64-v8a" "$OUT_ROOT/armeabi-v7a"

    log "Building arm64-v8a (static, API $ANDROID_API_64)..."
    "$TOOLCHAIN/aarch64-linux-android${ANDROID_API_64}-clang" \
        -static -O2 -s -Wall -Wextra -fvisibility=hidden \
        "$JNI_DIR/at_bridge.c" -o "$OUT_ROOT/arm64-v8a/AT" -ldl

    log "Building armeabi-v7a (static, API $ANDROID_API_32)..."
    "$TOOLCHAIN/armv7a-linux-androideabi${ANDROID_API_32}-clang" \
        -static -O2 -s -Wall -Wextra -fvisibility=hidden \
        "$JNI_DIR/at_bridge.c" -o "$OUT_ROOT/armeabi-v7a/AT_32" -ldl

    # primary phone binaries at payloads root (what gets pushed by workflows)
    cp "$OUT_ROOT/arm64-v8a/AT"      "$REPO_DIR/payloads/AT"
    cp "$OUT_ROOT/armeabi-v7a/AT_32" "$REPO_DIR/payloads/AT_32"
    ok "arm64-v8a: $(stat -c%s "$OUT_ROOT/arm64-v8a/AT") bytes"
    ok "armeabi-v7a: $(stat -c%s "$OUT_ROOT/armeabi-v7a/AT_32") bytes"
}

case "${1:-build}" in
    clean)
        rm -rf "$OUT_ROOT" "$REPO_DIR/payloads/AT" "$REPO_DIR/payloads/AT_32"
        ok "cleaned" ;;
    host)
        build_host ;;
    build)
        build_android
        build_host || log "host build skipped (gcc missing)" ;;
    deploy)
        "$0" build
        adb push "$REPO_DIR/payloads/AT" /data/local/tmp/AT
        adb shell chmod 755 /data/local/tmp/AT
        ok "deployed to /data/local/tmp/AT" ;;
    deploy-run)
        "$0" build
        adb push "$REPO_DIR/payloads/AT" /data/local/tmp/AT
        adb shell chmod 755 /data/local/tmp/AT
        adb shell /data/local/tmp/AT ;;
    *)
        echo "Usage: $0 [build|host|clean|deploy|deploy-run]"; exit 2 ;;
esac
