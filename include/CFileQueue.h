// Modified by the Seihai project, 2026-10-31: see CHANGES-SEIHAI.md.
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

#include <cstdint>
#include <queue>

#include <CEncryption.h>
#include <CSocket.h>
#include <CString.h>

class CFileQueue
{
public:
    explicit CFileQueue(CSocket* pSock) : m_socket(pSock), m_bytesSentWithoutFile(0) {}

    void addPacket(const CString& packet);
    void addRawPacket(const CString& packet);

    void clearBuffers();

    void setSocket(CSocket* pSock);
    void setCodec(uint32_t gen, uint8_t key);
    void setGen5Steps(Gen5Steps steps) noexcept { m_outputCodec.setGen5Steps(steps); } // Seihai

    [[nodiscard]] bool canSend() const;
    void sendCompress(bool forceSendFiles = false);

private:
    CSocket* m_socket;
    std::queue<std::pair<bool, CString>> m_normalBuffer;
    std::queue<std::pair<bool, CString>> m_fileBuffer;
    CString m_scratchBuffer;
    CString m_sendBuffer;

    CEncryption m_outputCodec;

    int m_bytesSentWithoutFile;
    int m_sendCallsWithoutData = 0;
};

inline void CFileQueue::setSocket(CSocket* pSock)
{
    m_socket = pSock;
}

inline void CFileQueue::setCodec(const uint32_t gen, const uint8_t key)
{
    m_outputCodec.setGen(gen);
    m_outputCodec.reset(key);
}

#endif
