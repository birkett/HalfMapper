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
#include <memory>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "Logger.h"


/**
 * Get the singleton instance.
 */
Logger* Logger::GetInstance()
{
	static std::auto_ptr<Logger> instance(new Logger());
	return instance.get();

}//end Logger::GetInstance()


/**
 * Destructor.
 */
Logger::~Logger()
{

}//end Logger::~Logger()


/**
 * Constructor. Private for singleton.
 */
Logger::Logger()
{

}//end Logger::Logger()


/**
 * Get the message type string from a given LogLevel.
 * \param eLevel The level value to get a string for.
 */
std::string Logger::GetMessageType(const LogLevel &eLevel)
{
	std::string szLevel;

	switch (eLevel) {
	case E_DEBUG:
		szLevel = "[DEBUG]";   break;
	case E_INFO:
		szLevel = "[INFO]";    break;
	case E_WARNING:
		szLevel = "[WARNING]"; break;
	case E_ERROR:
		szLevel = "[ERROR]";   break;
	}

	return szLevel;

}//end Logger::GetMessageType()


/**
 * Get the current time, for timestamping messages.
 */
std::string Logger::GetTime()
{
	std::time_t t = std::time(nullptr);
	std::tm tm    = *std::localtime(&t);

	std::stringstream time;
	time << "[" << std::put_time(&tm, "%T") << "]";

	return time.str();

}//end GetTime()


/**
 * Add a new end point to the registered list.
 * \param endpoint Pointer to the end point.
 */
void Logger::RegisterEndPoint(ILogEndPoint* endpoint)
{
	this->m_RegisteredEndPoints.push_back(endpoint);

}//end Logger::RegisterEndPoint()


/**
 * Dispatch a message to all registered end points.
 * \param szMessage String to send.
 */
void Logger::SendToEndPoints(const std::string &szMessage)
{
	for (size_t i = 0; i < this->m_RegisteredEndPoints.size(); i++) {
		this->m_RegisteredEndPoints[i]->WriteMessage(szMessage);
	}

}//end Logger::SendToEndPoints()
