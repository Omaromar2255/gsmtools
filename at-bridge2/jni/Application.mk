# Application.mk - GsmTools AT Bridge

# بناء لـ ARM64 و ARMv7 (دعم الأجهزة 64-بت و 32-بت)
APP_ABI          := arm64-v8a armeabi-v7a

# أقل إصدار أندرويد مدعوم
APP_PLATFORM     := android-29

# لا نحتاج STL
APP_STL          := none

# تحسين الحجم
APP_OPTIM        := release
