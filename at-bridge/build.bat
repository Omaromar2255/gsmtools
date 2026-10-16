@echo off
REM ============================================================================
REM  GsmTools AT Bridge - Windows Build Script
REM  بناء ملف AT لأجهزة سامسونج على ويندوز
REM ============================================================================
REM
REM  متطلبات:
REM    - Android NDK (r21+)
REM    - تحميل: https://developer.android.com/ndk/downloads
REM    - فك الضغط وضبط ANDROID_NDK_HOME
REM
REM  الاستخدام:
REM    build.bat              بناء
REM    build.bat clean        تنظيف
REM    build.bat deploy       بناء + رفع للجهاز
REM    build.bat deploy-run   بناء + رفع + تشغيل
REM
REM ============================================================================

setlocal enabledelayedexpansion

set "PROJECT_DIR=%~dp0"
set "OUTPUT_DIR=%PROJECT_DIR%libs\arm64-v8a"
set "BINARY_NAME=AT"

REM ============ البحث عن NDK ============

set "NDK_PATH="

if defined ANDROID_NDK_HOME if exist "%ANDROID_NDK_HOME%\ndk-build.cmd" (
    set "NDK_PATH=%ANDROID_NDK_HOME%"
    goto :ndk_found
)

if defined ANDROID_NDK_ROOT if exist "%ANDROID_NDK_ROOT%\ndk-build.cmd" (
    set "NDK_PATH=%ANDROID_NDK_ROOT%"
    goto :ndk_found
)

REM البحث في Android SDK
if exist "%LOCALAPPDATA%\Android\Sdk\ndk" (
    for /f "delims=" %%D in ('dir /b /ad /o-n "%LOCALAPPDATA%\Android\Sdk\ndk" 2^>nul') do (
        if exist "%LOCALAPPDATA%\Android\Sdk\ndk\%%D\ndk-build.cmd" (
            set "NDK_PATH=%LOCALAPPDATA%\Android\Sdk\ndk\%%D"
            goto :ndk_found
        )
    )
)

REM البحث في مسارات شائعة
for %%P in (
    "C:\Android\ndk"
    "C:\android-ndk"
    "%PROGRAMFILES%\Android\ndk"
    "%USERPROFILE%\android-ndk"
) do (
    if exist "%%~P\ndk-build.cmd" (
        set "NDK_PATH=%%~P"
        goto :ndk_found
    )
)

echo ========================================
echo  ERROR: Android NDK not found!
echo ========================================
echo.
echo  Download NDK from:
echo    https://developer.android.com/ndk/downloads
echo.
echo  Then set environment variable:
echo    set ANDROID_NDK_HOME=C:\path\to\ndk
echo.
echo  Or install via Android Studio SDK Manager
echo ========================================
exit /b 1

:ndk_found
set "NDK_BUILD=%NDK_PATH%\ndk-build.cmd"
echo [*] Using NDK: %NDK_PATH%

REM ============ الأوامر ============

if "%1"=="clean" goto :do_clean
if "%1"=="deploy" goto :do_deploy
if "%1"=="deploy-run" goto :do_deploy_run
goto :do_build

:do_clean
echo [*] Cleaning...
call "%NDK_BUILD%" NDK_PROJECT_PATH="%PROJECT_DIR%" ^
    APP_BUILD_SCRIPT="%PROJECT_DIR%jni\Android.mk" ^
    NDK_APPLICATION_MK="%PROJECT_DIR%jni\Application.mk" ^
    clean
if exist "%PROJECT_DIR%obj" rd /s /q "%PROJECT_DIR%obj"
if exist "%PROJECT_DIR%libs" rd /s /q "%PROJECT_DIR%libs"
echo [OK] Clean done
goto :eof

:do_build
echo [*] Building AT Bridge...
call "%NDK_BUILD%" NDK_PROJECT_PATH="%PROJECT_DIR%" ^
    APP_BUILD_SCRIPT="%PROJECT_DIR%jni\Android.mk" ^
    NDK_APPLICATION_MK="%PROJECT_DIR%jni\Application.mk" ^
    -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo [OK] Build successful!
echo [*] Binary: %OUTPUT_DIR%\%BINARY_NAME%
if exist "%OUTPUT_DIR%\%BINARY_NAME%" (
    for %%F in ("%OUTPUT_DIR%\%BINARY_NAME%") do echo     Size: %%~zF bytes
)
goto :eof

:do_deploy
call :do_build
if errorlevel 1 exit /b 1
echo.
echo [*] Pushing to device...
adb push "%OUTPUT_DIR%\%BINARY_NAME%" /data/local/tmp/AT
adb shell chmod 755 /data/local/tmp/AT
echo [OK] Deployed to /data/local/tmp/AT
goto :eof

:do_deploy_run
call :do_deploy
if errorlevel 1 exit /b 1
echo.
echo [*] Running on device...
echo ========================================
adb shell /data/local/tmp/AT
goto :eof
