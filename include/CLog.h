/*
 * include/CLog.h
 *
 * Copyright 2007-2019 OpenGraal
 *
 * This file is part of GS2Emu.
 *
 * GS2Emu is free software : you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GS2Emu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GS2Emu. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CLOG_H
#define CLOG_H

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include "CString.h"

// Typedef for the callback function pointer
typedef void (*StringPrintCallback)(const char*);

//! Logger class for logging information to a file.
class CLog
{
	public:
		//! Creates a new CLog that outputs to the specific file.
		//! \param file The file to log to.
		//! \param enabled If the class logs by default to the file or not.
		CLog();
		CLog(const CString& _file, bool _enabled = true);

		//! Cleans up and closes files.
		virtual ~CLog();

		//! Outputs a text string to a file.
		/*!
			Outputs text to a file.  Use like a standard printf() command.
			\param format Format string.
		*/
		void out(const CString format, ...);
		void append(const CString format, ...);

		//! Clears the output file.
		void clear();

		//! Closes the file.
		void close();

		//! Opens the file.
		void open();

		//! Gets the enabled state of the class.
		//! \return True if logging is enabled.
		bool getEnabled() const;

		//! Sets the enabled state of the logger class.
		/*!
			Sets the enabled state of the logger class.
			If /a enabled is set to false, it will no longer log to a file.
			\param enabled If true, log to file.  If false, don't log to file.
		*/
		void setEnabled(bool enabled);

		//! Sets the enabled state of timestamps for cli in the logger class.
		/*!
			Sets the enabled state of timestamps for cli in the logger class.
			If /a enabled is set to false, it will no longer log timestamps to cli.
			\param enabled If true, show timestamps in cli.  If false, don't show timestamps in cli.
		*/
		void setTimeStampsInCliEnabled(bool enabled);

		//! Sets the enabled state of logs for cli in the logger class.
		/*!
			Sets the enabled state of logs for cli in the logger class.
			If /a enabled is set to false, it will no longer log to cli.
			\param enabled If true, show logs in cli.  If false, don't show logs in cli.
		*/
		void setLogToCliEnabled(bool enabled);

		//! Gets the name of the log file.
		//! \return Name of the log file.
		const CString& getFilename() const;

		//! Sets the name of the file to write to.
		//! \param filename Name of the file to write to.
		void setFilename(const CString& filename);

		static StringPrintCallback external_log_function;

	private:
		//! If the class is enabled or not.
		bool enabled;

		bool timeStampsInCliEnabled = true;
		bool logToCliEnabled = true;

		//! Filename to write to.
		CString filename;

		//! Application home path.
		CString homepath;

		//! File handle.
		FILE* file;

		//! Mutex
		std::recursive_mutex* m_write;
};

inline
bool CLog::getEnabled() const
{
	return enabled;
}

inline
const CString& CLog::getFilename() const
{
	return filename;
}

inline
void CLog::setEnabled(bool enabled)
{
	this->enabled = enabled;
}

inline
void CLog::setTimeStampsInCliEnabled(bool enabled)
{
	this->timeStampsInCliEnabled = enabled;
}

inline
void CLog::setLogToCliEnabled(bool enabled)
{
	this->logToCliEnabled = enabled;
}

#endif
