THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

TARGET = npp.exe
TARGET_FILES = $(TARGET) npp.pdb
SRCDIR = $(TOPDIR)/trunk/PowerEditor
OUTDIR = $(WORKDIR)/npp
SCINTILLA_DIR = $(TOPDIR)/trunk/scintilla
FINDCMD = /usr/bin/find

INCLUDE_DIRS_BASE = $(SRCDIR)/src
INCLUDE_DIRS = $(SRCDIR)/include $(SCINTILLA_DIR)/include

DEP_MAKES = $(MAIN_DIR)scilexer.mak

SRC_FILE_EXCLUDE = \
	$(SRCDIR)/src/MISC/Process/ProcessAvecThread/Process.cpp \
	$(SRCDIR)/src/WinControls/TreeView/TreeView.cpp \
	$(shell $(FINDCMD) $(SRCDIR)/src/tools -name '*.cpp')
SRC_FILES := $(filter-out $(SRC_FILE_EXCLUDE), \
	$(shell $(FINDCMD) $(SRCDIR)/src -name '*.cpp' -or -name '*.c'))
RES_DIRS = $(SRCDIR)/src/WinControls $(SRCDIR)/src/ScitillaComponent
RES_FILES := \
	$(shell $(FINDCMD) $(RES_DIRS) -name '*.rc') \
	$(SRCDIR)/src/MISC/RegExt/regExtDlg.rc \
	$(SRCDIR)/src/Notepad_plus.rc \

DBG_INFO_CFLAGS = /Zi /Fd$(OUTDIR)/
PCH_CFLAGS_cpp = $$(PCH_USE_CFLAGS)
PCH_CFLAGS_c =

CFLAGS_VARS += CFLAGS RT_CFLAGS
NPP_CFLAGS_DEFS = WIN32 _WINDOWS _USE_64BIT_TIME_T \
	TIXML_USE_STL TIXMLA_USE_STL _CRT_NONSTDC_NO_DEPRECATE \
	_CRT_SECURE_NO_WARNINGS _CRT_NON_CONFORMING_SWPRINTFS=1 \
	_UNICODE UNICODE
CFLAGS = /GF /FD /EHa /Gy /WX /WX $(addprefix /D,$(NPP_CFLAGS_DEFS))

LDFLAGS_VARS = BASE_LDFLAGS LDFLAGS NPP_LIBS CPP_RT_LIBS
LDFLAGS_ms = /SUBSYSTEM:WINDOWS
NPP_LIBS_ms = comctl32.lib shlwapi.lib shell32.lib Oleacc.lib kernel32.lib \
	user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
	ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib oldnames.lib

RESFLAGS_VARS = RESFLAGS
RESFLAGS_ms = /dUNICODE

XML_FILES = stylers.model.xml config.model.xml langs.model.xml shortcuts.xml \
	contextMenu.xml functionList.xml userDefineLang.xml
PRE_INSTALL_DEP = $(INSTALL_PATH)/npp_lic.txt \
	$(addprefix $(INSTALL_PATH)/,$(XML_FILES))

$(eval $(call COMMON_RULES))

$(INSTALL_PATH)/npp_lic.txt: $(SRCDIR)/bin/License.txt | $(INSTALL_PATH)
	cp $< $@

$(addprefix $(INSTALL_PATH)/,$(XML_FILES)): $(INSTALL_PATH)/% : \
	$(TOPDIR)/trunk/PowerEditor/src/% | $(INSTALL_PATH)
	cp $< $@

