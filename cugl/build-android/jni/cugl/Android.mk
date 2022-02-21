###########################
#
# CUGL static library
#
###########################
LOCAL_PATH := $(call my-dir)
CUGL_PATH  := $(LOCAL_PATH)/../../..
include $(CLEAR_VARS)

LOCAL_MODULE := CUGL

LOCAL_C_INCLUDES := $(CUGL_PATH)/include
LOCAL_C_INCLUDES += $(CUGL_PATH)/external/box2d

# Add your application source files here...
LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(CUGL_PATH)/lib/base/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/base/platform/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/util/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/math/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/math/*.c) \
	$(wildcard $(CUGL_PATH)/lib/math/polygon/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/math/dsp/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/input/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/input/gestures/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/io/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/render/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/audio/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/audio/codecs/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/audio/graph/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/assets/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/scene2/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/scene2/graph/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/scene2/ui/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/scene2/layout/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/scene2/actions/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/physics2/*.cpp) \
	$(wildcard $(CUGL_PATH)/lib/net/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/cJSON/*.c) \
	$(wildcard $(CUGL_PATH)/external/poly2tri/common/*.cc) \
	$(wildcard $(CUGL_PATH)/external/poly2tri/sweep/*.cc) \
	$(wildcard $(CUGL_PATH)/external/clipper/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/box2d/collision/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/box2d/common/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/box2d/dynamics/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/box2d/rope/*.cpp) \
	$(wildcard $(CUGL_PATH)/external/slikenet/Source/src/*.cpp))

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES

LOCAL_EXPORT_LDLIBS := -Wl,--undefined=Java_org_libsdl_app_SDLActivity_nativeInit -ldl -lGLESv1_CM -lGLESv2 -lGLESv3 -llog -landroid -latomic
include $(BUILD_STATIC_LIBRARY)
