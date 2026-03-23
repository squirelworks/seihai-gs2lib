/*
 * include/CCommon.h
 *
 * Copyright 2019 OpenGraal
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

#ifndef CCOMMON_H
#define CCOMMON_H

#include <stdarg.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include "CString.h"

//! Logger class for logging information to a file.
class CCommon
{
public:
	//! Sets the name of the file to write to.
	//! \param targetX X-position to call the triggeraction.
	//! \param targetY Y-position to call the triggeraction.
	//! \param action
	//! \param weapon
	//! \param args
	static const CString triggerAction(char targetX, char targetY, CString action, CString weapon, CString args);
};
#endif