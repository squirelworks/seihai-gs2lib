// Seihai project, 2026-11-14: see CHANGES-SEIHAI.md.
// GNU General Public License version 3 or later (see LICENSE.md).
// The actual queue/string/codec/util sources are linked with this controlled
// socket transport, excluding CSocket.cpp. No network or production API seam.

#include <CFileQueue.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
struct Transport
{
    int allowance = 1000000;
    bool disconnect = false;
    std::string wire;
    std::vector<std::string> attempts;
} transport;
int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FileQueueSendTest: %s\n", message);
        ++failures;
    }
}

void resetTransport(int allowance = 1000000)
{
    transport = Transport{};
    transport.allowance = allowance;
}

void setup(CFileQueue& queue, uint32_t gen, Gen5Steps rule)
{
    queue.setCodec(gen, 0x4B);
    queue.setGen5Steps(rule);
}

void recoveryCase(uint32_t gen, Gen5Steps rule, int length, int firstWrite)
{
    CSocket socket;
    const CString packetA(std::string(" ") + std::string(length, 'a'));
    const CString packetB(" second");
    resetTransport();
    CFileQueue reference(&socket);
    setup(reference, gen, rule);
    reference.addPacket(PacketQueue::NORMAL, packetA);
    reference.sendCompress();
    const std::string frameA = transport.wire;
    reference.addPacket(PacketQueue::NORMAL, packetB);
    reference.sendCompress();
    const std::string fullReference = transport.wire;

    resetTransport(firstWrite);
    CFileQueue queue(&socket);
    setup(queue, gen, rule);
    queue.addPacket(PacketQueue::NORMAL, packetA);
    queue.sendCompress();
    const auto sent = static_cast<size_t>(std::max(firstWrite, 0));
    check(transport.attempts.size() == 1 && transport.attempts.front() == frameA,
          "initial frame differs from full-send reference");
    check(transport.wire == frameA.substr(0, sent), "initial short-send prefix");
    check(queue.canSend(), "pending encoded tail must remain schedulable");

    // Zero models sendData's EAGAIN contract. Negative is a simulated transport
    // error/contract violation; neither may remove or regenerate pending bytes.
    for (int blocked : {0, -1})
    {
        transport.allowance = blocked;
        const auto calls = transport.attempts.size();
        queue.sendCompress();
        check(transport.attempts.size() == calls + 1, "blocked retry must attempt exactly once");
        if (transport.attempts.size() > calls)
            check(transport.attempts.back() == frameA.substr(sent), "blocked retry changed encoded tail");
        check(transport.wire == frameA.substr(0, sent), "blocked retry changed sent bytes");
        check(queue.canSend(), "blocked retry lost pending scheduling signal");
    }

    // Recover writability without enqueueing anything. The exact remainder must
    // drain once, with no header/cipher regeneration or iterator advance.
    transport.allowance = 1000000;
    auto calls = transport.attempts.size();
    queue.sendCompress();
    check(transport.attempts.size() == calls + 1, "recovery must attempt exactly once");
    check(transport.wire == frameA, "writable recovery did not drain exact frame A");
    check(!queue.canSend(), "drained queue still reports data");
    calls = transport.attempts.size();
    queue.sendCompress();
    check(transport.attempts.size() == calls, "empty queue attempted another send");
    queue.addPacket(PacketQueue::NORMAL, packetB);
    queue.sendCompress();
    check(transport.wire == fullReference, "next frame/cipher state differs after recovery");

    // Also queue B while A is still pending. One call drains A only; B remains
    // schedulable and is encoded on the following call, preserving frame order.
    resetTransport(firstWrite);
    CFileQueue overlapping(&socket);
    setup(overlapping, gen, rule);
    overlapping.addPacket(PacketQueue::NORMAL, packetA);
    overlapping.sendCompress();
    overlapping.addPacket(PacketQueue::NORMAL, packetB);
    transport.allowance = 1000000;
    calls = transport.attempts.size();
    overlapping.sendCompress();
    check(transport.attempts.size() == calls + 1 && transport.wire == frameA,
          "pending-first retry must send only frame A remainder");
    check(overlapping.canSend(), "queued B lost scheduling signal after A drained");
    overlapping.sendCompress();
    check(transport.wire == fullReference, "queued B reordered or regenerated frame A");
    check(!overlapping.canSend(), "fully drained overlapping queue still reports data");
}

void cleanupCases()
{
    CSocket socket;
    resetTransport(3);
    CFileQueue queue(&socket);
    queue.setCodec(ENCRYPT_GEN_1, 0);
    queue.addPacket(PacketQueue::NORMAL, CString(" first"));
    queue.sendCompress();
    socket.setState(SOCKET_STATE_DISCONNECTED);
    const auto calls = transport.attempts.size();
    queue.sendCompress();
    check(transport.attempts.size() == calls, "disconnected socket attempted send");
    queue.addPacket(PacketQueue::FILE, CString(" file"));
    queue.clearBuffers();
    check(!queue.canSend(), "clearBuffers must cancel pending and queued bytes");
    socket.setState(SOCKET_STATE_CONNECTED);
    transport.allowance = 1000000;
    queue.sendCompress();
    check(transport.attempts.size() == calls, "cleared tail sent after reconnect");
    queue.addPacket(PacketQueue::NORMAL, CString(" next"));
    queue.sendCompress();
    check(transport.wire == " fi next\n", "clearBuffers left stale encoded bytes");

    resetTransport(0);
    transport.disconnect = true;
    CFileQueue errorQueue(&socket);
    errorQueue.setCodec(ENCRYPT_GEN_1, 0);
    errorQueue.addPacket(PacketQueue::NORMAL, CString(" error"));
    errorQueue.sendCompress();
    check(transport.attempts.size() == 1, "disconnecting send retried internally");
    errorQueue.sendCompress();
    check(transport.attempts.size() == 1, "retry after disconnect attempted send");
    errorQueue.clearBuffers();
    check(!errorQueue.canSend(), "owner cleanup did not cancel disconnected tail");

    CFileQueue detached(nullptr);
    detached.addPacket(PacketQueue::NORMAL, CString(" detached"));
    detached.sendCompress();
    check(transport.attempts.size() == 1, "null socket attempted send");
}
}

// Link-time transport replacement using the real CSocket declaration/layout.
CSocket::CSocket() : webSocket(false) { properties.state = SOCKET_STATE_CONNECTED; }
CSocket::~CSocket() = default;
int CSocket::setState(int state) { properties.state = state; return SOCKET_OK; }
int CSocket::sendData(const char* data, unsigned int* remaining)
{
    transport.attempts.emplace_back(data, *remaining);
    if (transport.disconnect) properties.state = SOCKET_STATE_DISCONNECTED;
    if (transport.allowance <= 0) return transport.allowance;
    const unsigned int sent = std::min(*remaining, static_cast<unsigned int>(transport.allowance));
    transport.wire.append(data, sent);
    *remaining -= sent;
    return static_cast<int>(sent);
}

int main()
{
    int cases = 0;
    for (uint32_t gen : {ENCRYPT_GEN_1, ENCRYPT_GEN_5})
        for (auto rule : {Gen5Steps::PerStartedGroup, Gen5Steps::PerWholeGroup})
            for (int length : {12, 100, 9000})
                for (int firstWrite : {0, 3})
                {
                    recoveryCase(gen, rule, length, firstWrite);
                    ++cases;
                }
    // Simulated negative error on the initial/new-frame send path as well.
    recoveryCase(ENCRYPT_GEN_5, Gen5Steps::PerStartedGroup, 100, -1);
    cleanupCases();
    std::printf("FileQueueSendTest: %d recovery cases plus error/cleanup checks; %d failures\n", cases + 1, failures);
    return failures == 0 ? 0 : 1;
}
