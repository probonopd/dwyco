ifeq "$(NDK_ABI)" "arm"
APP_PROJECT_PATH := $(HOME)/git/dwyco/phoobld
APP_ABI := armeabi-v7a
else
APP_PROJECT_PATH := $(HOME)/git/dwyco/phoobld
APP_ABI := x86
endif
APP_CPPFLAGS += -frtti -fexceptions -DANDROID -g
APP_CFLAGS += -DANDROID -g
APP_STL := gnustl_shared
#APP_MODULES := jhead cdc32 gsm lpc ppm pgm pbm vc crypto5 zlib kazlib jenkins dwcls
#APP_PLATFORM := android-20
#NDK_TOOLCHAIN_VERSION := 4.8
APP_PLATFORM := android-19
NDK_TOOLCHAIN_VERSION := 4.9
APP_OPTIM := release
