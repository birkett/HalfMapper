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
#include <iostream>
#include "CLogConsoleEndPoint.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
	#include <Windows.h>
#endif


/**
 * Constructor.
 */
CLogConsoleEndPoint::CLogConsoleEndPoint()
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	if (AllocConsole()) {
		freopen("CONOUT$", "wt", stdout);
		freopen("CONOUT$", "wt", stderr);
	}
#endif

}//end CLogConsoleEndPoint::CLogConsoleEndPoint()


/**
 * Destructor.
 */
CLogConsoleEndPoint::~CLogConsoleEndPoint()
{

}//end CLogConsoleEndPoint::~CLogConsoleEndPoint()


/**
 * Print a message to the console.
 * \param szMessage String to print.
 */
void CLogConsoleEndPoint::WriteMessage(const std::string &szMessage)
{
	std::cout << szMessage << std::endl;

#if defined(_MSC_VER) || defined(__MINGW32__)
	#ifdef _DEBUG
		OutputDebugString(szMessage.c_str());
		OutputDebugString("\n");
	#endif
#endif

}//end CLogConsoleEndPoint::WriteMessage()
