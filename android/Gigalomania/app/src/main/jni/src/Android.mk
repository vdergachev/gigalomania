LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
SDL_IMAGE_PATH := ../SDL_image
SDL_MIXER_PATH := ../SDL_mixer

LOCAL_CPP_FEATURES += exceptions

LOCAL_CPPFLAGS += -DUSE_SDL3_LOGGING

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/$(SDL_IMAGE_PATH)/include \
	$(LOCAL_PATH)/$(SDL_MIXER_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	game.cpp \
	gamestate.cpp \
	gui.cpp \
	image.cpp \
	logging.cpp \
	main.cpp \
	panel.cpp \
	player.cpp \
	resources.cpp \
	screen.cpp \
	sector.cpp \
	sector_combat.cpp \
	sector_ai.cpp \
	sound.cpp \
	tutorial.cpp \
	utils.cpp \
	TinyXML/tinyxml.cpp \
	TinyXML/tinyxmlerror.cpp \
	TinyXML/tinyxmlparser.cpp

LOCAL_SHARED_LIBRARIES := SDL3 SDL3_image SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
