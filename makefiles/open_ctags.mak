THIS_MAKE := $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST))
THIS_DIR := $(dir $(THIS_MAKE))
MK = $(MAKE) --no-print-directory -r
DBGBLD = $(filter 1,$(DEBUG))

TOOLCHAIN=ms
TOPDIR = $(THIS_DIR)..
SRCDIR = $(TOPDIR)/open_ctags
WORKDIR = $(TOPDIR)/$(if $(DBGBLD),dbg,opt)
OUTDIR = $(WORKDIR)/open_ctags
TARGET=open_ctags.dll

INCLUDE_DIRS = \
	$(TOPDIR)/trunk/scintilla/include \
	$(TOPDIR)/trunk/PowerEditor/src/MISC/PluginsManager

VPATH = $(SRCDIR)
SRC_FILES = OpenCTagsFrm.cpp OpenCTagsForNpp.cpp OpenCTagsApp.cpp \
	octCWndCTags.cpp octCFileIndex.cpp
RES_FILES = $(SRCDIR)/OpenCTagsForNpp.rc

CFLAGS_VARS = BASE_CFLAGS OCTAGS_CFLAGS OCTAGS_RT_CFLAGS
CFLAGS_DEFS = WIN32 _WINDOWS _USRDLL OPENCTAGSFORNPP_EXPORTS \
	_CRT_SECURE_NO_WARNINGS _CRT_NON_CONFORMING_SWPRINTFS _WINDLL \
	_UNICODE UNICODE _CRT_NONSTDC_NO_DEPRECATE
OCTAGS_CFLAGS = /EHsc /W3 $(addprefix /D,$(CFLAGS_DEFS))
OPT_OCTAGS_RT_CFLAGS = /MT
DBG_OCTAGS_RT_CFLAGS = /MTd

LDFLAGS_VARS = BASE_LDFLAGS OCTAGS_LDFLAGS OCTAGS_LIBS OCTAGS_RT_LIBS
OCTAGS_LDFLAGS = /NODEFAULTLIB
OCTAGS_LIBS = kernel32.lib user32.lib gdi32.lib shell32.lib comctl32.lib \
	comdlg32.lib oldnames.lib
OPT_OCTAGS_RT_LIBS = libcmt.lib libcpmt.lib
DBG_OCTAGS_RT_LIBS = libcmtd.lib libcpmtd.lib


all:
	@$(MK) -f $(THIS_DIR)generic.mak

clean rebuild:
	@$(MK) -f $(THIS_DIR)generic.mak $@

.EXPORT_ALL_VARIABLES:

.phony: all clean rebuild
