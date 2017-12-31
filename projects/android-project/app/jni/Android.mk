LOCAL_PATH				:= $(call my-dir)

include $(LOCAL_PATH)/eepp.mk

#************* empty_window *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= empty_window

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= examples/empty_window/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= eepp

include $(BUILD_SHARED_LIBRARY)
#************ empty_window ************

#************* external_shader *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= external_shader

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= examples/external_shader/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= eepp

include $(BUILD_SHARED_LIBRARY)
#************ external_shader ************

#************* full_test *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= main

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= test/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= eepp

include $(BUILD_SHARED_LIBRARY)
#************ full_test ************
