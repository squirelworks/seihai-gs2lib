/*
 * include/CFileQueue.h
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

#ifndef CFILEQUEUE_H
#define CFILEQUEUE_H

#include <queue>
#include "CString.h"
#include "CSocket.h"
#include "CEncryption.h"

class CFileQueue
{
	public:
		CFileQueue(CSocket* pSock) : sock(pSock), prev100(false), size100(0), bytesSentWithoutFile(0) {}

		void addPacket(CString pPacket);
		void clearBuffers();
		void setSocket(CSocket* pSock)		{ sock = pSock; }
		void setCodec(unsigned int gen, unsigned char key)
		{
			out_codec.setGen(gen);
			out_codec.reset(key);
		}

		bool canSend();

		void sendCompress(bool forceSendFiles = false);

	private:
		CSocket* sock;
		std::queue<CString> normalBuffer;
		std::queue<CString> fileBuffer;
		bool prev100;
		unsigned int size100;
		CString pack100;
		CString oBuffer;

		CEncryption out_codec;

		int bytesSentWithoutFile;
		int sendCallsWithoutData = 0;
};

#endif
