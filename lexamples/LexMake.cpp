/*  Copyright 2014, Gur Stavi, gur.stavi@gmail.com  */

/*
    This file is part of lexamples.

    lexamples is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    lexamples is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lexamples.  If not, see <http://www.gnu.org/licenses/>.
*/

 /* Includes required for LexCommon.h */
#include <assert.h>
#include <string>
#include "ILexer.h"
#include "Scintilla.h"
#include "LexAccessor.h"
#include "WordList.h"

#include "LexCommon.h"
#include <string.h>
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

class LexerMake;
class MakeEntity;
class MkTokener;

static const char defaultFunctionStrings[] = "subst patsubst strip findstring "
  "filter filter-out sort word words wordlist firstword lastword dir notdir "
  "suffix basename addsuffix addprefix join wildcard realpath abspath error "
  "warning info shell origin flavor foreach if or and call eval value";
static const char defaultDirectiveStrings[] = "define endef undefine ifdef "
  "ifeq ifneq else endif include sinclude -include override export unexport "
  "private vpath";
static const char defaultNmakeDirectiveStrings[] = "cmdswitches error message "
  "include if ifdef ifndef else elseif elseifdef elseifndef endif undef";

/* Makefile entities */

/** Entity types:
MK_TOP
MK_EMPTY_LINE       A line not yet classified
MK_DIRECTIVE_LINE   Starts with a directive, will not transorm to target
MK_TEXT_LINE        A line with at least one non-directive word
MK_PREPROC_LINE     An NMAKE pre-processor line
MK_TARGETS          With words that act as targets of a rule
MK_RECIPE           Line that starts with TAB
MK_PREREQ           Follows TARGETS in standard rule line
MK_TARGET_PATTERN   2nd part of static pattern rule
MK_PREREQ_PATTERN   3rd part of static pattern rule
MK_WORD             A word in a line
MK_COMMENT          Comment
MK_SINGLE_CHAR_VAR  A token of the form $X
MK_FUNC             Start of func/variable: $( can be child of almost anything
MK_BRACE_FUNC       Start of func/variable: ${ can be child of almost anything
MK_FUNC_KEYWORD     The first word in a func - If an internal function will
                    be transformed to MK_FUNCTION. child of MK_FUNC
MK_FUNCTION         An internal function.
MK_DIRECTIVE        A word that was detected as a known directive
MK_VPATH_DIRECTIVE  The specific vpath directive
MK_VPATH_PATTERN    The pattern word of a vpath directive
MK_DRCTIV_VAR_LIST  A var list following export, unexport, etc.
MK_INCL_DIRECTIVE   One of the include directives
MK_INCL_FILE        A file following include directive
MK_ASSIGN_OP        = or := or += or ?=
MK_VARIABLE         Variable. Often on left side of an asignment
MK_ASSIGN_SRC       Right side of an asignment.
MK_BAD_EOL_CONT     Backslash (\) followd by whitespace at end of line
**/

/* Define an enum of entity types and assign a tokening method to each */
#define MAKE_TYPES(TypE) \
  TypE(MK_EMPTY_LINE,       WithPreProcTest) \
  TypE(MK_TEXT_LINE,        WithWordStartTest) \
  TypE(MK_DIRECTIVE_LINE,   WithWordStartTest) \
  TypE(MK_PREPROC_LINE,     WithWordStartNoOps) \
  TypE(MK_EMPTY_RECIPE,     WithRecipeStart) \
  TypE(MK_RECIPE,           Default) \
  TypE(MK_TARGETS,          Default) \
  TypE(MK_PREREQ,           WithWordStartTest) \
  TypE(MK_TARGET_PATTERN,   Default) \
  TypE(MK_PREREQ_PATTERN,   Default) \
  TypE(MK_WORD,             WithWordEndTest) \
  TypE(MK_COMMENT,          WithCommentChars) \
  TypE(MK_SINGLE_CHAR_VAR,  Default) \
  TypE(MK_FUNC,             Default) \
  TypE(MK_BRACE_FUNC,       Default) \
  TypE(MK_FUNC_KEYWORD,     WithKeywordEndTest) \
  TypE(MK_FUNCTION,         Default) \
  TypE(MK_DIRECTIVE,        Default) \
  TypE(MK_VPATH_DIRECTIVE,  Default) \
  TypE(MK_VPATH_PATTERN,    Default) \
  TypE(MK_DRCTIV_VAR_LIST,  WithWordStartTest) \
  TypE(MK_INCL_DIRECTIVE,   Default) \
  TypE(MK_INCL_FILE,        Default) \
  TypE(MK_ASSIGN_OP,        Default) \
  TypE(MK_VARIABLE,         Default) \
  TypE(MK_ASSIGN_SRC,       Default) \
  TypE(MK_BAD_EOL_CONT,     Default)

enum MkType {
MK_TOP = 0,
#define TypE_enum(NAME,FUNC) NAME,
MAKE_TYPES(TypE_enum)
MK_LAST
};

typedef enum {
  TOK_NULL = 0,
  TOK_CHAR,
  TOK_EOL,
  TOK_EOL_CONT,
  TOK_BAD_EOL_CONT,
  TOK_WORD_START,
  TOK_WORD_END,
  TOK_KEYWORD_END,
  TOK_FUNC_START,
  TOK_BRACE_FUNC_START,
  TOK_PREPROC_START,
  TOK_RECIPE_START,
  TOK_ASSIGN_OP,
  TOK_COLON,
  TOK_DOUBLE_COLON,
  TOK_SINGLE_CHAR_VAR,
  TOK_SKIP,
  TOK_DBL_DOLLAR_SIGN,
  TOK_ESCAPED_CHAR
} MkToken;

typedef enum {
  FOLD_DEFAULT = 0,
  FOLD_CONTINUATION = 1,
  FOLD_RECIPE = 2,
  FOLD_MASK = 0x3 /* A mask the covers all relevant fold levels */
} MkFoldLevel;

class LexerMake : public LexerCommon
{
public:
  LexerMake();
  virtual ~LexerMake() {}

  int FindValidLineStart(LexAccessor *styler, int pos);
  void DoLex(DoLexContext *ctx);
  void PropertyUpdateNotification(void *PropPtr, int PropOffset);

  std::string functionStrings;
  std::string directiveStrings;
  std::string nmakeDirectiveStrings;

  WordList functions;
  WordList directives;
  WordList includeDirectives;
  WordList implyVar;
  WordList implyVarList;
  WordList nmakeDirectives;
  WordList preReqDirectives;
  WordList assignDirectives;

  /* Configurable properties */
  int  hlUnterminatedIdent;
  bool fold;
  bool hlVarInIdent;
  bool hlIdentInTarget;
  bool hlIdentInRecipe;
  bool hlIdentInPreReq;
  bool hlIdentInVariable;
  bool hlIdentInAssignSrc;
  bool hlBadEolCont;
  bool hlIdentWithDblDollar;

  static ILexer *LexerFactoryMake();
  static const char *wordListDesc[];
};

class MakeEntity : public LexerEntity
{
  public:
  MakeEntity(int type, int pos);
  ~MakeEntity() {}

  MakeEntity *ProcessToken(MkTokener *tokener);

  MakeEntity *CreateSibling(int sibType, int sibSize=0, int sibPos=-1) {
    return static_cast<MakeEntity *>(LcCreateSibling(sibType, sibSize, sibPos));}
  MakeEntity *CreateChild(int childType, int childSize=0, int childPos=-1) {
    return static_cast<MakeEntity *>(LcCreateChild(childType, childSize, childPos));}
  MakeEntity *FinishElement(int endPos) {
    return static_cast<MakeEntity *>(LcFinishElement(endPos));}

private:
  LexerEntity *Create(int type, int pos) const;
  void PostCreate();
  int GetStyle(LexerCommon::DoLexContext *ctx) const;
  bool CanHaveMakeComment() const;
  void MarkOpenChild(bool hasOpenChild);
  MakeEntity *ProcessWordEndToken(LexerCommon::DoLexContext *ctx,
    MakeEntity *wordEnt);

  /* Count bracket balance in an identifier. In a variable invocation we don't
   * really count. We only need a single closing bracket. '$(((AAA)' is valid.
   * In function calls sticter counting is needed. '$(info ($(AAA))' is not
   * valid */
  int count;
  bool strict_count;
  bool hasOpenChild;
};

class MkTokener
{
public:
  MkTokener(LexerCommon::DoLexContext *inCtx, int inPos);
  ~MkTokener() {}
  void Reset();

  void SetNextToken();
  void ProcessingLoop();
  MakeEntity *FlushStyles();
  void SetFoldLevel();
  bool SetTok(MkToken inTok, int inTokSize) {
    tokSize = inTokSize;
    tok = inTok;
    return true;
  }

  /* Auxilary methods to detect a subset of tokens
   * Return true if a token was set */
  bool EolTest(int pos);
  bool HighPrioTokenTest();
  bool OpTokenTest();
  bool EndOfWord();

  /* These will always find a token, at least MK_CHAR. They return bool to
   * allow the use of: return SetTok() */
  bool Default();
  bool WithOperators();
  bool WithWordStartTest();
  bool WithWordStartNoOps();
  bool WithPreProcTest();
  bool WithWordEndTest();
  bool WithKeywordEndTest();
  bool WithCommentChars();
  bool WithRecipeStart();

  /* Array of tokenning method per entity type. Contains the methods above  */
  static bool (MkTokener::* tokenerMethods[MK_LAST])();

  LexerCommon::DoLexContext *ctx;
  MakeEntity top;
  LexerMake *lexer;
  MakeEntity *ent;
  MkToken tok;
  int tokSize;
  int lineNumber;
  int prevFoldLevel;
  int foldLevel;
  int ContLineCount;
  int type;
  int pType;
  int currPos;
  int docLength;
  bool atEndOfDoc;
  char ch[2];
};

LexerMake::LexerMake()
{
  functionStrings = defaultFunctionStrings;
  functions.Set(functionStrings.c_str());
  directiveStrings = defaultDirectiveStrings;
  directives.Set(directiveStrings.c_str());
  includeDirectives.Set("include sinclude -include");
  implyVar.Set("define undefine ifdef override export unexport private");
  implyVarList.Set("export unexport");
  preReqDirectives.Set("undefine override export private");
  assignDirectives.Set("override export unexport private");

  nmakeDirectiveStrings = defaultNmakeDirectiveStrings;
  nmakeDirectives.Set(nmakeDirectiveStrings.c_str());

  hlUnterminatedIdent = 2;
  fold = true;
  hlVarInIdent = false;
  hlIdentInTarget = false;
  hlIdentInRecipe = false;
  hlIdentInPreReq = false;
  hlIdentInVariable = true;
  hlIdentInAssignSrc = true;
  hlBadEolCont = true;
  hlIdentWithDblDollar = true;

  DefinePropertyCommonLexer("fold", fold, NULL);
  DefinePropertyCommonLexer("lexer.makefile.unterminated.ident", hlUnterminatedIdent,
    "Highlight unterminated identifier. 0-don't highlight 1-highlight "
    "outermost 2-highlight innermost.");
  DefinePropertyCommonLexer("lexer.makefile.var.in.ident", hlVarInIdent,
    "Highlight variable in identifier");
  DefinePropertyCommonLexer("lexer.makefile.ident.in.target", hlIdentInTarget,
    "Highlight identifiers in target");
  DefinePropertyCommonLexer("lexer.makefile.ident.in.recipe", hlIdentInRecipe,
    "Highlight identifiers in recipe lines");
  DefinePropertyCommonLexer("lexer.makefile.ident.in.pre.requisite", hlIdentInPreReq,
    "Highlight identifiers in pre-requisites");
  DefinePropertyCommonLexer("lexer.makefile.ident.in.variable", hlIdentInVariable,
    "Highlight identifiers in variables. Variables appear on the left side of "
    "an assignment or following specific directives like export");
  DefinePropertyCommonLexer("lexer.makefile.ident.in.assign.source", hlIdentInAssignSrc,
    "Highlight identifiers in right side of an asignment");
  DefinePropertyCommonLexer("lexer.makefile.bad.eol.cont", hlBadEolCont,
    "Highlight bad EOL continuation where space trails a backslash (\\) "
    "before EOL");
  DefinePropertyCommonLexer("lexer.makefile.ident.with.dbl.dollar", hlIdentWithDblDollar,
    "Treat $$( as the beginning of an identifier");
  DefinePropertyCommonLexer("lexer.makefile.functions", functionStrings,
    "List of functions that are highlighted inside $(func ...) expressions.");
  DefinePropertyCommonLexer("lexer.makefile.directives", directiveStrings,
    "List of directives that are recognized by the lexer.");
  DefinePropertyCommonLexer("lexer.makefile.nmake.directives", nmakeDirectiveStrings,
    "List of directives (in lower case) that are recognized inside an nmake "
    "pre processor line.");
}

int LexerMake::FindValidLineStart(LexAccessor *styler, int pos)
{
  int validPos = -1;
  int endPos = pos;

  for(;;)
  {
    int i, prePos;
    char ch = '\0';
    char prevCh;
    if (endPos == 0)
      return endPos;

    prePos = endPos > 256 ? endPos - 256 : 0;
    i = prePos;
    prevCh = '\\';
    for (;;)
    {
      /* Find next EOL */
      while (i <= endPos)
      {
        ch = styler->SafeGetCharAt(i);
        i++;
        if (ch == '\n' || ch == '\r')
          break;
        prevCh = ch;
      }
      if (i > endPos)
        break;
      /* Skip EOL to next char */
      if (ch == '\r')
      {
        ch = styler->SafeGetCharAt(i);
        if (ch == '\n')
        {
          if (i == endPos)
            break;
          i++;
        }
      }
      if (prevCh != '\\')
        validPos = i;
    }
    if (validPos != -1)
      return validPos;
    endPos = prePos;
  }
}

void LexerMake::DoLex(DoLexContext *ctx)
{
  int pos = FindValidLineStart(&ctx->styler, ctx->startPos);
  /* For folding, we need to start a line earlier since a new recipe line can
   * add a folding point at the line before */
  if (fold && pos > 0)
    pos = FindValidLineStart(&ctx->styler, pos-1);

  MkTokener tokener(ctx, pos);
  tokener.ProcessingLoop();
}

void LexerMake::PropertyUpdateNotification(void *, int PropOffset)
{
  switch (PropOffset)
  {
    case PROP_OFFSET(LexerMake, functionStrings):
      if (functionStrings.empty())
        functionStrings = defaultFunctionStrings;
      functions.Set(functionStrings.c_str());
      break;
    case PROP_OFFSET(LexerMake, directiveStrings):
      if (directiveStrings.empty())
        directiveStrings = defaultDirectiveStrings;
      directives.Set(directiveStrings.c_str());
      break;
    case PROP_OFFSET(LexerMake, nmakeDirectiveStrings):
      if (nmakeDirectiveStrings.empty())
        nmakeDirectiveStrings = defaultNmakeDirectiveStrings;
      nmakeDirectives.Set(nmakeDirectiveStrings.c_str());
      break;
  }
}

MakeEntity::MakeEntity(int type, int pos):
  LexerEntity(type, pos)
{
  count = 0;
  strict_count = false;
  hasOpenChild = false;
}

LexerEntity *MakeEntity::Create(int type, int pos) const
{
  return new MakeEntity(type, pos);
}

void MakeEntity::PostCreate()
{
  if (parent != NULL)
    strict_count = static_cast<MakeEntity *>(parent)->strict_count;
}

int MakeEntity::GetStyle(LexerCommon::DoLexContext *ctx) const
{
  int style;
  MakeEntity *p = static_cast<MakeEntity *>(parent);
  const LexerMake *lexMake = static_cast<const LexerMake *>(ctx->lexer);

  switch (type)
  {
    case MK_COMMENT:
      return SCE_MAKE_COMMENT;
    case MK_INCL_DIRECTIVE:
    case MK_VPATH_DIRECTIVE:
    case MK_DIRECTIVE:
      if (p->type == MK_PREPROC_LINE)
        return SCE_MAKE_PREPROCESSOR_DIRECTIVE;
      return SCE_MAKE_DIRECTIVE;
    case MK_VPATH_PATTERN:
      return SCE_MAKE_VPATH_PATTERN;
    case MK_INCL_FILE:
      return SCE_MAKE_INCLUDE_FILE;
    case MK_TARGETS:
      return SCE_MAKE_TARGET;
    case MK_PREPROC_LINE:
      return SCE_MAKE_PREPROCESSOR;
    case MK_TARGET_PATTERN:
      return SCE_MAKE_TARGET_PATTERN;
    case MK_FUNCTION:
      style = p->GetStyle(ctx);
      return style == SCE_MAKE_UNTERMINATED_IDENTIFIER ?
        SCE_MAKE_UNTERMINATED_FUNCTION : SCE_MAKE_FUNCTION;
    case MK_SINGLE_CHAR_VAR:
      return SCE_MAKE_IDENTIFIER;
    case MK_WORD:
      return p->GetStyle(ctx);
    case MK_FUNC_KEYWORD:
      return p->GetStyle(ctx);
    case MK_FUNC:
    case MK_BRACE_FUNC:
      switch (lexMake->hlUnterminatedIdent)
      {
        case 1:
          if (count > 0)
            return SCE_MAKE_UNTERMINATED_IDENTIFIER;
          break;
        case 2:
          if (count > 0 && !hasOpenChild)
            return SCE_MAKE_UNTERMINATED_IDENTIFIER;
          break;
      }
      style = p->GetStyle(ctx);
      switch (style)
      {
        case SCE_MAKE_TARGET:
        case SCE_MAKE_TARGET_PATTERN:
          if (lexMake->hlIdentInTarget)
            return SCE_MAKE_IDENTIFIER;
          break;
        case SCE_MAKE_RECIPE_LINE:
          if (lexMake->hlIdentInRecipe)
            return SCE_MAKE_IDENTIFIER;
          break;
        case SCE_MAKE_PREREQ:
        case SCE_MAKE_PREREQ_PATTERN:
          if (lexMake->hlIdentInPreReq)
            return SCE_MAKE_IDENTIFIER;
          break;
        case SCE_MAKE_VARIABLE:
          if (lexMake->hlIdentInVariable)
            return SCE_MAKE_IDENTIFIER;
          break;
        case SCE_MAKE_ASSIGN_SRC:
          if (lexMake->hlIdentInAssignSrc)
            return SCE_MAKE_IDENTIFIER;
          break;
        case SCE_MAKE_DEFAULT:
          return SCE_MAKE_IDENTIFIER;
      }
      return style;
    case MK_BAD_EOL_CONT:
      return SCE_MAKE_BAD_EOL_CONTINUATION;
    case MK_PREREQ:
      return SCE_MAKE_PREREQ;
    case MK_PREREQ_PATTERN:
      return SCE_MAKE_PREREQ_PATTERN;
    case MK_RECIPE:
      return SCE_MAKE_RECIPE_LINE;
    case MK_VARIABLE:
      if (p->type == MK_FUNC || p->type == MK_BRACE_FUNC)
      {
        style = p->GetStyle(ctx);
        if (style != SCE_MAKE_IDENTIFIER)
          return style;
      }
      return SCE_MAKE_VARIABLE;
    case MK_ASSIGN_SRC:
      return SCE_MAKE_ASSIGN_SRC;
    case MK_ASSIGN_OP:
      return SCE_MAKE_OPERATOR;
  }
  return SCE_MAKE_DEFAULT;
}

void MakeEntity::MarkOpenChild(bool hasOpenChild)
{
  MakeEntity *tmpEnt = this;

  while (tmpEnt->parent != NULL)
  {
    tmpEnt = static_cast<MakeEntity *>(tmpEnt->parent);
    switch (tmpEnt->type)
    {
      case MK_FUNC:
      case MK_BRACE_FUNC:
        tmpEnt->hasOpenChild = hasOpenChild;
        return;
    }
  }
}

bool MakeEntity::CanHaveMakeComment() const
{
  const MakeEntity *tmpEnt = this;

  while (tmpEnt->type != MK_TOP)
  {
    switch (tmpEnt->type)
    {
      case MK_RECIPE:
      case MK_COMMENT:
        return false;
    }
    tmpEnt = static_cast<MakeEntity *>(tmpEnt->parent);
  }
  return true;
}

#define TEST_ENT_ALLOC(e) if (e == NULL) return NULL

MakeEntity *MakeEntity::ProcessWordEndToken(LexerCommon::DoLexContext *ctx,
  MakeEntity *wordEnt)
{
  LexerMake *lexer = static_cast<LexerMake *>(ctx->lexer);
  MakeEntity *p = static_cast<MakeEntity *>(wordEnt->parent);
  char str[64];

  switch (p->type)
  {
    case MK_PREPROC_LINE:
      if (wordEnt->PrevType() == MK_INCL_DIRECTIVE)
      {
        wordEnt->type = MK_INCL_FILE;
        /* continue adding tokens to the file entity instead returning to the
         * directive line so all words will be colored as file */
        return wordEnt;
      }
      wordEnt->GetStrLowered(&ctx->styler, str, sizeof(str));
      if (lexer->nmakeDirectives.InList(str))
      {
        wordEnt->type = ::strcmp(str,"include") == 0 ? MK_INCL_DIRECTIVE :
          MK_DIRECTIVE;
      }
      return p;
    case MK_DRCTIV_VAR_LIST:
      wordEnt->type = MK_VARIABLE;
      return p;
    case MK_EMPTY_LINE:
    case MK_DIRECTIVE_LINE:
      break;
    default:
      return p;
  }

  /* Handle word over empty or directive line */
  wordEnt->GetStr(&ctx->styler, str, sizeof(str));
  if (lexer->directives.InList(str))
  {
    /* Handle a directive word */
    if (wordEnt->prevSib == NULL && lexer->implyVarList.InList(str))
    {
      p->type = MK_DRCTIV_VAR_LIST;
      wordEnt->type = MK_DIRECTIVE;
      return p;
    }
    p->type = MK_DIRECTIVE_LINE;
    wordEnt->type =
      ::strcmp(str,"vpath") == 0 ? MK_VPATH_DIRECTIVE :
      lexer->includeDirectives.InList(str) ? MK_INCL_DIRECTIVE :
      MK_DIRECTIVE;
    return p;
  }
  /* First word in an empty line */
  if (p->type == MK_EMPTY_LINE)
  {
    p->type = MK_TEXT_LINE;
    return p;
  }
  /* Non directive word in a directive line */
  switch (wordEnt->PrevType())
  {
    case MK_VPATH_DIRECTIVE:
      wordEnt->type = MK_VPATH_PATTERN;
      return p;
    case MK_INCL_DIRECTIVE:
      wordEnt->type = MK_INCL_FILE;
      return wordEnt;
    case MK_DIRECTIVE:
      if (wordEnt->prevSib->InWordList(&ctx->styler, &lexer->implyVar))
        wordEnt->type = MK_VARIABLE;
      return p;
  }
  return p;
}

MakeEntity *MakeEntity::ProcessToken(MkTokener *tokener)
{
  LexerCommon::DoLexContext *ctx = tokener->ctx;
  LexerMake *lexer = tokener->lexer;
  int tokSize = tokener->tokSize;
  MakeEntity *ent = this;
  MakeEntity *tmpEnt;
  int nextPos = tokener->currPos + tokSize;
  int lineType;

  switch ((int)tokener->tok)
  {
    case TOK_CHAR:
      switch (tokener->ch[0])
      {
        case '#':
          if (CanHaveMakeComment())
          {
            ent = CreateSibling(MK_COMMENT, tokSize);
            TEST_ENT_ALLOC(ent);
          }
          break;
        case '(':
          if (type == MK_FUNC && strict_count)
            count++;
          break;
        case ')':
          if (type == MK_FUNC)
          {
            count--;
            if (count == 0)
            {
              MarkOpenChild(false);
              ent = FinishElement(nextPos);
            }
          }
          break;
        case '{':
          if (type == MK_BRACE_FUNC && strict_count)
            count++;
          break;
        case '}':
          if (type == MK_BRACE_FUNC)
          {
            count--;
            if (count == 0)
            {
              MarkOpenChild(false);
              ent = FinishElement(nextPos);
            }
          }
          break;
        case '\t':
          if (type == MK_EMPTY_LINE && size == 0)
            type = MK_EMPTY_RECIPE;
          break;
        case ';':
          tmpEnt = type == MK_PREREQ_PATTERN ? ent :
            ParentType() == MK_PREREQ ? static_cast<MakeEntity *>(parent) :
            NULL;
          if (tmpEnt != NULL)
          {
            ent = tmpEnt->CreateSibling(MK_RECIPE, tokSize);
            TEST_ENT_ALLOC(ent);
          }
          break;
      }
      break;
    case TOK_EOL:
      size = nextPos - pos;
      tokener->SetFoldLevel();
      ent = tokener->FlushStyles();
      TEST_ENT_ALLOC(ent);
      /* Indicate that processing loop should exit */
      if (tokener->atEndOfDoc)
        ctx->doneStyling = true;
      tokener->ContLineCount = 0;
      break;
    case TOK_EOL_CONT:
      tokener->ContLineCount++;
      break;
    case TOK_BAD_EOL_CONT:
      tmpEnt = CreateChild(MK_BAD_EOL_CONT, tokSize);
      TEST_ENT_ALLOC(tmpEnt);
      break;
    case TOK_PREPROC_START:
      type = MK_PREPROC_LINE;
      break;
    case TOK_RECIPE_START:
      tokener->foldLevel = FOLD_RECIPE;
      type = MK_RECIPE;
      break;
    case TOK_COLON:
    case TOK_DOUBLE_COLON:
      tmpEnt = type == MK_WORD ? static_cast<MakeEntity *>(parent) : ent;
      lineType = tmpEnt->Type();
      switch (lineType)
      {
        case MK_PREREQ:
          tmpEnt->type = MK_TARGET_PATTERN;
          ent = tmpEnt->CreateSibling(MK_PREREQ_PATTERN, 0, nextPos);
          TEST_ENT_ALLOC(ent);
          break;
        case MK_DIRECTIVE_LINE:
          if (type != MK_WORD)
          {
            /* Create a word with only the colon to prevent propogating effect
             * of previous directives like 'export' to next word */
            CreateChild(MK_WORD, tokSize);
          }
          break;
        case MK_EMPTY_LINE:
        case MK_TEXT_LINE:
          if (type == MK_WORD)
            ent = FinishElement(nextPos);
          tokener->foldLevel = FOLD_DEFAULT;
          ent->type = MK_TARGETS;
          ent = ent->CreateSibling(MK_PREREQ, 0, nextPos);
          TEST_ENT_ALLOC(ent);
      }
     break;
    case TOK_SINGLE_CHAR_VAR:
      tmpEnt = CreateChild(MK_SINGLE_CHAR_VAR, tokSize);
      TEST_ENT_ALLOC(tmpEnt);
      break;
    case TOK_FUNC_START:
      ent = CreateChild(MK_FUNC, tokSize);
      TEST_ENT_ALLOC(ent);
      ent->count = 1;
      ent->MarkOpenChild(true);
      ent = ent->CreateChild(MK_FUNC_KEYWORD);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_BRACE_FUNC_START:
      MarkOpenChild(true);
      ent = CreateChild(MK_BRACE_FUNC, tokSize);
      TEST_ENT_ALLOC(ent);
      ent->count = 1;
      ent->MarkOpenChild(true);
      ent = ent->CreateChild(MK_FUNC_KEYWORD);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_ASSIGN_OP:
      type = MK_ASSIGN_OP;
      if (prevSib != NULL)
      {
        const WordList *directives;
        tmpEnt = static_cast<MakeEntity *>(prevSib);
        tmpEnt->type = MK_VARIABLE;
        /* Test if previous words are valid directives that should
         * be highlighted */
        directives = ParentType() == MK_PREREQ ? &lexer->preReqDirectives :
          &lexer->assignDirectives;
        while (tmpEnt->prevSib != NULL)
        {
          tmpEnt = static_cast<MakeEntity *>(tmpEnt->prevSib);
          if (tmpEnt->InWordList(&ctx->styler, directives))
            tmpEnt->type = MK_DIRECTIVE;
          else if (tmpEnt->type == MK_VARIABLE)
            tmpEnt->type = MK_WORD;
        }
      }
      if (parent != NULL)
      {
        tmpEnt = static_cast<MakeEntity *>(parent);
        tmpEnt->type = MK_TEXT_LINE;
      }
      ent = CreateSibling(MK_ASSIGN_SRC,0, nextPos);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_WORD_START:
      ent = CreateChild(MK_WORD);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_KEYWORD_END:
      if (InWordList(&ctx->styler, &lexer->functions))
      {
        type = MK_FUNCTION;
        static_cast<MakeEntity *>(parent)->strict_count = true;
      }
      else if (firstChild == NULL && lexer->hlVarInIdent)
        type = MK_VARIABLE;
      ent = FinishElement(nextPos);
      break;
    case TOK_WORD_END:
      ent->size = nextPos - ent->pos;
      ent = ProcessWordEndToken(ctx, ent);
      break;
  }

  ent->size = nextPos - ent->pos;
  return ent;
}

MkTokener::MkTokener(LexerCommon::DoLexContext *inCtx, int inPos):
  top(MK_TOP, inPos)
{
  ctx = inCtx;
  lexer = static_cast<LexerMake *>(ctx->lexer);
  currPos = inPos;
  docLength = ctx->styler.Length();
  Reset();
}

void MkTokener::Reset()
{
  ent = NULL;
  tok = TOK_NULL;
  tokSize = 0;
  lineNumber = ctx->styler.GetLine(currPos);
  prevFoldLevel = lineNumber > 0 ?
    ctx->styler.LevelAt(lineNumber - 1) : FOLD_DEFAULT;
  foldLevel = FOLD_DEFAULT;
  ContLineCount = 0;
  type = MK_TOP;
  pType = MK_TOP;
  atEndOfDoc = false;
  ch[0] = '\0';
  ch[1] = '\0';
}

void MkTokener::SetNextToken()
{
  type = ent->Type();
  pType = ent->ParentType();
  ch[0] = ctx->styler.SafeGetCharAt(currPos, '\0');
  ch[1] = ctx->styler.SafeGetCharAt(currPos + 1, '\0');

  (this->*tokenerMethods[type])();
}

void MkTokener::ProcessingLoop()
{
  ctx->StartAt(currPos);
  Reset();
  ent = top.CreateChild(MK_EMPTY_LINE);

  while (ent != NULL && !ctx->doneStyling)
  {
    SetNextToken();
    ent = ent->ProcessToken(this);
    currPos += tokSize;
  }
  /* Call once more to set the folding state of previous line */
  SetFoldLevel();
}

MakeEntity *MkTokener::FlushStyles()
{
  MakeEntity *ent;

  top.SetStyles(ctx);
  ent = top.CreateChild(MK_EMPTY_LINE);
  return ent;
}

void MkTokener::SetFoldLevel()
{
  if (!lexer->fold)
    return;

  bool foldCont = false;
  int baseType = top.FirstChType();
  if (foldLevel == FOLD_DEFAULT && ContLineCount > 0)
    foldCont = true;
  if (baseType == MK_EMPTY_LINE || baseType == MK_EMPTY_RECIPE)
    foldLevel |= SC_FOLDLEVELWHITEFLAG;

  /* We finished lexing the current line, now we can set the fold level and
   * flags of the previous line. */
  for(;;)
  {
    if (lineNumber > 0)
    {
      int prevVal = prevFoldLevel & FOLD_MASK;
      int currVal = foldLevel & FOLD_MASK;
      if (prevVal < currVal)
        prevFoldLevel |= SC_FOLDLEVELHEADERFLAG;
      else
        prevFoldLevel &= ~SC_FOLDLEVELHEADERFLAG;
      ctx->styler.SetLevel(lineNumber - 1, prevFoldLevel | SC_FOLDLEVELBASE);
    }
    lineNumber++;
    prevFoldLevel = foldLevel;
    if (ContLineCount == 0)
      break;
    if (foldCont)
    {
      /* Update 'Make' fold level without reseting the flags */
      foldLevel &= ~FOLD_MASK;
      foldLevel |= FOLD_CONTINUATION;
      foldCont = false;
    }
    ContLineCount--;
  }
  /* Reset foldLevel before lexing next line */
  foldLevel = FOLD_DEFAULT;
}

/* Checks at pos for UNIX(\n) Windows(\r\n) or Mac(\r) end of line */
bool MkTokener::EolTest(int pos)
{
  char tmpCh;

  tmpCh = ctx->styler.SafeGetCharAt(pos, '\0');
  if (tmpCh == '\n')
    return SetTok(TOK_EOL, 1);

  if (tmpCh == '\r')
  {
    tmpCh = ctx->styler.SafeGetCharAt(pos + 1, '\0');
    return SetTok(TOK_EOL, tmpCh == '\n' ? 2 : 1);
  }

  /* End of document is equivilent to EOL */
  if (pos >= docLength)
  {
    atEndOfDoc = true;
    return SetTok(TOK_EOL, 0);
  }
  return false;
}

bool MkTokener::HighPrioTokenTest()
{
  if (EolTest(currPos))
    return true;

  if (ch[0] == '\\')
  {
    if (ch[1] == '\\' || ch[1] == '#')
      return SetTok(TOK_ESCAPED_CHAR, 2);

    if (EolTest(currPos + 1))
      return SetTok(TOK_EOL_CONT, tokSize + 1);

    if (lexer->hlBadEolCont)
    {
      char tmpCh = ch[1];
      int i = 1;
      while (tmpCh == ' ' || tmpCh == '\t')
      {
        i++;
        tmpCh = ctx->styler.SafeGetCharAt(currPos + i, '\0');
      }
      if (i > 1 && EolTest(currPos + i))
      {
        /* Don't include the EOL itself */
        return SetTok(TOK_BAD_EOL_CONT, i);
      }
    }
    return SetTok(TOK_CHAR, 1);
  }

  if (ch[0] == '$')
  {
    switch (ch[1])
    {
      case '(':
        return SetTok(TOK_FUNC_START, 2);
      case '{':
        return SetTok(TOK_BRACE_FUNC_START, 2);
      case '$':
        if (lexer->hlIdentWithDblDollar)
        {
          char tmpCh = ctx->styler.SafeGetCharAt(currPos + 2, '\0');
          switch (tmpCh)
          {
            case '(':
              return SetTok(TOK_FUNC_START, 3);
            case '{':
              return SetTok(TOK_BRACE_FUNC_START, 3);
            default:
              break;
          }
        }
        return SetTok(TOK_DBL_DOLLAR_SIGN, 2);
      case ' ':
      case '\t':
      case '#':
      case '=':
      case ':':
        return SetTok(TOK_SINGLE_CHAR_VAR, 1);
      default:
        return SetTok(TOK_SINGLE_CHAR_VAR, 2);
    }
  }
  return false;
}

bool MkTokener::OpTokenTest()
{
  switch (ch[0])
  {
    case '=':
      return SetTok(TOK_ASSIGN_OP, 1);
    case '?':
    case '+':
      return ch[1] == '=' ? SetTok(TOK_ASSIGN_OP, 2): false ;
    case ':':
      switch (ch[1])
      {
        case '=':
          return SetTok(TOK_ASSIGN_OP, 2);
        case ':':
          return SetTok(TOK_DOUBLE_COLON, 2);
        default:
          return SetTok(TOK_COLON, 1);
      }
  }
  return false;
}

bool MkTokener::EndOfWord()
{
  switch ((int)tok)
  {
    case TOK_CHAR:
      return ch[0] == ' ' || ch[0] == '\t';
    case TOK_EOL:
    case TOK_EOL_CONT:
    case TOK_BAD_EOL_CONT:
      return true;
  }
  return false;
}

bool MkTokener::WithWordStartTest()
{
  WithOperators();
  switch ((int)tok)
  {
    case TOK_EOL:
    case TOK_EOL_CONT:
    case TOK_BAD_EOL_CONT:
    case TOK_COLON:
    case TOK_DOUBLE_COLON:
      return true;
    case TOK_CHAR:
      if (ch[0] == ' ' || ch[0] == '\t')
        return true;
  }
  return SetTok(TOK_WORD_START, 0);
}

bool MkTokener::WithWordStartNoOps()
{
  WithOperators();
  switch ((int)tok)
  {
    case TOK_EOL:
    case TOK_EOL_CONT:
    case TOK_BAD_EOL_CONT:
      return true;
    case TOK_ASSIGN_OP:
    case TOK_COLON:
    case TOK_DOUBLE_COLON:
      tok = TOK_SKIP;
      return true;
    case TOK_CHAR:
      if (ch[0] == ' ' || ch[0] == '\t')
        return true;
  }
  return SetTok(TOK_WORD_START, 0);
}

bool MkTokener::WithPreProcTest()
{
  WithWordStartTest();
  if (tok == TOK_WORD_START && ch[0] == '!' && ent->Size() == 0)
    return SetTok(TOK_PREPROC_START, 1);

  return true;
}

bool MkTokener::WithWordEndTest()
{
  WithOperators();

  /* When scanning inside a word normally WORD_END will be indicated by white
   * space or EOL. Anything else will resume the word.
   * Exceptions are:
   * ASSIGN_OP should end a current non empty word that will become a VARIABLE.
   * The OP itself will will be inserted to a sibling word.
   * COLON should be passed without WORD_END. The COLON itself will end the
   * word without allowing it to become a directive.
   */
  switch ((int)tok)
  {
    case TOK_ASSIGN_OP:
      if (ent->Size() == 0)
        return true;
      break;
    case TOK_COLON:
    case TOK_DOUBLE_COLON:
      if (pType != MK_DIRECTIVE_LINE)
        return true;
      break;
    default:
      if (!EndOfWord())
        return true;
  }
  return SetTok(TOK_WORD_END, 0);
}

bool MkTokener::WithKeywordEndTest()
{
  Default();

  if (EndOfWord())
    return SetTok(TOK_KEYWORD_END, 0);

  if (tok != TOK_CHAR)
    return true;

  switch (ch[0])
  {
    case ')':
    case '}':
      return SetTok(TOK_KEYWORD_END, 0);
  }
  return true;
}

bool MkTokener::WithCommentChars()
{
  if (ch[0] == '\n' || ch[0] == '\r' || ch[0] == '\\' || ch[0] == '\0')
    return Default();

  char tmpCh;
  int i = 0;
  do
    {
      i++;
      tmpCh = ctx->styler.SafeGetCharAt(currPos + i, '\n');
    }
  while (tmpCh != '\n' && tmpCh != '\r' && tmpCh != '\\');
  return SetTok(TOK_SKIP, i);
}

bool MkTokener::Default()
{
  if (HighPrioTokenTest())
    return true;
  return SetTok(TOK_CHAR, 1);
}

bool MkTokener::WithOperators()
{
  if (HighPrioTokenTest())
    return true;
  if (OpTokenTest())
    return true;
  return SetTok(TOK_CHAR, 1);
}

bool MkTokener::WithRecipeStart()
{
  Default();
  switch ((int)tok)
  {
    case TOK_EOL:
      return true;
    case TOK_CHAR:
      if (ch[0] == ' ' || ch[0] == '\t')
        return true;
  }
  return SetTok(TOK_RECIPE_START, 0);
}

/* Initialize array of tokening methods per make entity type */
bool (MkTokener::* MkTokener::tokenerMethods[MK_LAST])() = {
NULL, /* for MK_TOP */
#define TypE_func(NAME,FUNC) &MkTokener::FUNC,
MAKE_TYPES(TypE_func)
};

ILexer *LexerMake::LexerFactoryMake()
{
  return new LexerMake;
}

ILexer *lexamples_create_make_lexer()
{
  return LexerMake::LexerFactoryMake();
}

const char *LexerMake::wordListDesc[] = { NULL };

