@echo off
REM ============================================================================
REM  GsmTools AT Bridge - Windows build script (v4.0)
REM  Requires Android NDK (set ANDROID_NDK_HOME) - falls back to common paths.
REM ============================================================================
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "REPO_DIR=%SCRIPT_DIR%.."
set "JNI_DIR=%SCRIPT_DIR%jni"
set "OUT_ROOT=%REPO_DIR%\payloads\bin"

if not defined ANDROID_NDK_HOME (
    if exist "%LOCALAPPDATA%\Android\Sdk\ndk" (
        for /f "delims=" %%D in ('dir /b /ad /on "%LOCALAPPDATA%\Android\Sdk\ndk" 2^>nul') do set "NDK_CAND=%%D"
        if defined NDK_CAND set "ANDROID_NDK_HOME=%LOCALAPPDATA%\Android\Sdk\ndk\%NDK_CAND%"
    )
)
if not defined ANDROID_NDK_HOME (
    echo [ERROR] ANDROID_NDK_HOME not set and no NDK found under %%LOCALAPPDATA%%\Android\Sdk\ndk
    echo         Install from https://developer.android.com/ndk/downloads
    exit /b 1
)
echo [*] Using NDK: %ANDROID_NDK_HOME%

set "TOOLCHAIN=%ANDROID_NDK_HOME%\toolchains\llvm\prebuilt\windows-x86_64\bin"
if not exist "%TOOLCHAIN%" set "TOOLCHAIN=%ANDROID_NDK_HOME%\toolchains\llvm\prebuilt\windows-x86_64\bin"

if not exist "%OUT_ROOT%\arm64-v8a"   mkdir "%OUT_ROOT%\arm64-v8a"
if not exist "%OUT_ROOT%\armeabi-v7a" mkdir "%OUT_ROOT%\armeabi-v7a"

echo [*] Building arm64-v8a (static, API 21)...
"%TOOLCHAIN%\aarch64-linux-android21-clang" -static -O2 -s -Wall -Wextra -fvisibility=hidden "%JNI_DIR%\at_bridge.c" -o "%OUT_ROOT%\arm64-v8a\AT" -ldl
if errorlevel 1 goto :fail

echo [*] Building armeabi-v7a (static, API 21)...
"%TOOLCHAIN%\armv7a-linux-androideabi21-clang" -static -O2 -s -Wall -Wextra -fvisibility=hidden "%JNI_DIR%\at_bridge.c" -o "%OUT_ROOT%\armeabi-v7a\AT_32" -ldl
if errorlevel 1 goto :fail

copy /Y "%OUT_ROOT%\arm64-v8a\AT"      "%REPO_DIR%\payloads\AT"      >nul
copy /Y "%OUT_ROOT%\armeabi-v7a\AT_32" "%REPO_DIR%\payloads\AT_32" >nul

echo [OK] Built payloads\AT (arm64) and payloads\AT_32 (arm)
exit /b 0

:fail
echo [ERROR] build failed
exit /b 1
