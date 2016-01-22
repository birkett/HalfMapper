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
 * \param eLevel    Log level.
 * \param szMessage String to print.
 */
void CLogConsoleEndPoint::WriteMessage(const LogLevel &eLevel, const std::string &szMessage)
{
	this->SetColour(eLevel);
	std::cout << szMessage << std::endl;
	this->ResetColour();

#if defined(_MSC_VER) || defined(__MINGW32__)
	#ifdef _DEBUG
		OutputDebugString(szMessage.c_str());
		OutputDebugString("\n");
	#endif
#endif

}//end CLogConsoleEndPoint::WriteMessage()


/**
 * Set the colour of a message in the console.
 * \param eLevel Log level of the message.
 */
void CLogConsoleEndPoint::SetColour(const LogLevel &eLevel)
{

// On Windows, use SetConsoleTextAttribute().
#if defined(_MSC_VER) || defined(__MINGW32__)
	WORD attributes = FOREGROUND_INTENSITY;

	switch (eLevel)
	{
	case E_DEBUG:
		// Debug messages use the default colour.
		break;
	case E_INFO:
		// Blue for info.
		attributes |= FOREGROUND_BLUE;
		break;
	case E_WARNING:
		// Yellow for warnings.
		attributes |= FOREGROUND_RED | FOREGROUND_GREEN;
		break;
	case E_ERROR:
		// Red for errors.
		attributes |= FOREGROUND_RED;
		break;
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attributes);

// On Linux and MacOSX, use the bash modifiers.
#elif defined(__APPLE__) || defined(__linux__)
	switch (eLevel)
	{
	case E_DEBUG:
		// Light grey.
		std::cout << "\x1b[37;1m";
		break;
	case E_INFO:
		// Blue.
		std::cout << "\x1b[34;1m";
		break;
	case E_WARNING:
		// Yellow.
		std::cout << "\x1b[33;1m";
		break;
	case E_ERROR:
		// Red.
		std::cout << "\x1b[31;1m";
		break;
	}
#endif

}//end CLogConsoleEndPoint::SetColour()


/**
 * Reset the console back to default colours.
 */
void CLogConsoleEndPoint::ResetColour()
{
// On Windows, use SetConsoleTextAttribute().
#if defined(_MSC_VER) || defined(__MINGW32__)
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);

// On Linux and MacOSX, use the bash modifiers.
#elif defined(__APPLE__) || defined(__linux__)
	std::cout << "\x1b[0m";
#endif

}//end CLogConsoleEndPoint:ResetColour()
