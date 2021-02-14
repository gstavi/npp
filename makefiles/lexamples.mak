THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

TARGET = lexamples.dll
TARGET_FILES = $(TARGET) $(if $(DBGBLD),$(if $(MSBLD),lexamples.pdb))
INSTALL_DIR = plugins
SRCDIR = $(TOPDIR)/lexamples
OUTDIR = $(WORKDIR)/lexamples

INCLUDE_DIRS = \
	$(TOPDIR)/trunk/scintilla/include \
	$(TOPDIR)/trunk/scintilla/lexlib \
	$(TOPDIR)/trunk/PowerEditor/src/MISC/PluginsManager

VPATH = $(SRCDIR)
SRC_FILES = lexamples_npp.cpp LexMake.cpp LexMib.cpp LexCommon.cpp \
	$(TOPDIR)/trunk/scintilla/lexlib/WordList.cxx  \
	$(TOPDIR)/trunk/scintilla/lexlib/DefaultLexer.cxx
RES_FILES = $(SRCDIR)/lexamples.rc

CFLAGS_VARS += CFLAGS RT_CFLAGS
CFLAGS_ms = /EHsc /W4 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE /DSCI_NAMESPACE
# For Optimized build we want to avoid debugging information completely
BASE_CFLAGS_ms_opt = /O2 /Os /GL /GR- /GS- /DNDEBUG

LDFLAGS_VARS = BASE_LDFLAGS LDFLAGS LEXAMPLES_LIBS CPP_RT_LIBS
BASE_LDFLAGS_ms_opt =
LEXAMPLES_LIBS_ms = kernel32.lib user32.lib
LDFLAGS_ms_dbg = /DEF:$(SRCDIR)/lexamples.def
LDFLAGS_ms_opt = /LTCG /DEF:$(SRCDIR)/lexamples.def

CFLAGS_gcc = -DUNICODE -D_UNICODE -DSCI_LEXER --std=c++0x -fno-rtti -fpermissive
LDFLAGS_gcc = -s -static $(SRCDIR)/lexamples.def
BASE_CFLAGS_gcc_opt = -O2 -DNDEBUG
LEXAMPLES_LIBS_gcc = -lstdc++

PRE_INSTALL_DEP = $(INSTALL_PATH)/config/lexamples.xml

$(eval $(call COMMON_RULES))

$(INSTALL_PATH)/config/lexamples.xml: $(SRCDIR)/lexamples.xml | $(INSTALL_PATH)/config
	cp $< $@

$(INSTALL_PATH)/config: ; mkdir -p $@
