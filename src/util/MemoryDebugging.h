/*
 * halfmapper, a renderer for GoldSrc maps and chapters.
 *
 * Copyright(C) 2014  Gonzalo Ávila "gzalo" Alterach
 * Copyright(C) 2015  Anthony "birkett" Birkett
 *
 * This file is part of halfmapper.
 *
 * This program is free software; you can redistribute it and / or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 */
#ifndef MEMORYDEBUGGING_H
#define MEMORYDEBUGGING_H

// Define this symbol for all builds by default. Overridden below.
#define PrintMemoryLeaks()

// MSVCRT memory debugging.
#ifdef _MSC_VER
	#ifdef _DEBUG
		#ifdef USE_CRT_LD
			#define _CRTDBG_MAP_ALLOC
			#include <stdlib.h>
			#include <crtdbg.h>

			// When running the MSVCRT memory leak detection, redefine new.
			//   This allows for more detailed output from the leak detector,
			//   including allocation file name and line number.
			#ifndef DBG_NEW
				#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
				#define new DBG_NEW
			#endif //DBG_NEW

			#undef PrintMemoryLeaks
			#define PrintMemoryLeaks() _CrtDumpMemoryLeaks()
		#endif //USE_CRT_LD

		#ifdef USE_VLD
			#include <vld.h>
		#endif //USE_VLD
	#endif //_DEBUG
#endif //_MSC_VER

#endif //MEMORYDEBUGGING_H
