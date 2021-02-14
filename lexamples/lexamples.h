/*  Copyright 2019, Gur Stavi, gur.stavi@gmail.com  */

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

#ifndef LEXAMPLES_H
#define LEXAMPLES_H

namespace Scintilla {
    class ILexer4;
};

extern "C" Scintilla::ILexer4 *lexamples_create_make_lexer();
extern "C" Scintilla::ILexer4 *lexamples_create_mib_lexer();

#endif /* LEXAMPLES_H */
