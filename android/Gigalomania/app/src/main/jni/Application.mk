
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information

# we need support for STL and C++ exceptions:
#APP_STL := stlport_static
APP_STL := c++_static
#APP_STL := gnustl_static

# https://developer.android.com/ndk/guides/application_mk says APP_ABI is ignored, if instead specified
# by abiFilters in build.gradle
# see https://code.google.com/p/android/issues/detail?id=225123
#APP_ABI := armeabi-v7a armeabi x86
#APP_ABI := armeabi-v7a armeabi
#APP_ABI := armeabi

# The NDK takes the APP_PLATFORM from minSdkVersion in build.gradle if not specified
#APP_PLATFORM := android-14

# Needed to avoid compilation errors for SDL_image - oddly only appear when using SDL 2.0.4; 2.0.3 was fine
# Update: no longer needed for SDL 2.0.8, SDL_image 2.0.3
#APP_CFLAGS += -Wno-error=format-security
