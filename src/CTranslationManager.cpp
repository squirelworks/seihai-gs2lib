/*
 * include/CTranslationManager.cpp
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

#include "CTranslationManager.h"

// -- Function: Translate String -- //
const char * CTranslationManager::translate(const std::string& pLanguage, const std::string& pKey)
{
	if (mTranslationList[pLanguage][pKey].empty())
	{
		this->add(pLanguage, pKey, "");
		return pKey.c_str();
	}

	return mTranslationList[pLanguage][pKey].c_str();
}

// -- Function: Add Translation -- //
void CTranslationManager::add(const std::string& pLanguage, const std::string& pKey, const std::string& pTranslation)
{
	mTranslationList[pLanguage][pKey] = pTranslation;
}

// -- Function: Remove Translation -- //
void CTranslationManager::remove(const std::string& pLanguage, const std::string& pKey)
{
	mTranslationList[pLanguage].erase(pKey);
}

// -- Function: Reset Language -- //
void CTranslationManager::reset(const std::string& pLanguage)
{
	mTranslationList[pLanguage].clear();
}

// -- Function: Reset All Languages -- //
void CTranslationManager::reset()
{
	// Clear Languages..?
	for (std::map<std::string, STRMAP>::iterator i = mTranslationList.begin(); i != mTranslationList.end(); ++i)
		(*i).second.clear();

	/* -- unsure if this would be enough to free the sub-maps -- */
	// mTranslationList.clear();
}
