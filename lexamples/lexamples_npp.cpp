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
#include "lexamples.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "resource.h"

#include <tchar.h>
#include <PluginInterface.h>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

static HWND NppHndl = NULL;

static void LexamplesAbout();

BOOL APIENTRY DllMain(HINSTANCE, DWORD, LPVOID)
{
  return TRUE;
}

extern "C" void setInfo(NppData notpadPlusData)
{
  NppHndl = notpadPlusData._nppHandle;
}

extern "C" const TCHAR * getName()
{
  return _T("lexamples");
}

extern "C" FuncItem *getFuncsArray(int *nbF)
{
  static struct FuncItem LexamplesFuncs[] = {
    {_T("About"),      LexamplesAbout, 0, false, NULL}};

  *nbF = ARRAY_SIZE(LexamplesFuncs);
  return LexamplesFuncs;
}

extern "C" void beNotified(SCNotification *)
{
}

extern "C" LRESULT messageProc(UINT, WPARAM, LPARAM)
{
  return TRUE;
}

extern "C" BOOL isUnicode() {
  return TRUE;
}

typedef Scintilla::ILexer4 *(LexerFactoryFunc)();

struct {
  const char *LexerName;
  const TCHAR *desc;
  LexerFactoryFunc *factory_func;
} lexamples_lexer_arr[] = {
  { "Make", TEXT("Makefile (lexamples plugin)"), lexamples_create_make_lexer},
  { "Mib",  TEXT("MIB/ASN.1 (lexamples plugin)"),  lexamples_create_mib_lexer}
};

extern "C" int __stdcall GetLexerCount()
{
  return ARRAY_SIZE(lexamples_lexer_arr);
}

extern "C" void __stdcall GetLexerName(unsigned int Index, char *name, int buflength)
{
  if (Index < ARRAY_SIZE(lexamples_lexer_arr))
    ::strncpy(name, lexamples_lexer_arr[Index].LexerName, buflength);
}

extern "C" void __stdcall GetLexerStatusText(unsigned int Index, TCHAR *desc, int buflength)
{
  if (Index < ARRAY_SIZE(lexamples_lexer_arr))
    ::_tcsncpy(desc, lexamples_lexer_arr[Index].desc, buflength);
}

extern "C" LexerFactoryFunc * __stdcall GetLexerFactory(unsigned int Index)
{
  return Index < ARRAY_SIZE(lexamples_lexer_arr) ?
    lexamples_lexer_arr[Index].factory_func : NULL;
}

void LexamplesAbout()
{
  static TCHAR AboutText[] =
    TEXT("lexamples - syntax highlighting lexer package\n")
    TEXT("Makefile lexer\n")
    TEXT("MIB/ASN.1 lexer\n\n")
    TEXT("Version: ") TEXT(VERSION_LEXAMPLES) TEXT("\n\n")
    TEXT("Copyright 2014 by Gur Stavi\n")
    TEXT("gur.stavi@gmail.com");

  if (NppHndl != NULL)
  {
    ::MessageBox(NppHndl, AboutText, TEXT("About lexamples"), MB_OK);
  }
}
