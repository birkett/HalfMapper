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
#ifndef ILOGENDPOINT_H
#define ILOGENDPOINT_H

#include <string>

#define abstract_class class


/**
 * Interface for log end points.
 */
abstract_class ILogEndPoint
{
public:
	/** Constructor. */
	ILogEndPoint() {};

	/** Destructor. */
	virtual ~ILogEndPoint() {}

	/**
	 * Print messages using an end point.
	 * \param szMessage String to send.
	 */
	virtual void WriteMessage(const std::string &szMessage) = 0;

};//end ILogEndPoint

#endif //ILOGENDPOINT_H
