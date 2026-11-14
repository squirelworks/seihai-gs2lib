// Seihai project, 2026-11-14: see CHANGES-SEIHAI.md.
// This file is released under the GNU General Public License version 3 or later.
// Only literal loopback addresses and ephemeral ports are used.

#include <CSocket.h>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <string>
#if !defined(_WIN32) && !defined(_WIN64)
#include <cerrno>
#include <unistd.h>
#endif

namespace
{
int failures = 0;
#if defined(_WIN32) || defined(_WIN64)
using AddressLength = int;
#else
using AddressLength = socklen_t;
#endif

bool check(bool condition, const char* operation, int transport, int family)
{
    if (!condition)
    {
        std::fprintf(stderr, "%s failed (transport=%d, family=%d)\n", operation, transport, family);
        ++failures;
    }
    return condition;
}

bool ipv6Available()
{
    SOCKET handle = socket(AF_INET6, SOCK_STREAM, 0);
    int result = -1;
    if (handle != INVALID_SOCKET)
    {
        sockaddr_in6 address{};
        address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_loopback;
        result = bind(handle, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    }
#if defined(_WIN32) || defined(_WIN64)
    const int error = result == 0 ? 0 : WSAGetLastError();
    if (handle != INVALID_SOCKET) closesocket(handle);
    const bool unavailable = error == WSAEAFNOSUPPORT || error == WSAEADDRNOTAVAIL || error == WSAEPROTONOSUPPORT;
#else
    const int error = result == 0 ? 0 : errno;
    if (handle != INVALID_SOCKET) close(handle);
    const bool unavailable = error == EAFNOSUPPORT || error == EADDRNOTAVAIL || error == EPROTONOSUPPORT;
#endif
    if (result == 0) return true;
    if (unavailable)
        std::printf("IPv6 loopback unavailable (native error %d); IPv6 cases skipped\n", error);
    else
    {
        std::fprintf(stderr, "Native IPv6 control failed unexpectedly: %d\n", error);
        ++failures;
    }
    return false;
}

void exercise(const char* host, int requestedFamily, int expectedFamily, int transport)
{
    CSocket server;
    server.setType(SOCKET_TYPE_SERVER);
    server.setProtocol(transport);
    if (!check(server.init(host, "0", requestedFamily) == SOCKET_OK, "server init", transport, requestedFamily)) return;
    if (!check(server.connect() == SOCKET_OK, "server bind/listen", transport, requestedFamily)) return;

    sockaddr_storage bound{};
    AddressLength length = sizeof(bound);
    if (!check(getsockname(server.getHandle(), reinterpret_cast<sockaddr*>(&bound), &length) == 0,
               "getsockname", transport, requestedFamily)) return;
    if (!check(bound.ss_family == expectedFamily, "bound family", transport, requestedFamily)) return;
    const auto port = expectedFamily == AF_INET
        ? reinterpret_cast<const sockaddr_in*>(&bound)->sin_port
        : reinterpret_cast<const sockaddr_in6*>(&bound)->sin6_port;
    if (!check(port != 0, "ephemeral port", transport, requestedFamily)) return;

    CSocket client;
    client.setProtocol(transport);
    const std::string service = std::to_string(ntohs(port));
    if (!check(client.init(host, service.c_str(), requestedFamily) == SOCKET_OK, "client init", transport, requestedFamily)) return;
    if (!check(client.connect() == SOCKET_OK, "client connect", transport, requestedFamily)) return;
    sockaddr_storage peer{};
    length = sizeof(peer);
    check(getpeername(client.getHandle(), reinterpret_cast<sockaddr*>(&peer), &length) == 0
              && peer.ss_family == expectedFamily, "connected family", transport, requestedFamily);
    if (transport == SOCKET_PROTOCOL_TCP)
    {
        std::unique_ptr<CSocket> accepted(server.accept());
        check(accepted != nullptr, "accept", transport, requestedFamily);
    }
    std::printf("Loopback case checked: transport=%d requested=%d actual=%d\n", transport, requestedFamily, expectedFamily);
}
}

int main()
{
    CSocket initializeSocketSystem;
    const bool ipv6 = ipv6Available();
    for (int transport : {SOCKET_PROTOCOL_TCP, SOCKET_PROTOCOL_UDP})
    {
        exercise("127.0.0.1", SOCKET_PROTOCOL_IPV4, AF_INET, transport);
        exercise("127.0.0.1", SOCKET_PROTOCOL_ANY, AF_INET, transport);
        if (ipv6)
        {
            exercise("::1", SOCKET_PROTOCOL_IPV6, AF_INET6, transport);
            exercise("::1", SOCKET_PROTOCOL_ANY, AF_INET6, transport);
        }
    }
    return failures == 0 ? 0 : 1;
}
