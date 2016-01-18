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
#ifndef CLOGCONSOLEENDPOINT_H
#define CLOGCONSOLENEDPOINT_H

#include "ILogEndPoint.h"

/**
 * Prints messages to the console window.
 */
class CLogConsoleEndPoint : public ILogEndPoint
{
public:
	/** Constructor. */
	CLogConsoleEndPoint();

	/** Destructor. */
	~CLogConsoleEndPoint();

	/**
	 * Print a message to the console.
	 * \param szMessage String to print.
	 */
	void WriteMessage(const std::string &szMessage);

};//end CLogConsoleEndPoint

#endif //CLOGCONSOLENEDPOINT_H
