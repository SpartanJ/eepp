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

#************* ecode *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= ecode

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= tools/ecode/*.cpp \
							tools/ecode/plugins/*.cpp \
							tools/ecode/plugins/autocomplete/*.cpp \
							tools/ecode/plugins/git/*.cpp \
							tools/ecode/plugins/linter/*.cpp \
							tools/ecode/plugins/formatter/*.cpp \
							tools/ecode/plugins/lsp/*.cpp \
							tools/ecode/plugins/xmltools/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= efsw eterm eepp

include $(BUILD_SHARED_LIBRARY)
#************ ecode ************

#************* eterm *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= eterm-app

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= tools/eterm/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= eepp eterm

include $(BUILD_SHARED_LIBRARY)
#************ eterm ************

#************* full_test *************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_BASE_PATH)

LOCAL_MODULE			:= main

LOCAL_LDLIBS			:= $(EEPP_LDLIBS)

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

CORE_SRCS				:= tests/test_all/*.cpp

LOCAL_SRC_FILES			:= $(SDL_MAIN_PATH) $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= eepp-physics eepp-maps eepp

include $(BUILD_SHARED_LIBRARY)
#************ full_test ************
