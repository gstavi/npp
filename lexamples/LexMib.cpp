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
#include "lexamples.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "LexAccessor.h"
#include "WordList.h"

#include "LexCommon.h"
#include <memory.h>
#include <string.h>
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

class LexerMib;
class MibEntity;
class MibTokener;

/* Define an enum of entity types and assign a tokening method to each */
#define MIB_TYPES \
  TypE(MIB_WORD,               Default) \
  TypE(MIB_OBJECT_WORD,        Default) \
  TypE(MIB_TABLE_OBJECT,       Default) \
  TypE(MIB_IDENTIFIER,         Default) \
  TypE(MIB_ENUM_NAME,          Default) \
  TypE(MIB_TYPE_WORD,          Default) \
  TypE(MIB_ERROR,              Default) \
  TypE(MIB_OID_BASE_WORD,      Default) \
  TypE(MIB_NUMBER,             Default) \
  TypE(MIB_STRING,             InString) \
  TypE(MIB_KEYWORD,            Default) \
  TypE(MIB_SEQUENCE_OF,        Default) \
  TypE(MIB_TYPE,               Default) \
  TypE(MIB_ATTRIBUTE,          Default) \
  TypE(MIB_IMPORT_SRC,         Default) \
  TypE(MIB_USER_TYPE,          Default) \
  TypE(MIB_OP,                 Default) \
  TypE(MIB_BRACE_EXPR,         InsideBraces) \
  TypE(MIB_INTEGER_BRACE_EXPR, InsideIntegerBraces) \
  TypE(MIB_BRACE_ELEMENT,      InsideBraceElement) \
  TypE(MIB_OID_EXPR,           InsideOid) \
  TypE(MIB_PARENTHESES_EXPR,   InsideParentheses) \
  TypE(MIB_IMP_EXP,            InsideImpExp) \
  TypE(MIB_MACRO_NAME,         Default) \
  TypE(MIB_COMMENT,            Default)

enum MibType {
MIB_TOP = 0,
#define TypE(NAME,FUNC) NAME,
MIB_TYPES
#undef TypE
MIB_LAST
};

typedef enum {
  TOK_NULL = 0,
  TOK_CHAR,
  TOK_SKIP_CHAR,
  TOK_STRING_END_ERROR,
  TOK_EOL,
  TOK_COMMENT,
  TOK_STRING_START,
  TOK_STRING_END,
  TOK_PARENTHESES_START,
  TOK_PARENTHESES_END,
  TOK_BRACE_EXPR_START,
  TOK_BRACE_EXPR_END,
  TOK_NEW_BRACE_ELEMENT,
  TOK_OP,
  TOK_WORD,
  TOK_WORD_IN_IMP_EXP,
  TOK_WORD_IN_PARENTHESES,
  TOK_ENUM_NAME,
  TOK_OID_BASE_WORD,
  TOK_NUMBER,
  TOK_NUMBER_ERROR,
  TOK_END_IMPORTS
} MibToken;

class LexerMib : public LexerCommon
{
public:
  LexerMib();
  virtual ~LexerMib() {}

  void PropertyUpdateNotification(void *PropPtr, int PropOffset);
  void DoLex(DoLexContext *ctx);

  PosBank idlePosBank;
  WordList keywords;
  WordList mibTypes;
  WordList attributes;
  WordList objDeclarationKeywords;

  /* Configurable properties */
  std::string extraKeywordsStrings;
  int stringTerminationHelperMode;
  bool fold;
  bool highlightTableObj;

  static Scintilla::ILexer4 *LexerFactoryMib();
  static const char *wordListDesc[];

  static const char keywordsStrings[];
  static const char objDeclarationKeywordsStrings[];
  static const char standardTypesStrings[];
  static const char attributesStrings[];
};

#define TEST_ENT_ALLOC(e) if ((e) == NULL) return NULL

class MibEntity : public LexerEntity
{
  public:
  MibEntity(int type, int pos);
  ~MibEntity() {};

  MibEntity *CreateSibling(int sibType, int sibSize=0, int sibPos=-1) {
    return static_cast<MibEntity *>(LcCreateSibling(sibType, sibSize, sibPos));}
  MibEntity *CreateChild(int childType, int childSize=0, int childPos=-1) {
    return static_cast<MibEntity *>(LcCreateChild(childType, childSize, childPos));}
  MibEntity *FinishElement(int endPos) {
    return static_cast<MibEntity *>(LcFinishElement(endPos));}

  MibEntity *SetAsIdentifier(LexerCommon::DoLexContext *ctx);
  MibEntity *SetAsEnumName(LexerCommon::DoLexContext *ctx);
  MibEntity *SetAsTypeWord(LexerCommon::DoLexContext *ctx);
  MibEntity *PrevNotComment();

private:
  LexerEntity *Create(int type, int pos) const;
  int GetStyle(LexerCommon::DoLexContext *ctx) const;

public:
  int wordCount;

  friend class MibTokener;
};

#define FOLD_BRACE_HEADER   (1U << 0)
#define FOLD_STR_HEADER     (1U << 1)
#define FOLD_COMMENT_HEADER (1U << 2)
#define FOLD_KEYWORD_HEADER (1U << 3)
#define FOLD_IMP_EXP_HEADER (1U << 4)

/* Inidicate an expectation for additional element that may appear on the next
 * line. Flush should be held off since the new element may need to observe the
 * previous entities. */
#define HOLD_OFF_BRACE         (1U<<0)
#define HOLD_OFF_STR           (1U<<1)
#define HOLD_OFF_SINGLE_WORD   (1U<<2)
#define HOLD_OFF_ASSIGN        (1U<<3)
#define HOLD_OFF_ANY_WORD      (1U<<4)
#define HOLD_OFF_SYNTAX_TYPE   (1U<<5)

class MibTokener
{
public:
  MibTokener(LexerCommon::DoLexContext *inCtx, int inPos);
  ~MibTokener();
  void Reset();
  void SetNextToken();
  void ProcessingLoop();
  MibEntity *FlushStyles();
  void SetFoldLevel();
  bool SetTok(MibToken inTok, int inTokSize) {
    tokSize = inTokSize;
    tok = inTok;
    return true;
  }
  int GetFoldLevelAtPrevLine();
  MibEntity *ProcessEolToken();

  MibEntity *ProcessToken();
  MibEntity *ProcessWordToken();
  MibEntity *ProcessKeywordToken(MibEntity *word, const char *keywordStr);
  MibEntity *ProcessWord_SEQUENCE(MibEntity *word);
  MibEntity *ProcessWord_TEXTUAL_CONVENTION(MibEntity *word);
  void ProcessWord_MACRO(MibEntity *word);

  /* Auxilary methods to detect a subset of tokens
   * Return true if a token was set */
  bool EolTest(int pos);
  bool CommentTest();
  bool WordTest();
  bool NumberTest();
  bool HexNumberTest();
  bool BinaryNumberTest();
  bool OpTest();

  /* These will always find a token, at least TOK_CHAR. They return bool to
   * allow the use of: return SetTok() */
  bool Default();
  bool InsideBraces();
  bool InsideIntegerBraces();
  bool InsideBraceElement();
  bool InsideParentheses();
  bool InsideOid();
  bool InsideImpExp();
  bool InString();

  /* Array of tokenning method per entity type. Contains the methods above  */
  static bool (MibTokener::* tokenerMethods[MIB_LAST])();

  LexerCommon::DoLexContext *ctx;
  MibEntity top;
  LexerMib *lexer;
  MibEntity *ent;
  const char *terminateSectionStr;
  MibEntity *lastObjWord;
  MibToken tok;
  int tokSize;
  int type;
  int lineNumber;
  int currFoldLevel;
  int nextFoldLevel;
  int savedFoldLevel;
  int foldHeaderFlags;
  int currPos;
  int nextPos;
  int docLength;
  int holdOffFlushFlags;
  int prevHoldOff;
  int commentLineCount;
  bool atEndOfDoc;
  bool isEmptyLine;
  bool isCommentLine;
  char ch[4];
};

const char LexerMib::objDeclarationKeywordsStrings[] =
  "AGENT-CAPABILITIES MODULE-COMPLIANCE MODULE-IDENTITY NOTIFICATION-GROUP "
  "NOTIFICATION-TYPE OBJECT-GROUP OBJECT-IDENTITY OBJECT-TYPE "
  "TRAP-TYPE";

const char LexerMib::keywordsStrings[] =
  " " /* Extra space */
  "ACCESS AUGMENTS BEGIN BITS CHOICE CONTACT-INFO DEFVAL DESCRIPTION "
  "DISPLAY-HINT END ENTERPRISE EXPORTS GROUP IDENTIFIER IMPLICIT IMPLIED "
  "IMPORTS INCLUDES INDEX LAST-UPDATED MACRO MANDATORY-GROUPS MAX-ACCESS "
  "MIN-ACCESS MODULE NOTATION OBJECT OBJECTS OF ORGANIZATION PRODUCT-RELEASE "
  "REFERENCE REVISION SEQUENCE SIZE STATUS SUPPORTS SYNTAX TEXTUAL-CONVENTION "
  "TYPE UNITS VALUE VARIABLES VARIATION";

const char LexerMib::standardTypesStrings[] = "Counter Counter32 Counter64 "
  "DisplayString Gauge Gauge32 IDENTIFIER INTEGER Integer32 IpAddress "
  "NetworkAddress NsapAddress OBJECT OCTET Opaque PhysAddress STRING TimeTicks "
  "UNITS Unsigned32 MacAddress TruthValue TestAndIncr AutonomousType "
  "InstancePointer VariablePointer RowPointer RowStatus TimeStamp TimeInterval "
  "DateAndTime StorageType TDomain TAddress";

const char LexerMib::attributesStrings[] = "accessible accessible-for-notify "
  "create current deprecated for mandatory not not-accessible notify "
  "not-implemented obsolete only optional read read-create read-create "
  "read-only read-write write write-only";

LexerMib::LexerMib():
  idlePosBank()
{
  /* Initialize word lists */
  objDeclarationKeywords.Set(objDeclarationKeywordsStrings);
  mibTypes.Set(standardTypesStrings);
  attributes.Set(attributesStrings);
  /* Set keywords word list */
  PropertyUpdateNotification(NULL, PROP_OFFSET(LexerMib, extraKeywordsStrings));

  fold = true;
  highlightTableObj = true;
  stringTerminationHelperMode = 2;

  DefinePropertyCommonLexer("fold", fold, NULL);
  DefinePropertyCommonLexer("lexer.mib.extra.keywords", extraKeywordsStrings,
    "List of additional keywords to highlight.");
  DefinePropertyCommonLexer("lexer.mib.string.termination.helper.mode", stringTerminationHelperMode,
    "Normally every addition/deletion of a quotation mark will change all "
    "strings to non-strings and all non-strings to strings throughout the "
    "file. Values: 0 - no helper, 1 - empty line terminates string, 2 assign "
    "op (::=) terminates string.");
  DefinePropertyCommonLexer("lexer.mib.highlight.table.object", highlightTableObj,
    "Use different highlighting for table objects based on the existence of "
    "SEQUENCE OF.");
}

void LexerMib::PropertyUpdateNotification(void *, int PropOffset)
{
  std::string tmpStr;

  switch (PropOffset)
  {
    case PROP_OFFSET(LexerMib, extraKeywordsStrings):
      tmpStr = objDeclarationKeywordsStrings;
      tmpStr += keywordsStrings;
      if (!extraKeywordsStrings.empty())
      {
        tmpStr += " ";
        tmpStr += extraKeywordsStrings;
      }
      keywords.Set(tmpStr.c_str());
      break;
  }
}

void LexerMib::DoLex(DoLexContext *ctx)
{
  int pos = idlePosBank.Find(ctx->startPos);
  MibTokener tokener(ctx, pos);
  tokener.ProcessingLoop();
}

MibEntity::MibEntity(int type, int pos):
  LexerEntity(type, pos)
{
  wordCount = 0;
}

LexerEntity *MibEntity::Create(int ent_type, int ent_pos) const
{
  return new MibEntity(ent_type, ent_pos);
}

int MibEntity::GetStyle(LexerCommon::DoLexContext *ctx) const
{
  MibTokener *tokener = static_cast<MibTokener *>(ctx->userPtr);

  switch (type)
  {
    case MIB_COMMENT:
      return SCE_ASN1_COMMENT;
    case MIB_STRING:
      return SCE_ASN1_STRING;
    case MIB_OID_BASE_WORD:
      return SCE_ASN1_OID_BASE;
    case MIB_NUMBER:
      return SCE_ASN1_SCALAR;
    case MIB_TABLE_OBJECT:
      if (tokener->lexer->highlightTableObj)
        return SCE_ASN1_TABLE_NAME;
    case MIB_OBJECT_WORD:
    case MIB_IDENTIFIER:
      return SCE_ASN1_IDENTIFIER;
    case MIB_ENUM_NAME:
      return SCE_ASN1_ENUM_NAME;
    case MIB_TYPE_WORD:
      return SCE_ASN1_DEFAULT;
    case MIB_ERROR:
      return SCE_ASN1_ERROR;
    case MIB_SEQUENCE_OF:
    case MIB_KEYWORD:
      return SCE_ASN1_KEYWORD;
    case MIB_TYPE:
      return SCE_ASN1_TYPE;
    case MIB_ATTRIBUTE:
      return SCE_ASN1_ATTRIBUTE;
    case MIB_OP:
      return SCE_ASN1_OPERATOR;
    case MIB_OID_EXPR:
      return SCE_ASN1_OID;
    case MIB_IMPORT_SRC:
      return SCE_ASN1_IMPORT_SRC;
    case MIB_MACRO_NAME:
      return SCE_ASN1_MACRO_NAME;
  }
  return SCE_ASN1_DEFAULT;
}

MibEntity *MibEntity::PrevNotComment()
{
  MibEntity *ent = this;

  if (ent == NULL)
    return NULL;

  while (ent->prevSib != NULL)
  {
    ent = static_cast<MibEntity *>(ent->prevSib);
    if (ent->type != MIB_COMMENT)
      break;
  }
  return ent;
}

MibEntity *MibEntity::SetAsIdentifier(LexerCommon::DoLexContext *ctx)
{
  char firstCh = ctx->styler.SafeGetCharAt(pos, '\0');
  type = MIB_IDENTIFIER;
  if (firstCh >= 'A' && firstCh <= 'Z')
  {
    MibEntity *errEnt;
    errEnt = CreateChild(MIB_ERROR, 1, pos);
    TEST_ENT_ALLOC(errEnt);
  }
  return this;
}

MibEntity *MibEntity::SetAsEnumName(LexerCommon::DoLexContext *ctx)
{
  char firstCh = ctx->styler.SafeGetCharAt(pos, '\0');
  type = MIB_ENUM_NAME;
  if (firstCh >= 'A' && firstCh <= 'Z')
  {
    MibEntity *errEnt;
    errEnt = CreateChild(MIB_ERROR, 1, pos);
    TEST_ENT_ALLOC(errEnt);
  }
  return this;
}

MibEntity *MibEntity::SetAsTypeWord(LexerCommon::DoLexContext *ctx)
{
  MibTokener *tokener = static_cast<MibTokener *>(ctx->userPtr);
  char firstCh = ctx->styler.SafeGetCharAt(pos, '\0');

  if (InWordList(&tokener->ctx->styler, &tokener->lexer->mibTypes))
    type = MIB_TYPE;
  else
    type = MIB_TYPE_WORD;

  if (firstCh >= 'a' && firstCh <= 'z')
  {
    MibEntity *errEnt;
    errEnt = CreateChild(MIB_ERROR, 1, pos);
    TEST_ENT_ALLOC(errEnt);
  }
  return this;
}

MibTokener::MibTokener(LexerCommon::DoLexContext *inCtx, int inPos):
  top(MIB_TOP, inPos)
{
  ctx = inCtx;
  ctx->userPtr = this;
  lexer = static_cast<LexerMib *>(ctx->lexer);
  currPos = inPos;
  docLength = ctx->styler.Length();
  Reset();
}

MibTokener::~MibTokener()
{
}

void MibTokener::SetNextToken()
{
  type = ent->Type();
  ch[0] = ctx->styler.SafeGetCharAt(currPos, '\0');
  ch[1] = ctx->styler.SafeGetCharAt(currPos + 1, '\0');
  ch[2] = ctx->styler.SafeGetCharAt(currPos + 2, '\0');

  (this->*tokenerMethods[type])();
  nextPos = currPos + tokSize;
}

void MibTokener::Reset()
{
  top.DeleteChildren();
  ent = &top;
  terminateSectionStr = NULL;
  lastObjWord = NULL;
  tok = TOK_NULL;
  tokSize = 0;
  type = MIB_TOP;
  lineNumber = ctx->styler.GetLine(currPos);;
  currFoldLevel = 0;
  nextFoldLevel = 0;
  savedFoldLevel = 0;
  foldHeaderFlags = 0;
  nextPos = 0;
  holdOffFlushFlags = 0;
  prevHoldOff = 0;
  commentLineCount = 0;
  atEndOfDoc = false;
  isEmptyLine = true;
  isCommentLine = true;
  ::memset(ch, 0 , sizeof(ch));
}

void MibTokener::ProcessingLoop()
{
  Reset();
  ctx->StartAt(currPos);
  while (ent != NULL && !ctx->doneStyling)
  {
    SetNextToken();
    ent = ProcessToken();
    /* Update line 'isEmpty' and 'isComment' flags based on last token */
    if (tok == TOK_EOL)
    {
      /* Reset flags for next line */
      isEmptyLine = true;
      isCommentLine = true;
    }
    else if (isEmptyLine || isCommentLine)
    {
      switch ((int)tok)
      {
        case TOK_COMMENT:
          isEmptyLine = false;
          break;
        case TOK_SKIP_CHAR:
        case TOK_CHAR:
          if (ch[0] == ' ' || ch[0] == '\t')
            break;
        default:
          isEmptyLine = false;
          isCommentLine = false;
          commentLineCount = 0;
      }
    }
    currPos = nextPos;
  }
}

MibEntity *MibTokener::FlushStyles()
{
  top.SetStyles(ctx);
  top.wordCount = 0;
  ent = &top;
  lastObjWord = NULL;
  /* Add position immediately after flush to bank so future Lex operations
   * could start from it */
  lexer->idlePosBank.Add(top.Pos());
  return ent;
}

void MibTokener::SetFoldLevel()
{
  if (!lexer->fold)
    return;

  int FoldLevel = currFoldLevel;
  int FoldFlags = SC_FOLDLEVELBASE;

  /* Handle folding of multiple line comment. First line should be a header,
   * next lines should have +1 for their level */
  if (isCommentLine)
  {
    if (commentLineCount > 0)
      FoldLevel = currFoldLevel + 1;
    /* Empty line is considered a comment line but it can't be the header */
    else if (!isEmptyLine)
    {
      foldHeaderFlags |= FOLD_COMMENT_HEADER;
      commentLineCount++;
    }
  }

  if (isEmptyLine)
    FoldFlags |= SC_FOLDLEVELWHITEFLAG;
  else if (foldHeaderFlags)
    FoldFlags |= SC_FOLDLEVELHEADERFLAG;

  ctx->styler.SetLevel(lineNumber, FoldLevel | FoldFlags);
  /* Reset for next line */
  foldHeaderFlags = 0;
}

MibEntity *MibTokener::ProcessEolToken()
{
  SetFoldLevel();

  /* Hold off is extended over comments or empty lines */
  if (isCommentLine)
    holdOffFlushFlags = prevHoldOff;

  currFoldLevel = nextFoldLevel;
  if (atEndOfDoc)
  {
    currFoldLevel = 0;
    holdOffFlushFlags = 0;
  }
  else if (top.wordCount == 1)
  {
    holdOffFlushFlags |= HOLD_OFF_SINGLE_WORD;
  }

  if (currFoldLevel == 0 && holdOffFlushFlags == 0)
    ent = FlushStyles();

  lineNumber++;
  /* Initialize flags for next line */
  prevHoldOff = holdOffFlushFlags;
  holdOffFlushFlags = 0;
  return ent;
}

MibEntity *MibTokener::ProcessKeywordToken(MibEntity *word,
  const char *keywordStr)
{
  MibEntity *prevEnt;

  word->type = MIB_KEYWORD;
  prevEnt = word->PrevNotComment();

  if (lexer->objDeclarationKeywords.InList(keywordStr))
  {
    currFoldLevel = 0;
    nextFoldLevel = 1;
    foldHeaderFlags |= FOLD_KEYWORD_HEADER;
    terminateSectionStr = NULL;
    if (prevEnt->Type() == MIB_WORD)
    {
      prevEnt = prevEnt->SetAsIdentifier(ctx);
      TEST_ENT_ALLOC(prevEnt);
      prevEnt->type = MIB_OBJECT_WORD;
      lastObjWord = prevEnt;
    }
  }
  else if (terminateSectionStr != NULL &&
    ::strcmp(keywordStr, terminateSectionStr) == 0)
  {
    nextFoldLevel = 0;
  }

  if (::strcmp(keywordStr, "SYNTAX") == 0)
  {
    holdOffFlushFlags |= HOLD_OFF_SYNTAX_TYPE;
  }
  else if (::strcmp(keywordStr, "OF") == 0 &&
    ctx->IsEqual(prevEnt, "SEQUENCE"))
  {
    prevEnt->type = MIB_SEQUENCE_OF;
    word->type = MIB_SEQUENCE_OF;
    if (lastObjWord != NULL)
    {
      lastObjWord->type = MIB_TABLE_OBJECT;
      lastObjWord = NULL;
    }
  }
  else if (::strcmp(keywordStr, "IDENTIFIER") == 0 &&
    ctx->IsEqual(prevEnt, "OBJECT"))
  {
    MibEntity *identEnt = prevEnt->PrevNotComment();
    if (identEnt->Type() == MIB_WORD)
    {
      identEnt = identEnt->SetAsIdentifier(ctx);
      TEST_ENT_ALLOC(identEnt);
      currFoldLevel = 0;
      nextFoldLevel = 1;
      foldHeaderFlags |= FOLD_KEYWORD_HEADER;
      terminateSectionStr = NULL;
    }
  }
  else if (::strcmp(keywordStr, "TEXTUAL-CONVENTION") == 0)
  {
    word = ProcessWord_TEXTUAL_CONVENTION(word);
    TEST_ENT_ALLOC(word);
  }
  else if (::strcmp(keywordStr, "SEQUENCE") == 0)
  {
    word = ProcessWord_SEQUENCE(word);
    TEST_ENT_ALLOC(word);
  }
  else if (::strcmp(keywordStr, "IMPORTS") == 0 ||
    ::strcmp(keywordStr, "EXPORTS") == 0)
  {
    savedFoldLevel = nextFoldLevel;
    nextFoldLevel = currFoldLevel + 1;
    foldHeaderFlags |= FOLD_IMP_EXP_HEADER;
    ent = ent->CreateChild(MIB_IMP_EXP, 0, nextPos);
    TEST_ENT_ALLOC(ent);
  }
  else if (::strcmp(keywordStr, "MACRO") == 0)
  {
    ProcessWord_MACRO(word);
  }
  else if (::strcmp(keywordStr, "END") == 0)
  {
    ent = FlushStyles();
  }
  else if (word->ParentType() == MIB_TOP)
  {
    if (::strcmp(keywordStr, "INTEGER") == 0)
    {
      holdOffFlushFlags |= HOLD_OFF_BRACE;
      foldHeaderFlags |= FOLD_KEYWORD_HEADER;
    }
    else if (::strcmp(keywordStr, "DESCRIPTION") == 0)
    {
      holdOffFlushFlags |= HOLD_OFF_STR;
      foldHeaderFlags |= FOLD_KEYWORD_HEADER;
    }
  }
  return ent;
}

MibEntity *MibTokener::ProcessWordToken()
{
  MibEntity *prevEnt, *word;
  char wordStr[64];

  ent->wordCount++;
  word = ent->CreateChild(MIB_WORD, tokSize);
  TEST_ENT_ALLOC(word);
  prevEnt = word->PrevNotComment();
  word->GetStr(&ctx->styler, wordStr, sizeof(wordStr));
  if (lexer->keywords.InList(wordStr))
  {
    ent = ProcessKeywordToken(word, wordStr);
  }
  else if (ctx->IsEqual(prevEnt, "SYNTAX"))
  {
    word = word->SetAsTypeWord(ctx);
    TEST_ENT_ALLOC(word);
    /* Test if SYNTAX was on the same line */
    if (holdOffFlushFlags & HOLD_OFF_SYNTAX_TYPE)
      holdOffFlushFlags &= ~HOLD_OFF_SYNTAX_TYPE;
    else
      currFoldLevel = GetFoldLevelAtPrevLine();
  }
  else if (ent->Type() == MIB_BRACE_ELEMENT)
  {
    switch (ent->wordCount)
    {
      case 1:
        word = word->SetAsIdentifier(ctx);
        TEST_ENT_ALLOC(word);
        break;
      case 2:
        word = word->SetAsTypeWord(ctx);
        TEST_ENT_ALLOC(word);
        break;
      case 3:
        if (strcmp(wordStr, "STRING") == 0 && ctx->IsEqual(prevEnt, "OCTET"))
        {
          word = word->SetAsTypeWord(ctx);
          TEST_ENT_ALLOC(word);
        }
        break;
    }
  }
  else if (word->InWordList(&ctx->styler, &lexer->mibTypes))
  {
    word->type = MIB_TYPE;
  }
  else if (word->InWordList(&ctx->styler, &lexer->attributes))
  {
    word->type = MIB_ATTRIBUTE;
  }
  else if (prevEnt->Type() == MIB_SEQUENCE_OF)
  {
    word = word->SetAsTypeWord(ctx);
    TEST_ENT_ALLOC(word);
  }
  return ent;
}

MibEntity *MibTokener::ProcessToken()
{
  MibEntity *word, *prevEnt, *tmpEnt;

  switch ((int)tok)
  {
    case TOK_BRACE_EXPR_START:
      savedFoldLevel = nextFoldLevel;
      foldHeaderFlags |= FOLD_BRACE_HEADER;
      if (isEmptyLine || isCommentLine)
      {
        currFoldLevel = GetFoldLevelAtPrevLine() + 1;
        nextFoldLevel = currFoldLevel + 1;
      }
      else
      {
        nextFoldLevel = currFoldLevel + 2;
      }

      ent = ent->CreateChild(MIB_BRACE_EXPR);
      TEST_ENT_ALLOC(ent);
      prevEnt = ent->PrevNotComment();
      if (prevEnt->Type() == MIB_OP)
      {
        ent->SetType(MIB_OID_EXPR);
      }
      else if (ctx->IsEqual(prevEnt, "INTEGER"))
      {
        ent->SetType(MIB_INTEGER_BRACE_EXPR);
        break;
      }
      else if (ctx->IsEqual(prevEnt, "SEQUENCE"))
      {
        ent = ent->CreateChild(MIB_BRACE_ELEMENT);
        TEST_ENT_ALLOC(ent);
      }
      break;
    case TOK_NEW_BRACE_ELEMENT:
      ent = ent->FinishElement(currPos);
      ent = ent->CreateChild(MIB_BRACE_ELEMENT, 0, nextPos);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_BRACE_EXPR_END:
      holdOffFlushFlags &= ~(HOLD_OFF_BRACE|HOLD_OFF_ASSIGN);
      if (foldHeaderFlags & FOLD_BRACE_HEADER)
      {
        /* Brace expression started and ended at the same line, don't modify
         * currFoldLevel */
        foldHeaderFlags &= ~FOLD_BRACE_HEADER;
      }
      else
      {
        currFoldLevel = nextFoldLevel - 1;
      }

      if (ent->Type() == MIB_OID_EXPR)
      {
        ent = ent->FinishElement(nextPos);
        ent = FlushStyles();
        nextFoldLevel = 0;
        break;
      }

      if (ent->Type() == MIB_BRACE_ELEMENT)
        ent = ent->FinishElement(currPos);

      nextFoldLevel = savedFoldLevel;
      ent = ent->FinishElement(nextPos);
      break;
    case TOK_PARENTHESES_START:
      ent = ent->CreateChild(MIB_PARENTHESES_EXPR, tokSize);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_PARENTHESES_END:
      ent = ent->FinishElement(nextPos);
      break;
    case TOK_OP:
      holdOffFlushFlags |= HOLD_OFF_ASSIGN;
      word = ent->CreateChild(MIB_OP, tokSize);
      TEST_ENT_ALLOC(word);
      break;
    case TOK_NUMBER:
    case TOK_NUMBER_ERROR:
      word = ent->CreateChild(tok == TOK_NUMBER ? MIB_NUMBER : MIB_ERROR,
        tokSize);
      TEST_ENT_ALLOC(word);
      if (word->PrevType() == MIB_OP)
      {
        ent = FlushStyles();
        nextFoldLevel = 0;
      }
      break;
    case TOK_OID_BASE_WORD:
      ent->wordCount++;
      tmpEnt = ent->CreateChild(MIB_OID_BASE_WORD, tokSize);
      TEST_ENT_ALLOC(tmpEnt);
      break;
    case TOK_WORD_IN_IMP_EXP:
      ent->wordCount++;
      word = ent->CreateChild(MIB_WORD, tokSize);
      TEST_ENT_ALLOC(word);
      prevEnt = word->PrevNotComment();
      if (ctx->IsEqual(prevEnt, "FROM"))
        word->SetType(MIB_IMPORT_SRC);
      break;
    case TOK_WORD_IN_PARENTHESES:
      word = ent->CreateChild(MIB_WORD, tokSize);
      TEST_ENT_ALLOC(word);
      if (word->InWordList(&ctx->styler, &lexer->keywords))
        word->SetType(MIB_KEYWORD);
      break;
    case TOK_ENUM_NAME:
      word = ent->CreateChild(MIB_WORD, tokSize);
      TEST_ENT_ALLOC(word);
      word = word->SetAsEnumName(ctx);
      TEST_ENT_ALLOC(word);
      break;
    case TOK_WORD:
      ent = ProcessWordToken();
      break;
    case TOK_COMMENT:
      tmpEnt = ent->CreateChild(MIB_COMMENT, tokSize);
      TEST_ENT_ALLOC(tmpEnt);
      break;
    case TOK_STRING_START:
      savedFoldLevel = nextFoldLevel;
      foldHeaderFlags |= FOLD_STR_HEADER;
      if (isEmptyLine || isCommentLine)
      {
        currFoldLevel = GetFoldLevelAtPrevLine() + 1;
        nextFoldLevel = currFoldLevel + 1;
      }
      else
      {
        nextFoldLevel = currFoldLevel + 2;
      }
      ent = ent->CreateChild(MIB_STRING,tokSize);
      TEST_ENT_ALLOC(ent);
      break;
    case TOK_STRING_END_ERROR:
      tmpEnt = ent->CreateChild(MIB_ERROR, tokSize);
      TEST_ENT_ALLOC(tmpEnt);
    case TOK_STRING_END:
      holdOffFlushFlags &= ~HOLD_OFF_STR;
      if (foldHeaderFlags & FOLD_STR_HEADER)
      {
        /* String started and ended at the same line, don't modify
         * currFoldLevel */
        foldHeaderFlags &= ~FOLD_STR_HEADER;
      }
      else
      {
        currFoldLevel = nextFoldLevel - 1;
      }
      nextFoldLevel = savedFoldLevel;
      ent = ent->FinishElement(nextPos);
      break;
    case TOK_END_IMPORTS:
      foldHeaderFlags &= ~FOLD_IMP_EXP_HEADER;
      nextFoldLevel = savedFoldLevel;
      ent = ent->FinishElement(nextPos);
      ent = FlushStyles();
      break;
    case TOK_EOL:
      ent->size = nextPos - ent->pos;
      ent = ProcessEolToken();
      break;
  }
  ent->size = nextPos - ent->pos;
  return ent;
}

MibEntity *MibTokener::ProcessWord_SEQUENCE(MibEntity *word)
{
  MibEntity *tmpEnt = word->PrevNotComment();

  if (tmpEnt->Type() != MIB_OP)
    return word;

  currFoldLevel = 0;
  nextFoldLevel = 0;
  holdOffFlushFlags |= HOLD_OFF_BRACE;
  foldHeaderFlags |= FOLD_KEYWORD_HEADER;
  terminateSectionStr = NULL;

  tmpEnt = tmpEnt->PrevNotComment();
  if (tmpEnt->Type() == MIB_WORD)
  {
    tmpEnt = tmpEnt->SetAsTypeWord(ctx);
    TEST_ENT_ALLOC(tmpEnt);
  }
  return word;
}

MibEntity *MibTokener::ProcessWord_TEXTUAL_CONVENTION(MibEntity *word)
{
  MibEntity *tmpEnt = word->PrevNotComment();

  if (tmpEnt->Type() != MIB_OP)
    return word;

  currFoldLevel = 0;
  nextFoldLevel = 1;
  foldHeaderFlags |= FOLD_KEYWORD_HEADER;
  terminateSectionStr = "SYNTAX";

  tmpEnt = tmpEnt->PrevNotComment();
  if (tmpEnt->Type() == MIB_WORD)
  {
    tmpEnt = tmpEnt->SetAsTypeWord(ctx);
    TEST_ENT_ALLOC(tmpEnt);
  }
  return word;
}

void MibTokener::ProcessWord_MACRO(MibEntity *word)
{
  MibEntity *tmpEnt = word->PrevNotComment();

  currFoldLevel = 0;
  nextFoldLevel = 1;
  foldHeaderFlags |= FOLD_KEYWORD_HEADER;
  terminateSectionStr = "END";

  switch (tmpEnt->Type())
  {
    case MIB_WORD:
    case MIB_KEYWORD:
      tmpEnt->SetType(MIB_MACRO_NAME);
  }
}

int MibTokener::GetFoldLevelAtPrevLine()
{
  int level = 0;
  int line = lineNumber;

  while (line > 0)
  {
    line--;
    level = ctx->styler.LevelAt(line);
    if ((level & SC_FOLDLEVELWHITEFLAG) == 0)
      break;
  }
  return level & 0xF;
}

/* Checks at pos for UNIX(\n) Windows(\r\n) or Mac(\r) end of line */
bool MibTokener::EolTest(int pos)
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

bool MibTokener::CommentTest()
{
  int i;
  char tmpCh[2] = {'\0', '\0'};

  if (ch[0] != '-' || ch[1] != '-')
    return false;
  for (i = 2;; i++)
  {
    tmpCh[0] = tmpCh[1];
    tmpCh[1] = ctx->styler.SafeGetCharAt(currPos + i, '\n');

    ch[1] = ctx->styler.SafeGetCharAt(currPos + i, '\n');
    if ( tmpCh[1] == '\r' || tmpCh[1] == '\n')
      return SetTok(TOK_COMMENT, i);
    if (tmpCh[0] == '-' && tmpCh[1] == '-')
      return SetTok(TOK_COMMENT, i+1);
  }
}

bool MibTokener::WordTest()
{
  if (NumberTest())
    return true;

  if (!(ch[0] >= 'a' && ch[0] <= 'z') && !(ch[0] >= 'A' && ch[0] <= 'Z'))
    return false;

  int i = 1;
  while ((ch[1] >= 'a' && ch[1] <= 'z') || (ch[1] >= 'A' && ch[1] <= 'Z') ||
    (ch[1] >= '0' && ch[1] <= '9') || ch[1] == '-')
  {
    i++;
    ch[1] = ctx->styler.SafeGetCharAt(currPos + i, '\0');
  }
  return SetTok(TOK_WORD, i);
}

bool MibTokener::HexNumberTest()
{
  int i;
  char tmpCh;

  if (ch[0] != '\'')
    return false;

  for (i = 1; ; i++)
  {
    tmpCh = ctx->styler.SafeGetCharAt(currPos + i, '\0');
    if (tmpCh == '\'')
      break;
    if (tmpCh >= '0' && tmpCh <= '9')
      continue;
    if (tmpCh >= 'a' && tmpCh <= 'f')
      continue;
    if (tmpCh >= 'A' && tmpCh <= 'F')
      continue;
    return false;
  }

  tmpCh = ctx->styler.SafeGetCharAt(currPos + (++i), '\0');
  if (tmpCh != 'h' && tmpCh != 'H')
    return false;

  /* Test that hex number contains an even number of digits */
  return SetTok((MibToken)(i % 2 == 0 ? TOK_NUMBER : TOK_NUMBER_ERROR), i + 1);
}

bool MibTokener::BinaryNumberTest()
{
  int i;
  char tmpCh;

  if (ch[0] != '\'')
    return false;

  for (i = 1; ; i++)
  {
    tmpCh = ctx->styler.SafeGetCharAt(currPos + i, '\0');
    if (tmpCh == '\'')
      break;
    if (tmpCh == '0' || tmpCh == '1')
      continue;
    return false;
  }

  tmpCh = ctx->styler.SafeGetCharAt(currPos + (++i), '\0');
  if (tmpCh != 'b' && tmpCh != 'B')
    return false;

  /* Test that binary number contains a multiple of 8 digits */
  return SetTok((MibToken)((i - 2) % 8 == 0 ? TOK_NUMBER : TOK_NUMBER_ERROR),
    i + 1);
}

bool MibTokener::NumberTest()
{
  int i;

  if (ch[0] == '\'')
    return HexNumberTest() || BinaryNumberTest() ? true : false;

  if (ch[0] < '0' || ch[0] > '9')
    return false;

  for (i = 1; ch[1] >= '0' && ch[1] <= '9'; /*nothing*/)
    ch[1] = ctx->styler.SafeGetCharAt(currPos + (++i), '\0');

  return SetTok(TOK_NUMBER, i);
}

bool MibTokener::OpTest()
{
  if (ch[0] == ':' && ch[1] == ':' && ch[2] == '=')
    return SetTok(TOK_OP, 3);

  return false;
}

bool MibTokener::Default()
{
  if (EolTest(currPos))
    return true;
  if (CommentTest())
    return true;
  if (WordTest())
    return true;
  if (OpTest())
    return true;
  switch (ch[0])
  {
    case '"':
      return SetTok(TOK_STRING_START, 1);
    case '{':
      return SetTok(TOK_BRACE_EXPR_START, 1);
    case '(':
      return SetTok(TOK_PARENTHESES_START, 1);
  }
  return SetTok(TOK_CHAR, 1);
}

bool MibTokener::InString()
{
  if (ch[0] == '"')
    return SetTok(TOK_STRING_END, 1);
  if (EolTest(currPos))
  {
    if (isEmptyLine && lexer->stringTerminationHelperMode == 1)
      return SetTok(TOK_STRING_END, 0);
    return true;
  }
  if (lexer->stringTerminationHelperMode == 2 && OpTest())
    return SetTok(TOK_STRING_END_ERROR, tokSize);

  return SetTok(TOK_SKIP_CHAR, 1);
}

bool MibTokener::InsideParentheses()
{
  if (EolTest(currPos))
    return SetTok(TOK_PARENTHESES_END, 0);
  if (CommentTest())
    return true;
  if (WordTest())
  {
    if (tok == TOK_WORD)
      return SetTok(TOK_WORD_IN_PARENTHESES, tokSize);
    return true;
  }
  if (ch[0] == ')')
    return SetTok(TOK_PARENTHESES_END, 1);
  return SetTok(TOK_CHAR, 1);
}

bool MibTokener::InsideBraces()
{
  if (EolTest(currPos))
    return true;
  if (CommentTest())
    return true;
  if (WordTest())
    return true;
  if (ch[0] == '}')
    return SetTok(TOK_BRACE_EXPR_END, 1);
  return SetTok(TOK_CHAR, 1);
}

bool MibTokener::InsideIntegerBraces()
{
  InsideBraces();
  if (tok == TOK_WORD)
    return SetTok(TOK_ENUM_NAME,tokSize);
  return true;
}

bool MibTokener::InsideBraceElement()
{
  InsideBraces();
  if (tok == TOK_CHAR && ch[0] == ',')
    return SetTok(TOK_NEW_BRACE_ELEMENT, 1);
  return true;
}

bool MibTokener::InsideOid()
{
  if (EolTest(currPos))
    return true;
  if (CommentTest())
    return true;
  if (WordTest() && tok == TOK_WORD && ent->wordCount == 0)
    return SetTok(TOK_OID_BASE_WORD, tokSize);
  if (ch[0] == '}')
    return SetTok(TOK_BRACE_EXPR_END, 1);
  return SetTok(TOK_CHAR, 1);
}

bool MibTokener::InsideImpExp()
{
  if (EolTest(currPos))
    return true;
  if (CommentTest())
    return true;
  if (WordTest() && tok == TOK_WORD)
    return SetTok(TOK_WORD_IN_IMP_EXP, tokSize);
  if (ch[0] == ';')
    return SetTok(TOK_END_IMPORTS, 1);
  return SetTok(TOK_CHAR, 1);
}

/* Initialize array of tokenning methods per mib entity type */
bool (MibTokener::* MibTokener::tokenerMethods[MIB_LAST])() = {
&MibTokener::Default, /* for MIB_TOP */
#define TypE(NAME,FUNC) &MibTokener::FUNC,
MIB_TYPES
#undef TypE
};

Scintilla::ILexer4 *LexerMib::LexerFactoryMib()
{
  return new LexerMib;
}

Scintilla::ILexer4 *lexamples_create_mib_lexer()
{
  return LexerMib::LexerFactoryMib();
}

const char *LexerMib::wordListDesc[] = { NULL };

