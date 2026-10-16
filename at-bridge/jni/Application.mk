# Application.mk - GsmTools AT Bridge

# بناء لـ ARM64 فقط (أجهزة سامسونج الحديثة كلها 64-بت)
APP_ABI          := arm64-v8a

# أقل إصدار أندرويد مدعوم
APP_PLATFORM     := android-29

# لا نحتاج STL
APP_STL          := none

# تحسين الحجم
APP_OPTIM        := release
