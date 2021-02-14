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

#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

LexerCommon::LexerCommon()
{
  propList = NULL;
  lastProp = &propList;
  propNames = NULL;
}

LexerCommon::~LexerCommon()
{
  while (propList != NULL)
  {
    Property *tmp = propList;
    propList = propList->next;
    delete tmp;
  }
  if (propNames != NULL)
  {
    ::free(propNames);
  }
}

int LexerCommon::Version() const
{
  return lvRelease4;
}

void LexerCommon::Release()
{
  delete this;
}

const char *LexerCommon::PropertyNames()
{
  unsigned int n, offset;
  Property *p;

  if (propNames != NULL)
    return propNames;

  n = 1; /* For '\0' */
  for (p = propList; p != NULL; p = p->next)
    n += (int)::strlen(p->name) + 1;

  propNames = (char *)::malloc(n);
  if (propNames == NULL)
    return NULL;

  offset = 0;
  for (p = propList; p != NULL; p = p->next)
  {
    n = (unsigned)::strlen(p->name);
    ::memcpy(propNames + offset, p->name, n);
    offset += n;
    propNames[offset++] = '\n';
  }
  propNames[offset] = '\0';
  return propNames;
}

int LexerCommon::PropertyType(const char *name)
{
  Property *p = FindProp(name);
  return p != NULL ? p->type : SC_TYPE_BOOLEAN;
}

const char *LexerCommon::DescribeProperty(const char *name)
{
  Property *p = FindProp(name);
  return p != NULL ? p->desc : NULL;
}

int LexerCommon::PropertySet(const char *key, const char *val)
{
  char *base = (char *)&this->propList;
  Property *p = FindProp(key);

  if (p == NULL)
    return -1;

  switch (p->type)
  {
    case SC_TYPE_BOOLEAN:
    {
      bool *b = (bool *)(base + p->offset);
      bool new_b = ::atoi(val) != 0;
      if (*b == new_b)
        return 0;
      *b = new_b;
      break;
    }
    case SC_TYPE_INTEGER:
    {
      int *i = (int *)(base + p->offset);
      int new_i = ::atoi(val);
      if (*i == new_i)
        return 0;
      *i = new_i;
      break;
    }
    case SC_TYPE_STRING:
    {
      std::string *str = (std::string *)(base + p->offset);
      if (str->compare(val) == 0)
        return 0;
      *str = val;
      break;
    }
    default:
      return -1;
  }
  PropertyUpdateNotification(base + p->offset, p->offset);
  return 0;
}

const char *LexerCommon::DescribeWordListSets()
{
  return NULL;
}

int LexerCommon::WordListSet(int, const char *)
{
  return -1;
}

void LexerCommon::Lex(unsigned int inStartPos, int lengthDoc, int,
  IDocument *pAccess)
{
  LexerCommon::DoLexContext ctx(pAccess);

  ctx.lexer = this;
  ctx.userPtr = NULL;
  ctx.startPos = (int)inStartPos;
  ctx.length = (int)lengthDoc;
  ctx.doneStyling = false;

  DoLex(&ctx);

  ctx.styler.Flush();
}

void LexerCommon::Fold(unsigned int inStartPos, int lengthDoc, int,
  IDocument *pAccess)
{
  LexerCommon::DoLexContext ctx(pAccess);

  ctx.lexer = this;
  ctx.startPos = (int)inStartPos;
  ctx.length = (int)lengthDoc;
  ctx.doneStyling = false;

  DoFold(&ctx);

  ctx.styler.Flush();
}

void *LexerCommon::PrivateCall(int, void *)
{
  return NULL;
}

/* Default implementation for DoLex. Typically overrided by actual Lexer so this
 * method is not expected to be called. */
void LexerCommon::DoLex(DoLexContext *ctx)
{
  int endPos = ctx->startPos + ctx->length - 1;
  ctx->StartAt(ctx->startPos);
  ctx->ColourTo(endPos, 0);
}

/* Default implementation for DoFold. Typically overrided by actual Lexer so
 * this method is not expected to be called. */
void LexerCommon::DoFold(DoLexContext *)
{
}

void LexerCommon::PropertyUpdateNotification(void *, int)
{
}

LexerCommon::Property *LexerCommon::FindProp(const char *propName) const
{
  Property *p;
  for (p = propList; p != NULL; p = p->next)
  {
    if (::strcmp(p->name, propName) == 0)
      return p;
  }
  return NULL;
}

void LexerCommon::DoLexContext::StartAt(int pos)
{
  if (pos >= startPos + length)
  {
    startPos = startPos + length;
    length = 0;
    return;
  }

  length = startPos + length - pos;
  startPos = pos;
  styler.StartAt((unsigned)pos);
  styler.StartSegment((unsigned)pos);
}

void LexerCommon::DoLexContext::ColourTo(int pos, int chAttr)
{
  if (pos < startPos || doneStyling)
    return;

  if (pos + 1 >= startPos + length)
  {
    pos = startPos + length - 1;
    doneStyling = true;
  }
  styler.ColourTo((unsigned)pos, chAttr);
}

bool LexerCommon::DoLexContext::IsEqual(const LexerEntity *ent,
  const char *str)
{
  char entStr[128];
  ent->GetStr(&styler, entStr, sizeof(entStr));
  return ::strcmp(entStr, str) == 0;
}

bool LexerCommon::DoLexContext::IsEqualLowered(const LexerEntity *ent,
  const char *str)
{
  char entStr[128];
  ent->GetStrLowered(&styler, entStr, sizeof(entStr));
  return ::strcmp(entStr, str) == 0;
}

LexerEntity::LexerEntity(int type_, int pos_)
{
  parent = NULL;
  firstChild = NULL;
  lastChild = NULL;
  nextSib = NULL;
  prevSib = NULL;
  pos = pos_;
  size = 0;
  type = type_;
}

void LexerEntity::DeleteChildren()
{
  while (firstChild != NULL)
  {
    LexerEntity *tmp = firstChild;
    firstChild = firstChild->nextSib;
    tmp->parent = NULL;
    delete tmp;
  }
}

LexerEntity::~LexerEntity()
{
  if (parent != NULL)
  {
    if (prevSib != NULL)
      prevSib->nextSib = nextSib;
    else
      parent->firstChild = nextSib;
    if (nextSib != NULL)
      nextSib->prevSib = prevSib;
    else
      parent->lastChild = prevSib;
  }
  DeleteChildren();
}

int LexerEntity::GetStyle(LexerCommon::DoLexContext *) const
{
  return 0;
}

LexerEntity *LexerEntity::Create(int newType, int newPos) const
{
  return new LexerEntity(newType, newPos);
}

void LexerEntity::AddChild(LexerEntity *child)
{
  child->parent = this;
  child->prevSib = lastChild;

  if (firstChild == NULL)
    firstChild = child;
  else
    lastChild->nextSib = child;

  lastChild = child;
}

void LexerEntity::AddSibling(LexerEntity *sib)
{
  parent->AddChild(sib);
}

LexerEntity *LexerEntity::LcCreateChild(int childType, int childSize, int childPos)
{
  if (childPos < 0)
    childPos = pos + size;

  LexerEntity *newEnt = Create(childType, childPos);
  if (newEnt == NULL)
    return NULL;

  newEnt->size = childSize;
  AddChild(newEnt);
  newEnt->PostCreate();
  return newEnt;
}

LexerEntity *LexerEntity::LcCreateSibling(int sibType, int sibSize, int sibPos)
{
  if (sibPos < 0)
    sibPos = pos + size;
  else
    size = sibPos - pos;

  if (parent != NULL)
    return parent->LcCreateChild(sibType, sibSize, sibPos);

  return NULL;
}

LexerEntity *LexerEntity::LcFinishElement(int endPos)
{
  size = endPos - pos;
  parent->size = endPos - parent->pos;
  return parent;
}

void LexerEntity::SetStyles(LexerCommon::DoLexContext *ctx)
{
  LexerEntity *child = firstChild;
  int basePos = pos;

  firstChild = NULL;
  lastChild = NULL;

  while (child != NULL)
  {
    LexerEntity *tmp;

    if (pos < child->pos)
      ctx->ColourTo(child->pos - 1, GetStyle(ctx));

    child->SetStyles(ctx);
    pos = child->pos;

    tmp = child;
    child = child->nextSib;
    delete tmp;
  }

  if (pos < basePos + size)
  {
    ctx->ColourTo(basePos + size - 1, GetStyle(ctx));
    pos = basePos + size;
  }

  size = 0;
}

int LexerEntity::GetStr(LexAccessor *styler, char *strBuff, int buffSize) const
{
  int i = 0;

  if (this != NULL && size + 1 <= buffSize)
  {
    for (; i < size; i++)
      strBuff[i] = (*styler)[pos + i];
  }
  strBuff[i] = '\0';
  return i ;
}

int LexerEntity::GetStrLowered(LexAccessor *styler, char *strBuff, int buffSize) const
{
  int i = 0;

  if (this != NULL && size + 1 <= buffSize)
  {
    for (; i < size; i++)
      strBuff[i] = (char)::tolower((*styler)[pos + i]);
  }
  strBuff[i] = '\0';
  return i;
}

bool LexerEntity::InWordList(LexAccessor *styler, const WordList *wl) const
{
  char str[128];

  GetStr(styler, str, sizeof(str));
  return wl->InList(str);
}

bool LexerEntity::InWordListLowered(LexAccessor *styler, const WordList *wl) const
{
  char str[128];

  GetStrLowered(styler, str, sizeof(str));
  return wl->InList(str);
}

LexerCommon::PosBank::PosBank(int inMinDistance)
{
  minDistance = inMinDistance > 1 ? inMinDistance : 1;
  used = 0;
  allocSize = 128;
  posArr = (int *)::malloc(allocSize * sizeof(int));
  if (posArr != NULL)
    posArr[used++] = 0;
  else
    allocSize = 0;
}

LexerCommon::PosBank::~PosBank()
{
  if (posArr != NULL)
  {
    ::free(posArr);
    posArr = NULL;
    allocSize = 0;
    used = 0;
  }
}

void LexerCommon::PosBank::Add(int newPos)
{
  if (posArr == NULL || newPos < posArr[used-1] + minDistance)
    return;
  if (used == allocSize)
  {
    int newSize = allocSize * 2;
    int *newArr = (int *)::realloc(posArr, newSize * sizeof(int));
    if (newArr == NULL)
      return;

    allocSize = newSize;
    posArr = newArr;
  }
  posArr[used++] = newPos;
}

int LexerCommon::PosBank::Find(int posAfter)
{
  int from = 0;
  int count = used;

  if (posArr == NULL)
    return 0;

  /* Binary search */
  while (count > 1)
  {
    int idx = count / 2;
    if (posArr[from + idx] > posAfter)
    {
      count = idx;
    }
    else
    {
      count -= idx;
      from += idx;
    }
  }
  /* Clear all positions after */
  used = from + 1;
  return posArr[from];
}

