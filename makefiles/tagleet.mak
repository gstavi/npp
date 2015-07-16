THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

TARGET = TagLEET.dll
TARGET_FILES = $(TARGET) $(if $(DBGBLD),$(if $(MSBLD),TagLEET.pdb))
INSTALL_DIR = plugins
SRCDIR = $(TOPDIR)/tagleet
OUTDIR = $(WORKDIR)/tagleet

INCLUDE_DIRS = \
	$(TOPDIR)/trunk/scintilla/include \
	$(TOPDIR)/trunk/PowerEditor/src/MISC/PluginsManager

VPATH = $(SRCDIR)
TAG_ENGINE_FILES = file_reader.cpp file_reader_win.cpp tag_file.cpp tag_list.cpp avl.c
SRC_FILES = tag_leet_form.cpp tag_leet_npp.cpp tag_leet_app.cpp
SRC_FILES += $(addprefix tag_engine/,$(TAG_ENGINE_FILES))
RES_FILES = $(SRCDIR)/tagleet.rc

CFLAGS_VARS += CFLAGS RT_CFLAGS
CFLAGS_ms = /W4 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE
# For Optimized build we want to avoid debugging information completely
BASE_CFLAGS_ms_opt = /O2 /Os /GL /EHs-c- /GR- /GS- /DNDEBUG

LDFLAGS_VARS = BASE_LDFLAGS LDFLAGS TL_LIBS C_RT_LIBS
BASE_LDFLAGS_ms_opt =
LDFLAGS_ms_dbg = /NODEFAULTLIB
LDFLAGS_ms_opt = /NODEFAULTLIB /LTCG
TL_LIBS_ms = kernel32.lib user32.lib gdi32.lib comctl32.lib

CFLAGS_gcc = -D_WIN32_IE=0X0500 -DUNICODE -D_UNICODE
LDFLAGS_gcc = -s -static
TL_LIBS_gcc = -lgdi32 -lcomctl32 -lstdc++
BASE_CFLAGS_gcc_opt ?= -O2 -DNDEBUG

$(eval $(call COMMON_RULES))
