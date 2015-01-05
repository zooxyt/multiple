/* Version Number Information
   Copyright(C) 2013-2014 Cheryl Natsu

   This file is part of multiple - Multiple Paradigm Language Emulator

   multiple is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   multiple is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
   */

#ifndef _MULTIPLE_VERSION_H_
#define _MULTIPLE_VERSION_H_

#define _MULTIPLE_VERSION_ "0.2.134"
#define _MULTIPLE_MARK_ "alpha"

#ifndef COMPILER
#if defined(__GNUC__)
#define COMPILER "GCC "__VERSION__
#elif defined(_MSC_VER)
#define COMPILER "Microsoft C/C++ Compiler " TO_LITERAL_(_MSC_VER)
#define TO_LITERAL_(text) #text
#else
#define COMPILER "Unknown Compiler"
#endif
#endif

#endif

