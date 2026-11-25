// Modified by the Seihai project, 2026-10-31, 2026-11-12, 2026-11-25: see CHANGES-SEIHAI.md.
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
#include <deque>

#include <CEncryption.h>
#include <CSocket.h>
#include <CString.h>

enum class PacketQueue
{
    NORMAL,
    FILE
};

class CFileQueue
{
public:
    explicit CFileQueue(CSocket* pSock) : m_socket(pSock), m_bytesSentWithoutFile(0) {}

    void addPacket(PacketQueue queue, const CString& packet);
    void addRawPacket(PacketQueue queue, const CString& packet);

    void moveInProgressFileTransferToNormalQueue();

    void clearBuffers();

    void setSocket(CSocket* pSock);
    void setCodec(EncryptionGeneration generation, uint8_t key);
    void setCodec(uint32_t gen, uint8_t key) { setCodec(static_cast<EncryptionGeneration>(std::min(gen, static_cast<uint32_t>(EncryptionGeneration::GEN_6))), key); } // Seihai: old interface
    void setGen5Steps(Gen5Steps steps) noexcept { m_outputCodec.setGen5Steps(steps); } // Seihai
    void setAllowPartialIteration(bool allow) noexcept { m_outputCodec.allowPartialIteration = allow; } // Seihai

    [[nodiscard]] bool canSend() const;
    void sendCompress(bool forceSendFiles = false);

private:
    CSocket* m_socket;
    std::deque<std::pair<bool, CString>> m_normalBuffer;
    std::deque<std::pair<bool, CString>> m_fileBuffer;
    CString m_scratchBuffer;
    CString m_sendBuffer;

    CEncryption m_outputCodec;

    int m_bytesSentWithoutFile;
    int m_sendCallsWithoutData = 0;
    bool m_currentlySendingLargeFile = false;
};

inline void CFileQueue::setSocket(CSocket* pSock)
{
    m_socket = pSock;
}

inline void CFileQueue::setCodec(const EncryptionGeneration generation, const uint8_t key)
{
    m_outputCodec.setGeneration(generation);
    m_outputCodec.reset(key);
}

#endif
