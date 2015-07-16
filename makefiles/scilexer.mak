THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

TARGET = SciLexer.dll
TARGET_FILES = $(TARGET) SciLexer.pdb
SRCDIR = $(TOPDIR)/trunk/scintilla
OUTDIR = $(WORKDIR)/scilexer
INCLUDE_DIRS_BASE = $(SRCDIR)
INCLUDE_DIRS = $(TOPDIR)/boost_1_54_0
CPPEXT = cxx
DEP_MAKES = $(MAIN_DIR)boost_regex.mak

VPATH = $(SRCDIR)/src $(SRCDIR)/lexlib $(SRCDIR)/win32 $(SRCDIR)/boostregex
SRC_FILES = Accessor.cxx Catalogue.cxx ExternalLexer.cxx LexerBase.cxx \
	LexerModule.cxx LexerSimple.cxx ScintillaWin.cxx \
	ScintillaBase.cxx StyleContext.cxx WordList.cxx AutoComplete.cxx \
	CallTip.cxx CellBuffer.cxx CharacterSet.cxx CharClassify.cxx \
	ContractionState.cxx Decoration.cxx Document.cxx Editor.cxx KeyMap.cxx \
	Indicator.cxx LineMarker.cxx PerLine.cxx PlatWin.cxx PositionCache.cxx \
	PropSetSimple.cxx RESearch.cxx RunStyles.cxx Selection.cxx Style.cxx \
	UniConversion.cxx ViewStyle.cxx BoostRegExSearch.cxx CaseFolder.cxx \
	CaseConvert.cxx UTF8DocumentIterator.cxx CharacterCategory.cxx \
	XPM.cxx $(wildcard $(SRCDIR)/lexers/Lex*.cxx)
RES_FILES = ScintRes.rc


CFLAGS_VARS += CFLAGS RT_CFLAGS
CFLAGS = /EHsc /DSCI_OWNREGEX /DSCI_LEXER

LDFLAGS_VARS = BASE_LDFLAGS LDFLAGS SCI_LIBS CPP_RT_LIBS
LDFLAGS_ms = /NODEFAULTLIB /LIBPATH:$(PLATDORM_DIR)
SCI_LIBS = kernel32.lib user32.lib gdi32.lib ole32.lib imm32.lib uuid.lib \
	libboost_regex.lib

PRE_COMPILE_DEP = scintilla_regen
PRE_INSTALL_DEP = $(INSTALL_PATH)/sci_lic.txt

$(eval $(call COMMON_RULES))

scintilla_regen: $(SRCDIR)/include/SciLexer.h $(SRCDIR)/src/Catalogue.cxx \
	$(SRCDIR)/include/Scintilla.h

$(SRCDIR)/src/Catalogue.cxx: $(SRCDIR)/include/Scintilla.iface \
	$(wildcard $(SRCDIR)/lexers/Lex*.cxx)
	@cd $(SRCDIR)/scripts && python LexGen.py

$(SRCDIR)/include/Scintilla.h $(SRCDIR)/include/SciLexer.h: \
	$(SRCDIR)/include/Scintilla.iface
	@cd $(SRCDIR)/scripts && python HFacer.py

$(INSTALL_PATH)/sci_lic.txt: $(SRCDIR)/License.txt | $(INSTALL_PATH)
	cp $< $@

.PHONY: scintilla_regen
