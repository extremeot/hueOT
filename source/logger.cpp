//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Logger class - captures everything that happens on the server
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "logger.h"
#include "tools.h"
#include <ctime>

Logger::Logger()
{
	m_file.open("logs/server.txt", std::ios::app);
	if(m_file.good())
		m_registering = true;

	m_playermessages.open("logs/players.txt", std::ios::app);
	if (m_playermessages.good())
		m_registering = true;
}

Logger::~Logger()
{
	if(m_registering){
		m_file.close();
		m_playermessages.close();
	}
}

void Logger::logPlayerMessage(const char* title, std::string message, const char* func)
{
	//check if the file is open, if not, avoid writting to file
	if (!m_registering){
		return;
	}

	//write timestamp of the event
	char buffer[32];
	time_t now = std::time(NULL);
	formatDate(now, buffer);
	m_playermessages << buffer;

	//write channel generating the message
	if (title){
		m_playermessages << " [" << title << "] ";
	}

	//write the message
	m_playermessages << " " << message << std::endl;

	m_playermessages.flush();
}

void Logger::logMessage(const char* channel, std::string message, const char* func)
{
	//check if the file is open, if not, avoid writting to file
	if(!m_registering){
		return;
	}

	//write timestamp of the event
	char buffer[32];
	time_t now = std::time(NULL);
	formatDate(now, buffer);
	m_file << buffer;

	//write channel generating the message
	if(channel){
		m_file << " [" << channel << "] ";
	}

	//write the message
	m_file << " " << message << std::endl;

	m_file.flush();
}
