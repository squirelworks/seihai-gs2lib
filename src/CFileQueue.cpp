/*
 * include/CFileQueue.cpp
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

#include <queue>

#include <CFileQueue.h>
#include <IEnums.h>
#include <IUtil.h>

void CFileQueue::addPacket(const CString& packet)
{
    // Check for empty or invalid packets.
    if (packet.isEmpty() || static_cast<unsigned char>(packet[0]) < 0x20)
        return;

    const auto packetId = static_cast<unsigned char>(packet[0] - 0x20);

    // Certain file related packets go to the file buffer since they must be
    // sent in order.
    switch (packetId)
    {
        case PLO_LARGEFILESTART:
        case PLO_LARGEFILEEND:
        case PLO_LARGEFILESIZE:
            m_fileBuffer.emplace(false, packet);
            break;

        // Everything else goes into the normal buffer.
        default:
            m_normalBuffer.emplace(false, packet);
            break;
    }
}

void CFileQueue::addRawPacket(const CString& packet)
{
    // Check for empty or invalid packets.
    if (packet.isEmpty() || static_cast<unsigned char>(packet[0]) < 0x20)
        return;

    // Get the length of the raw packet.
    // Make sure we add space for the newline, if required.
    int rawPacketLength = packet.length();
    if (m_outputCodec.getGen() <= ENCRYPT_GEN_5)
        rawPacketLength += 1;

    auto& buffer = (static_cast<unsigned char>(packet[0] - 0x20) == PLO_BOARDPACKET) ? m_normalBuffer : m_fileBuffer;
    buffer.emplace(true, packet);
}

void CFileQueue::clearBuffers()
{
    std::queue<std::pair<bool, CString>>().swap(m_normalBuffer);
    std::queue<std::pair<bool, CString>>().swap(m_fileBuffer);
    m_bytesSentWithoutFile = 0;
    m_sendBuffer.clear();
}

bool CFileQueue::canSend() const
{
    if (!m_normalBuffer.empty() || !m_fileBuffer.empty())
        return true;
    return false;
}

void CFileQueue::sendCompress(const bool forceSendFiles)
{
    // 0xF000 - potential PLO_RAWDATA size - potential newline.
    constexpr int MAX_PACKET_SIZE = 0xF000 - 5 - 1;

    if (m_socket == nullptr || m_socket->getState() == SOCKET_STATE_DISCONNECTED)
        return;

    auto addPacket = [this](const std::pair<bool, CString>& packet)
    {
        // If this is a raw data packet, and we are not gen 6, prepend PLO_RAWDATA to the output.
        if (packet.first == true && m_outputCodec.getGen() <= ENCRYPT_GEN_5)
            m_scratchBuffer >> (char)PLO_RAWDATA >> (int)(packet.second.length() + 1) << '\n';

        // Output the packet.
        m_scratchBuffer << packet.second;

        // Add the newline.
        if (m_outputCodec.getGen() <= ENCRYPT_GEN_5)
            m_scratchBuffer << '\n';
    };

    m_scratchBuffer.clear();

    // If the next normal packet is huge, lets "try" to send it.
    // Everything else should skip because this may throw it way over the limit.
    if (!m_normalBuffer.empty() && m_normalBuffer.front().second.length() > MAX_PACKET_SIZE)
    {
        addPacket(m_normalBuffer.front());
        m_normalBuffer.pop();
    }

    // If we haven't sent a file in a while, forcibly send one now.
    if (m_scratchBuffer.isEmpty() && (m_bytesSentWithoutFile > 0x7FFF || forceSendFiles || m_sendCallsWithoutData >= 4) && !m_fileBuffer.empty()) // 32KB
    {
        m_bytesSentWithoutFile = 0;
        addPacket(m_fileBuffer.front());
        m_fileBuffer.pop();
    }

    // Keep adding packets from normalBuffer until we hit 48KB or we run out of packets.
    while (m_scratchBuffer.length() < 0xC000 && !m_normalBuffer.empty()) // 48KB
    {
        // If the next packet sticks us over 60KB, don't add it.
        if (m_scratchBuffer.length() + m_normalBuffer.front().second.length() > MAX_PACKET_SIZE)
            break;

        addPacket(m_normalBuffer.front());
        m_normalBuffer.pop();
    }
    m_bytesSentWithoutFile += m_scratchBuffer.length();

    // If we have less than 16KB of data, try to add a file.
    if (m_scratchBuffer.length() < 0x4000 && !m_fileBuffer.empty()) // 16KB
    {
        // If the next packet sticks us over 60KB, don't add it.
        if (m_scratchBuffer.length() + m_fileBuffer.front().second.length() <= MAX_PACKET_SIZE)
        {
            m_bytesSentWithoutFile = 0;
            addPacket(m_fileBuffer.front());
            m_fileBuffer.pop();
        }
    }

    // Reset this if we have no files to send.
    if (m_fileBuffer.empty())
        m_bytesSentWithoutFile = 0;

    // If we have no data, just return.
    if (m_scratchBuffer.isEmpty())
    {
        if (m_sendCallsWithoutData < 5)
            m_sendCallsWithoutData++;
        return;
    }
    m_sendCallsWithoutData = 0;

    // compress buffer
    switch (m_outputCodec.getGen())
    {
        // Just send whatever is in the buffer.
        default:
        case ENCRYPT_GEN_1:
        case ENCRYPT_GEN_6:
        {
            m_sendBuffer << m_scratchBuffer;

            if (m_socket->webSocket)
                webSocketFixOutgoingPacket(m_sendBuffer);

            // Send.
            unsigned int dsize = m_sendBuffer.length();
            m_sendBuffer.removeI(0, m_socket->sendData(m_sendBuffer.text(), &dsize));
            break;
        }

        case ENCRYPT_GEN_2:
        case ENCRYPT_GEN_3:
        {
            // Compress the packet.
            m_scratchBuffer.zcompressI();

            // Sanity check.
            if (m_scratchBuffer.length() > 0xFFFD)
            {
                printf("** [ERROR] Trying to send a GEN_3 packet over 65533 bytes!  Tossing data.\n");
                return;
            }

            // Add the packet to the out buffer.
            m_sendBuffer << (short)m_scratchBuffer.length() << m_scratchBuffer;

            if (m_socket->webSocket)
                webSocketFixOutgoingPacket(m_sendBuffer);

            // Send.
            unsigned int dsize = m_sendBuffer.length();
            m_sendBuffer.removeI(0, m_socket->sendData(m_sendBuffer.text(), &dsize));
            break;
        }

        case ENCRYPT_GEN_4:
        {
            m_scratchBuffer.bzcompressI();

            // Sanity check.
            if (m_scratchBuffer.length() > 0xFFFD)
            {
                printf("** [ERROR] Trying to send a GEN_4 packet over 65533 bytes!  Tossing data.\n");
                return;
            }

            // Encrypt the packet and add it to the out buffer.
            m_outputCodec.limitFromType(COMPRESS_BZ2);
            m_scratchBuffer = m_outputCodec.encrypt(m_scratchBuffer);
            m_sendBuffer << (short)m_scratchBuffer.length() << m_scratchBuffer;

            if (m_socket->webSocket)
                webSocketFixOutgoingPacket(m_sendBuffer);

            // Send.
            unsigned int dsize = m_sendBuffer.length();
            m_sendBuffer.removeI(0, m_socket->sendData(m_sendBuffer.text(), &dsize));
            break;
        }

        case ENCRYPT_GEN_5:
        {
            // unsigned int oldSize = pSend.length();

            // Choose which compression to use and apply it.
            int compressionType = COMPRESS_UNCOMPRESSED;
            if (m_scratchBuffer.length() > 0x2000) // 8KB
            {
                compressionType = COMPRESS_BZ2;
                m_scratchBuffer.bzcompressI();
            }
            else if (m_scratchBuffer.length() > 55)
            {
                compressionType = COMPRESS_ZLIB;
                m_scratchBuffer.zcompressI();
            }

            // unsigned int newSize = oSend.length();
            // printf("Compression [%s] - old size: %ld, new size: %ld\n", (compressionType == COMPRESS_UNCOMPRESSED ?
            // "uncompressed" : (compressionType == COMPRESS_ZLIB ? "zlib" : "bz2")), oldSize, newSize);

            // Sanity check.
            if (m_scratchBuffer.length() > 0xFFFC)
            {
                printf("** [ERROR] Trying to send a GEN_5 packet over 65532 bytes!  Tossing data.\n");
                return;
            }

            // Encrypt the packet and add it to the out buffer.
            m_outputCodec.limitFromType(compressionType);
            m_scratchBuffer = m_outputCodec.encrypt(m_scratchBuffer);
            m_sendBuffer << (short)(m_scratchBuffer.length() + 1) << (char)compressionType << m_scratchBuffer;

            if (m_socket->webSocket)
                webSocketFixOutgoingPacket(m_sendBuffer);

            // Send.
            unsigned int dsize = m_sendBuffer.length();
            m_sendBuffer.removeI(0, m_socket->sendData(m_sendBuffer.text(), &dsize));
            break;
        }
    }
}
