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
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "logger/ILogEndPoint.h"

/**
 * Log levels for specifying message types.
 */
enum LogLevel
{
	E_DEBUG,
	E_INFO,
	E_WARNING,
	E_ERROR,
};//end LogLevels


/**
 * Log messages to attached end points.
 */
class Logger
{
public:
	/** Get the singleton instance. */
	static Logger* GetInstance();

	/** Destructor. */
	~Logger();

	/**
	 * Add a message to the log, dispatching to all attached end points.
	 * This is a variadic template. Implimented in this header to avoid link errors.
	 * \param eLevel    Log level (debug, info, warning or error).
	 * \param firstItem The first item to print.
	 * \param moreItems Variable number of items left to print.
	 */
	template <typename FirstItem, typename... MoreItems>
	void AddMessage(const LogLevel &eLevel, const FirstItem &szMessage, const MoreItems&... moreArgs)
	{
		this->BufferString(this->GetTime(), this->GetMessageType(eLevel), szMessage, moreArgs...);
		this->SendToEndPoints(this->m_szTempString.str());
		this->m_szTempString.clear();

	}//end AddMessage()

	void RegisterEndPoint(ILogEndPoint* endpoint);

private:
	/** Constructor. Private for singleton. */
	Logger();

	/**
	 * Get the message type string from a given LogLevel.
	 * \param eLevel The level value to get a string for.
	 */
	std::string GetMessageType(const LogLevel &eLevel);

	/** Get the current time, for timestamping messages. */
	std::string GetTime();

	/**
	 * Print a message to the console.
	 * This is a variadic template, and is overloaded to accept multiple values to print.
	 * Implimented in this header to avoid link errors.
	 * \param singleItem A single item to print, or when called recursivly, the last item.
	 */
	template <typename SingleItem>
	void BufferString(const SingleItem& singleItem)
	{
		this->m_szTempString << singleItem; // Print a single item.

	}//end ConsolePrint()


	/**
	 * Print a message to the console.
	 * This is a variadic template, and is overloaded to accept multiple values to print.
	 * It will call itself, or ConsolePrint(singleItem) depending on how many elements are left to print.
	 * Implimented in this header to avoid link errors.
	 * \param firstItem The first item to print.
	 * \param moreItems Variable number of items left to print.
	 */
	template <typename FirstItem, typename... MoreItems>
	void BufferString(const FirstItem& firstItem, const MoreItems&... moreItems)
	{
		this->m_szTempString << firstItem << " "; // Print the first item.
		BufferString(moreItems...);               // Recurse to print the rest.

	}//end ConsolePrint()

	void SendToEndPoints(const std::string &szMessage);

	std::vector<ILogEndPoint*> m_RegisteredEndPoints; /** Store pointers to registered log end points. */
	std::stringstream          m_szTempString;        /** Temporary storage of the output string, before being send to end points. */

};//end Logger

#endif //LOGGER_H
