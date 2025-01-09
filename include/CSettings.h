/*
 * include/CSettings.h
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

#ifndef CSETTINGS_H
#define CSETTINGS_H

#include <vector>
#include <mutex>
#include "CString.h"

struct CKey
{
	CKey(const CString& pName, const CString& pRawValue)
	{
		name  = pName;
		rawvalue = pRawValue;

		int comment_pos = rawvalue.find("#");
		if (comment_pos == -1) {
			value = rawvalue;
			rawvalue.clear();
		}
		else {
			value = rawvalue.remove(comment_pos);
			value.trimI();
			rawvalue.removeI(0, value.length());
		}
	}

	CString name;
	CString value;
	CString rawvalue;
	bool saved = false;
};

class CSettings
{
	public:
		// Constructor-Destructor
		CSettings();
		CSettings(const CString& pStr, const CString& pSeparator = "=");
		~CSettings();

		// File-Loading Functions
		bool isOpened() const;
		bool loadFile(const CString& pStr);
		bool loadSettings(CString &settings, bool fromRC = false, bool save = false);
		CString getSettings();
		bool saveFile();
		void setSeparator(const CString& pSeparator);
		void clear();

		bool exists(const CString& pKey) const;
		CKey* addKey(const CString& pKey, const CString& pValue);

		// Get Type
		CKey *getKey(const CString& pStr);
		const CKey *getKey(const CString& pStr) const;
		bool getBool(const CString& pStr, bool pDefault = true) const;
		float getFloat(const CString& pStr, float pDefault = 1.00) const;
		int getInt(const CString& pStr, int pDefault = 1) const;
		const CString getStr(const CString& pStr, const CString& pDefault = "") const;

		const CString operator[](int pIndex) const;
		std::vector<CString> getFile() const;

	private:
		bool opened;
		CString strSep;
		CString filename;
		std::vector<CKey *> keys;
		std::vector<CString> strList;
		mutable std::recursive_mutex* m_preventChange;
};

#endif // CSETTINGS_H
