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
#ifndef COMMONTYPES_H
#define COMMONTYPES_H

/**
 * Represents a 3D point in space, using 3 float values.
 */
struct Point3f
{
	/** Constructor. */
	Point3f()
	{
		m_fX = m_fY = m_fZ = 0.0f;
	}

	float m_fX; /** X postion. */
	float m_fY; /** Y postition. */
	float m_fZ; /** Z position. */

};//end Point3f


/**
 * Represents a 2D point, using 2 floats.
 */
struct Point2f
{
	/** Constructor. */
	Point2f()
	{
		m_fX = m_fY = 0.0f;
	}

	float m_fX; /** X postion. */
	float m_fY; /** Y postition. */

};//end Point2f

#endif //COMMONTYPES_H