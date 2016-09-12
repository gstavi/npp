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

#ifndef LEXCOMMON_H
#define LEXCOMMON_H

#include <assert.h>
#include <string>
#include "ILexer.h"
#include "Scintilla.h"
#include "LexAccessor.h"
#include "WordList.h"

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class LexerEntity;

class LexerCommon : public ILexer
{
public:
  LexerCommon();
  virtual ~LexerCommon();

  int SCI_METHOD Version() const;
  void SCI_METHOD Release();
  const char * SCI_METHOD PropertyNames();
  int SCI_METHOD PropertyType(const char *name);
  const char * SCI_METHOD DescribeProperty(const char *name);
  int SCI_METHOD PropertySet(const char *key, const char *val);
  const char * SCI_METHOD DescribeWordListSets();
  int SCI_METHOD WordListSet(int n, const char *wl);
  void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
  void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
  void * SCI_METHOD PrivateCall(int operation, void *pointer);

  class PosBank;
  class DoLexContext;

  virtual void DoLex(DoLexContext *ctx);
  virtual void DoFold(DoLexContext *ctx);
  virtual void PropertyUpdateNotification(void *PropPtr, int PropOffset);

protected:

  struct Property
  {
    const char *name;
    const char *desc;
    int offset;
    int type;
    Property *next;

    static int TypeOfProp(bool *) { return SC_TYPE_BOOLEAN; }
    static int TypeOfProp(int *) { return SC_TYPE_INTEGER; }
    static int TypeOfProp(std::string *) { return SC_TYPE_STRING; }
  };

  Property *FindProp(const char *propName) const;

  Property *propList;
  Property **lastProp;
  char *propNames;
};

#define PROP_OFFSET(T,F) (\
  (char *)&reinterpret_cast<T *>(0x1000)->F -\
  (char *)&reinterpret_cast<T *>(0x1000)->LexerCommon::propList)

#define DefinePropertyCommonLexer(NAME, F, DESC) \
  *lastProp = new LexerCommon::Property; \
  if (*lastProp != NULL) { \
    (*lastProp)->name = NAME; \
    (*lastProp)->desc = DESC; \
    (*lastProp)->offset = (int)((char *)(&this->F) - (char *)&this->LexerCommon::propList); \
    (*lastProp)->type = LexerCommon::Property::TypeOfProp(&this->F); \
    (*lastProp)->next = NULL; \
    lastProp = &((*lastProp)->next);}

/* Context for single 'Lexing' call for a specific document.
 * Allocated on stack. */
class LexerCommon::DoLexContext
{
public:
  DoLexContext(IDocument *pAccess):
    styler(pAccess) {};

  /* Move Lexing starting point. Typically used if lexing was requested at the
   * middle of a multi line logical block (like comment) and we must start from
   * the beginning. */
  void StartAt(int pos);
  /* Set the specific style ;chAttr' from 'current position' (remembered by\
   * scintilla) to position 'pos'. */
  void ColourTo(int pos, int chAttr);
  /* Test if current entry within the lexing tree refers to a specific string. */
  bool IsEqual(const LexerEntity *ent, const char *str);
  bool IsEqualLowered(const LexerEntity *ent, const char *str);

  LexAccessor styler;
  LexerCommon *lexer;
  void *userPtr;
  /* Position within document of the first character that should be Lexed. */
  int startPos;
  /* Number of characters that should be Lexed. Used to stop Lexing at the end
   * of the visible region. */
  int length;
  /* Flag that indicates that we finished Lexing all requested characters. */
  bool doneStyling;
};

/* Maintain an array of 'idle' positions which are safe to start Lexing at.
 * Typically the beginnings of lines following the end of some logical block.
 * Any 'Find' (lookup) for a position flush all the position after it assuming
 * it will be followed by Lexing that modifies idle points anyway. */
class LexerCommon::PosBank
{
public:
  PosBank(int inMinDistance = 256);
  ~PosBank();
  void Add(int newPos);
  int Find(int posAfter);

private:
  int *posArr;
  /* Number of valid positions within 'posArr'. */
  int used;
  /* Number of allocated entries within 'posArr'. */
  int allocSize;
  /* Minimal distance between 2 postions in bank so small bank could cover large
   * file. */
  int minDistance;
};

class LexerEntity
{
  public:
  LexerEntity(int type_, int pos_);
  virtual ~LexerEntity();
  void DeleteChildren();

  /* The Lc prefix is for LexCommon. So an inheriting class can define similar
   * methods without the Lc that call these and return the actual class */
  LexerEntity *LcCreateChild(int childType, int childSize = 0, int childPos = -1);
  LexerEntity *LcCreateSibling(int sibType, int sibSize = 0, int sibPos = -1);
  LexerEntity *LcFinishElement(int endPos);

  void AddChild(LexerEntity *child);
  void AddSibling(LexerEntity *sib);
  /* Walk the LexerEntity tree and set styles. Release all child entries but not
  * the root */
  void SetStyles(LexerCommon::DoLexContext *ctx);

  int GetStr(LexAccessor *styler, char *strBuff, int buffSize) const;
  int GetStrLowered(LexAccessor *styler, char *strBuff, int buffSize) const;
  bool InWordList(LexAccessor *styler, const WordList *wl) const;
  bool InWordListLowered(LexAccessor *styler, const WordList *wl) const;

  int Type() const { return this != NULL ? type : 0;}
  int ParentType()  const { return parent->Type();}
  int NextType()    const { return nextSib->Type();}
  int PrevType()    const { return prevSib->Type();}
  int FirstChType() const { return firstChild->Type();}
  int LastChType()  const { return lastChild->Type();}
  int Size()        const { return this != NULL ? size : 0;}
  int Pos()         const { return this != NULL ? pos  : 0;}
  void SetType(int newType) { type = newType; }
private:
  /* Get style based on 'type' and parent/sibling entities */
  virtual int GetStyle(LexerCommon::DoLexContext *ctx) const;
  /* Create another entity of the same derivetive class */
  virtual LexerEntity *Create(int newType, int newPos) const;
  /* Can be used to initialize fields in inheriting class after parent and
   * sibling pointers are set */
  virtual void PostCreate() {};

protected:
  LexerEntity *parent;
  LexerEntity *firstChild;
  LexerEntity *lastChild;
  LexerEntity *nextSib;
  LexerEntity *prevSib;

  int pos;
  int size;
  int type;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif /* LEXCOMMON_H */
