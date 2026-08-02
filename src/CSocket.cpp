/*
 * include/CSocket.cpp
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

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cstring>

#include "CSocket.h"

#if defined(_WIN32) || defined(_WIN64)
	#include "IWin.h"
	#include <iphlpapi.h>
	#include <ws2tcpip.h>

#ifndef __GNUC__ // rain
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "iphlpapi.lib")
#endif

	#define SOCK_ENETDOWN			WSAENETDOWN
	#define SOCK_EADDRINUSE			WSAEADDRINUSE
	#define SOCK_EINTR				WSAEINTR
	#define SOCK_EINPROGRESS		WSAEINPROGRESS
	#define SOCK_EALREADY			WSAEALREADY
	#define SOCK_EADDRNOTAVAIL		WSAEADDRNOTAVAIL
	#define SOCK_EAFNOSUPPORT		WSAEAFNOSUPPORT
	#define SOCK_ECONNREFUSED		WSAECONNREFUSED
	#define SOCK_EFAULT				WSAEFAULT
	#define SOCK_EINVAL				WSAEINVAL
	#define SOCK_EISCONN			WSAEISCONN
	#define SOCK_ENETUNREACH		WSAENETUNREACH
	#define SOCK_ENOBUFS			WSAENOBUFS
	#define SOCK_ENOTSOCK			WSAENOTSOCK
	#define SOCK_ETIMEDOUT			WSAETIMEDOUT
	#define SOCK_EWOULDBLOCK		WSAEWOULDBLOCK
	#define SOCK_EAGAIN				WSAEWOULDBLOCK
	#define SOCK_EACCES				WSAEACCES
	#define SOCK_ENOTCONN			WSAENOTCONN
	#define SOCK_ENETRESET			WSAENETRESET
	#define SOCK_EOPNOTSUPP			WSAEOPNOTSUPP
	#define SOCK_EMSGSIZE			WSAEMSGSIZE
	#define SOCK_ECONNABORTED		WSAECONNABORTED
	#define SOCK_ECONNRESET			WSAECONNRESET
	#define SOCK_EHOSTUNREACH		WSAEHOSTUNREACH
	#define SOCK_ESHUTDOWN			WSAESHUTDOWN
	#define SOCK_SHUT_WR			SD_SEND
	#define SOCK_SHUT_RD			SD_RECEIVE
	#define SOCK_SHUT_RDWR			SD_BOTH

	// The following #define would cause some snprintf linking errors in cstdio with MinGW
	#if !defined(__GNUC__) && !(defined(_WIN32) || defined(_WIN64))
	  #define snprintf _snprintf
	#endif
#else
	#include <netdb.h>
	#include <cerrno>
	#include <unistd.h>
	#include <fcntl.h>

	#include <ifaddrs.h>
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/types.h>
	#include <sys/socket.h>

	#define SOCKET_ERROR	(-1)

	#define SOCK_ENETDOWN			ENETDOWN
	#define SOCK_EADDRINUSE			EADDRINUSE
	#define SOCK_EINTR				EINTR
	#define SOCK_EINPROGRESS		EINPROGRESS
	#define SOCK_EALREADY			EALREADY
	#define SOCK_EADDRNOTAVAIL		EADDRNOTAVAIL
	#define SOCK_EAFNOSUPPORT		EAFNOSUPPORT
	#define SOCK_ECONNREFUSED		ECONNREFUSED
	#define SOCK_EFAULT				EFAULT
	#define SOCK_EINVAL				EINVAL
	#define SOCK_EISCONN			EISCONN
	#define SOCK_ENETUNREACH		ENETUNREACH
	#define SOCK_ENOBUFS			ENOBUFS
	#define SOCK_ENOTSOCK			ENOTSOCK
	#define SOCK_ETIMEDOUT			ETIMEDOUT
	#define SOCK_EWOULDBLOCK		EWOULDBLOCK
	#define SOCK_EAGAIN				EAGAIN
	#define SOCK_EACCES				EACCES
	#define SOCK_ENOTCONN			ENOTCONN
	#define SOCK_ENETRESET			ENETRESET
	#define SOCK_EOPNOTSUPP			EOPNOTSUPP
	#define SOCK_EMSGSIZE			EMSGSIZE
	#define SOCK_ECONNABORTED		ECONNABORTED
	#define SOCK_ECONNRESET			ECONNRESET
	#define SOCK_EHOSTUNREACH		EHOSTUNREACH
	#define SOCK_ESHUTDOWN			ESHUTDOWN
	#define SOCK_SHUT_WR			SHUT_WR
	#define SOCK_SHUT_RD			SHUT_RD
	#define SOCK_SHUT_RDWR			SHUT_RDWR
#endif

// Don't send a signal.  Should only affect Linux.
#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

// Change this to any printf()-like function you use for logging purposes.
#define SLOG(x, ...)		 if (0) printf(x, ## __VA_ARGS__)
//////

// Function declarations.
static const char* errorMessage(int error);
static int identifyError();

bool CSocketManager::update(const long sec, const long usec)
{
	fd_set set_read{};
	fd_set set_write{};
	struct timeval tm{};

	tm.tv_sec = sec;
	tm.tv_usec = usec;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);

	// Put all the socket handles into the set.
	SOCKET max = 0;
	for (const auto& stub : stubList)
	{
		if (stub == nullptr) continue;
		if (const SOCKET sock = stub->getSocketHandle(); sock != INVALID_SOCKET)
		{
			if (sock > max) max = sock;
			if (stub->canRecv()) FD_SET(sock, &set_read);
			if (stub->canSend()) FD_SET(sock, &set_write);
		}
	}
	fd_max = max;

	// Do the select.
	select(fd_max + 1, &set_read, &set_write, nullptr, &tm);

	// Loop through all the socket handles and call relevant functions.
	for (auto& stub : stubList)
	{
		if (stub == nullptr) continue;
		if (const SOCKET sock = stub->getSocketHandle(); sock != INVALID_SOCKET)
		{
			bool success = true;
			if (FD_ISSET(sock, &set_read)) success = stub->onRecv();
			if (success && stub != nullptr && FD_ISSET(sock, &set_write)) success = stub->onSend();
			if (!success && stub != nullptr)
			{
				stub->onUnregister();
				stub = nullptr;
			}
		}
	}

	// Remove all nullptr stubs.
	std::erase(stubList, nullptr);

	// Add new stubs.
	if (!newStubs.empty())
	{
		stubList.insert(stubList.end(), newStubs.begin(), newStubs.end());
		newStubs.clear();
	}

	return true;
}

bool CSocketManager::updateSingle(CSocketStub* stub, const bool pRead, const bool pWrite, const long sec, const long usec)
{
	fd_set set_read{};
	fd_set set_write{};
	struct timeval tm{};

	if (stub == nullptr) return false;

	tm.tv_sec = sec;
	tm.tv_usec = usec;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);

	// Put the socket handle into the set.
	const SOCKET sock = stub->getSocketHandle();
	if (sock == INVALID_SOCKET) return false;
	if (pRead && stub->canRecv()) FD_SET(sock, &set_read);
	if (pWrite && stub->canSend()) FD_SET(sock, &set_write);

	// Do the select.
	select(fd_max + 1, &set_read, &set_write, nullptr, &tm);

	// Call relevant functions.
	if (FD_ISSET(sock, &set_read))
	{
		if (!stub->onRecv())
		{
			stub->onUnregister();
			if (const auto i = std::ranges::find(stubList, stub); i != stubList.end())
				*i = nullptr;
			return false;
		}
	}
	if (FD_ISSET(sock, &set_write))
	{
		if (!stub->onSend())
		{
			stub->onUnregister();
			if (const auto i = std::ranges::find(stubList, stub); i != stubList.end())
				*i = nullptr;
			return false;
		}
	}

	return true;
}

bool CSocketManager::registerSocket(CSocketStub* stub)
{
	if (stub == nullptr) return false;
	if (const SOCKET sock = stub->getSocketHandle(); stub->onRegister())
	{
		if (sock > fd_max) fd_max = sock;
		newStubs.push_back(stub);
		return true;
	}
	return false;
}

bool CSocketManager::unregisterSocket(CSocketStub* stub)
{
	if (stub == nullptr) return false;
	static std::unordered_set<CSocketStub*> unregisterSet;

	// Function to search for sockets that match this handle and add them to the unregister set.
	// This will also nullptr the stubs in the list so they can be removed later.
	unregisterSet.clear();
	auto findSocket = [](std::vector<CSocketStub*>& list, const SOCKET sock) -> bool
	{
		bool found = false;
		for (auto& s : list)
		{
			if (s == nullptr) continue;
			if (const SOCKET sock2 = s->getSocketHandle(); sock2 == sock)
			{
				unregisterSet.emplace(s);
				s = nullptr;
				found = true;
			}
		}
		return found;
	};

	// Search for sockets in both the stub list and the new stub list.
	const SOCKET sock = stub->getSocketHandle();
	bool found = findSocket(stubList, sock);
	found |= findSocket(newStubs, sock);

	// Call onUnregister for all the sockets we found.
	for (auto& s : unregisterSet)
	{
		if (s == nullptr) continue;
		s->onUnregister();
	}

	return found;
}

void CSocketManager::cleanup(const bool callOnUnregister)
{
	for (const auto stub : stubList)
	{
		if (stub == nullptr) continue;

		if (callOnUnregister)
			stub->onUnregister();
	}
	stubList.clear();
	fd_max = 0;
}

////////////////////////////////////////////////////////////

int CSocket::was_initiated = 0;

// Class functions
CSocket::CSocket() : webSocket(false)
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	memset((char *)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
}

CSocket::CSocket(const char* host, const char* port, const sock_properties* properties)
	: webSocket(false)
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	if (properties != nullptr)
		memcpy((void*)&this->properties, properties, sizeof(sock_properties));
	else
	{
		memset((char *)&this->properties.description, 0, SOCKET_MAX_DESCRIPTION);
	}
	this->init(host, port);
}

CSocket::~CSocket()
{
	// Destroy if still connected.
	if (properties.state != SOCKET_STATE_DISCONNECTED)
		disconnect();
}

int CSocket::init(const char* host, const char* port, int protocol)
{
	struct addrinfo hints{};
	struct addrinfo* res;

	// Make sure a TCP socket is disconnected.
	if (properties.protocol == SOCKET_PROTOCOL_TCP && properties.state != SOCKET_STATE_DISCONNECTED)
	{
		SLOG("[ERROR] Socket %s is already connected.\n", properties.description);
		return SOCKET_ALREADY_CONNECTED;
	}

	// Start creating the hints.
	memset((struct sockaddr_storage*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memset((void*)&hints, 0, sizeof(hints));
	if (properties.protocol == SOCKET_PROTOCOL_TCP) hints.ai_socktype = SOCK_STREAM;
	if (properties.protocol == SOCKET_PROTOCOL_UDP) hints.ai_socktype = SOCK_DGRAM;

	// Choose the protocol we want.
	switch (protocol)
	{
		case SOCKET_PROTOCOL_ANY:
			hints.ai_family = AF_UNSPEC;
			break;
		case SOCKET_PROTOCOL_IPV4:
			hints.ai_family = AF_INET;
			break;
		case SOCKET_PROTOCOL_IPV6:
			hints.ai_family = AF_INET6;
			break;
		default:
			return SOCKET_ERROR;
	}

	// Create the host.
	int error;
	if (properties.type == SOCKET_TYPE_CLIENT && host != nullptr)
		error = getaddrinfo(host, port, &hints, &res);
	else if (properties.type == SOCKET_TYPE_SERVER)
	{
		hints.ai_flags = AI_PASSIVE;		// Local socket.
		error = getaddrinfo(host, port, &hints, &res);
	}
	else
	{
		SLOG("[ERROR] Socket %s's properties.type is invalid.\n", properties.description);
		return SOCKET_ERROR;
	}

	// Check for errors.
	if (error)
	{
		SLOG("[CSocket::init] getaddrinfo() returned error: %d\n", error);
		return SOCKET_HOST_UNKNOWN;
	}
	else
	{
		memcpy((void*)&properties.address, res->ai_addr, res->ai_addrlen);
		properties.addresslen = res->ai_addrlen;
	}

	freeaddrinfo(res);

	return SOCKET_OK;
}

int CSocket::connect()
{
	// Make sure the socket is disconnected.
	if (properties.state != SOCKET_STATE_DISCONNECTED)
		return SOCKET_ALREADY_CONNECTED;

	// Flag the socket as connecting.
	properties.state = SOCKET_STATE_CONNECTING;

	// Create socket.
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
		properties.handle = socket(AF_INET, SOCK_STREAM, 0);
	else
		properties.handle = socket(AF_INET, SOCK_DGRAM, 0);

	// Make sure the socket was created correctly.
	if (properties.handle == INVALID_SOCKET)
	{
		SLOG("[CSocket::connect] socket() returned INVALID_SOCKET.\n");
		properties.state = SOCKET_STATE_DISCONNECTED;
		return SOCKET_INVALID;
	}

	// Bind the socket if it is a server-type socket.
	if (properties.type == SOCKET_TYPE_SERVER)
	{

        /* If we should use the reuse property. Default: yes */
        if (properties.reuse)
        {
		    // Let us reuse the address.
		    int value = 1;
		    setsockopt(properties.handle, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
        }

		// Bind the socket.
		if (::bind(properties.handle, reinterpret_cast<struct sockaddr *>(&properties.address), properties.addresslen) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] bind() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			return SOCKET_BIND_ERROR;
		}
	}

	// Connect the socket.
	if (properties.type != SOCKET_TYPE_SERVER)
	{
		if (::connect(properties.handle, reinterpret_cast<struct sockaddr *>(&properties.address), properties.addresslen) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] connect() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			return SOCKET_CONNECT_ERROR;
		}
	}

	// Disable the nagle algorithm.
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
	{
		int nagle = 1;
		setsockopt(properties.handle, IPPROTO_TCP, TCP_NODELAY, (char*)&nagle, sizeof(nagle));
	}

	// Set as non-blocking.
#if defined(_WIN32) || defined(_WIN64)
	u_long flags = 1;
	ioctlsocket(properties.handle, FIONBIO, &flags);
#else
	const int flags = fcntl(properties.handle, F_GETFL, 0);
	fcntl(properties.handle, F_SETFL, flags | O_NONBLOCK);
#endif

	// Socket connected!
	properties.state = SOCKET_STATE_CONNECTED;

	// Listening sockets.
	if (properties.type == SOCKET_TYPE_SERVER)
	{
		if (properties.protocol == SOCKET_PROTOCOL_UDP)
			properties.state = SOCKET_STATE_LISTENING;
		else if (properties.protocol == SOCKET_PROTOCOL_TCP)
		{
			if (::listen(properties.handle, SOMAXCONN) == SOCKET_ERROR)
			{
				SLOG("[CSocket::connect] listen() returned error: %s\n", errorMessage(identifyError()));
				disconnect();
				return SOCKET_CONNECT_ERROR;
			}

			properties.state = SOCKET_STATE_LISTENING;
		}
	}
	return SOCKET_OK;
}

void CSocket::disconnect()
{
	// Shut down the socket.
	if (shutdown(properties.handle, SOCK_SHUT_RDWR) == SOCKET_ERROR)
	{
		const int error = identifyError();
		if (error == SOCK_ENOTSOCK)
		{
			properties.handle = INVALID_SOCKET;
			properties.state = SOCKET_STATE_DISCONNECTED;
			return;
		}
		SLOG("[CSocket::destroy] shutdown returned error: %s\n", errorMessage(error));
	}

	// Mark socket as terminating.
	properties.state = SOCKET_STATE_TERMINATING;

	// Gracefully shut it down.
	/*
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
	{
		int count = 0;
		char buff[ 0x2000 ];
		int size;
		while (++count < 3)
		{
			size = recv(properties.handle, buff, 0x2000, 0);
			if (size == 0) break;
			if (size == SOCKET_ERROR) break;
		}
	}
	*/

	// Destroy the socket of d00m.
#if defined(_WIN32) || defined(_WIN64)
	if (closesocket(properties.handle) == SOCKET_ERROR)
	{
		SLOG("[CSocket::destroy] closesocket ");
#else
	if (close(properties.handle) == SOCKET_ERROR)
	{
		SLOG("[CSocket::destroy] close ");
#endif
		SLOG("returned error: %s\n", errorMessage(identifyError()));
	}

	// Reset the socket state.
	properties.handle = INVALID_SOCKET;
	properties.state = SOCKET_STATE_DISCONNECTED;
}

int CSocket::reconnect(const long delay, const int tries)
{
	for (int i = 0; i < tries; i++)
	{
		switch (this->connect())
		{
			case SOCKET_OK:
			case SOCKET_ALREADY_CONNECTED:
				return SOCKET_OK;
			case SOCKET_INVALID:
			case SOCKET_BIND_ERROR:
			case SOCKET_CONNECT_ERROR:
			default:
				// Do nothing.
				break;
		}
		if (delay != 0)
		{
			std::chrono::milliseconds dur{ delay };
			std::this_thread::sleep_for(dur);
		}
	}
	return SOCKET_CONNECT_ERROR;
}

CSocket* CSocket::accept()
{
	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return nullptr;

	// Only server type TCP sockets can accept new connections.
	if (properties.type != SOCKET_TYPE_SERVER || properties.protocol != SOCKET_PROTOCOL_TCP)
		return nullptr;

	sockaddr_storage addr{};
	int addrlen = sizeof(addr);
	SOCKET handle = 0;

	// Try to accept a new connection.
	handle = ::accept(properties.handle, reinterpret_cast<struct sockaddr *>(&addr), reinterpret_cast<socklen_t *>(&addrlen));
	if (handle == INVALID_SOCKET)
	{
		const int error = identifyError();
		if (error == SOCK_EWOULDBLOCK || error == SOCK_EINPROGRESS) return nullptr;
		SLOG("[CSocket::accept] accept() returned error: %s\n", errorMessage(error));
		return nullptr;
	}

	// Create the new socket to store the new connection.
	auto* sock = new CSocket();
	sock_properties props;
	memset((void*)&props, 0, sizeof(sock_properties));
	memset((void*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memcpy((void*)&props.address, &addr, sizeof(addr));
	props.protocol = properties.protocol;
	props.type = SOCKET_TYPE_CLIENT;
	props.state = SOCKET_STATE_CONNECTED;
	props.handle = handle;
	sock->setProperties(props);
	sock->setDescription(sock->getRemoteIp());

	// Disable the nagle algorithm.
	if (props.protocol == SOCKET_PROTOCOL_TCP)
	{
		int nagle = 1;
		setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (char*)&nagle, sizeof(nagle));
	}

	// Set as non-blocking.
#if defined(_WIN32) || defined(_WIN64)
	u_long flags = 1;
	ioctlsocket(handle, FIONBIO, &flags);
#else
	const int flags = fcntl(properties.handle, F_GETFL, 0);
	fcntl(handle, F_SETFL, flags | O_NONBLOCK);
#endif

	// Accept the connection by calling getsockopt.
	int type, typeSize = sizeof(int);
	getsockopt(handle, SOL_SOCKET, SO_TYPE, (char*)&type, reinterpret_cast<socklen_t *>(&typeSize));

	return sock;
}

int CSocket::sendData(const char* data, unsigned int* dsize)
{
	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
	{
		*dsize = 0;
		return 0;
	}

	// Send our data, yay!
	int sent = 0;
	if (sent = static_cast<int>(::send(properties.handle, data, *dsize, MSG_NOSIGNAL)); sent == SOCKET_ERROR)
	{
		switch (int intError = identifyError())
		{
			case SOCK_ENETDOWN:
			case SOCK_ENETRESET:
			case SOCK_ENOTCONN:
			case SOCK_EHOSTUNREACH:
			case SOCK_ECONNABORTED:
			case SOCK_ECONNRESET:
			case SOCK_ETIMEDOUT:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				return 0;
			case SOCK_EAGAIN:
				return 0;
			default:
				break;
		}
		disconnect();
		return 0;
	}

	// Remove what we sent from the total size.
	*dsize -= sent;

	// Return how much data was ultimately sent.
	return sent;
}

char* CSocket::getData(unsigned int* dsize)
{
	// Create the buffer.
	constexpr int BUFF_LEN = 0x8000;	// 32KB.
	static char buff[BUFF_LEN];	// 32KB.

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
	{
		*dsize = 0;
		return nullptr;
	}

	// Allocate buff.
	memset((void*)buff, 0, BUFF_LEN);

	// Get our data
	int size = 0;
	if (properties.protocol == SOCKET_PROTOCOL_UDP)
		size = static_cast<int>(recvfrom(properties.handle, buff, BUFF_LEN, 0, nullptr, nullptr));
	else
		size = static_cast<int>(recv(properties.handle, buff, BUFF_LEN, 0));

	// Check for error!
	int intError = 0;
	if (size == SOCKET_ERROR)
	{
		size = 0;
		intError = identifyError();
		switch (intError)
		{
			case SOCK_ENETDOWN:
			case SOCK_ENETRESET:
			case SOCK_ENOTCONN:
			case SOCK_EHOSTUNREACH:
			case SOCK_ECONNABORTED:
			case SOCK_ECONNRESET:
			case SOCK_ETIMEDOUT:
			case SOCK_ESHUTDOWN:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				break;

			default:
				break;
		}
	}

	// If size is 0, the socket was disconnected.
	if (size == 0 && intError != SOCK_EWOULDBLOCK)
		disconnect();

	// Set dsize to how much data was returned.
	*dsize = size;

	// Return the data.
	return buff;
}

char* CSocket::peekData(unsigned int* dsize)
{
	// Create the buffer.
	constexpr int BUFF_LEN = 0x8000;	// 32KB.
	static char buff[BUFF_LEN];	// 32KB.

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
	{
		*dsize = 0;
		return nullptr;
	}

	// Allocate buff.
	memset((void*)buff, 0, BUFF_LEN);

	// Get our data
	int size = 0;
	if (properties.protocol == SOCKET_PROTOCOL_UDP)
		size = static_cast<int>(recvfrom(properties.handle, buff, BUFF_LEN, MSG_PEEK, nullptr, nullptr));
	else
		size = static_cast<int>(recv(properties.handle, buff, BUFF_LEN, MSG_PEEK));

	// Check for error!
	if (size == SOCKET_ERROR)
	{
		switch (int intError = identifyError())
		{
			case SOCK_ENETDOWN:
			case SOCK_ENETRESET:
			case SOCK_ENOTCONN:
			case SOCK_EHOSTUNREACH:
			case SOCK_ECONNABORTED:
			case SOCK_ECONNRESET:
			case SOCK_ETIMEDOUT:
			case SOCK_ESHUTDOWN:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				break;
			default:
				break;
		}
		return nullptr;
	}

	// Return the data.
	*dsize = size;
	return buff;
}

int CSocket::setProtocol(const int sock_proto)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.protocol = sock_proto;
	else
		return SOCKET_INVALID;
	return SOCKET_OK;
}

int CSocket::setType(const int sock_type)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.type = sock_type;
	else
		return SOCKET_INVALID;
	return SOCKET_OK;
}

int CSocket::setDescription(const char *strDescription)
{
	memset((void*)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
	strncpy(properties.description, strDescription, std::min(strlen(strDescription), (size_t)SOCKET_MAX_DESCRIPTION - 1));
	return SOCKET_OK;
}

int CSocket::setProperties(sock_properties newprop)
{
	// Set the socket properties.
	memcpy((void*)&this->properties, (void *)&newprop, sizeof(sock_properties));

	return SOCKET_OK;
}

int CSocket::setState(const int iState)
{
	this->properties.state = iState;
	return SOCKET_OK;
}

const char* CSocket::getRemoteIp() const
{
	static char host[1025];
	memset((void*)host, 0, 1025);

	// Grab the IP address.
	if (getnameinfo(reinterpret_cast<const struct sockaddr *>(&properties.address), sizeof(struct sockaddr_storage), host, 1025, nullptr, 0, NI_NUMERICHOST))
		return nullptr;

	return &host[0];
}

const char* CSocket::getRemotePort() const
{
	static char port[32];
	memset((void*)port, 0, 32);

	// Grab the IP address.
	if (getnameinfo(reinterpret_cast<const struct sockaddr*>(&properties.address), sizeof(struct sockaddr_storage), nullptr, 0, port, 32, NI_NUMERICSERV))
		return nullptr;

	return &port[0];
}

static bool isPrivateIpv4(const uint32_t host)
{
    const uint8_t a = (host >> 24) & 0xFF;
    const uint8_t b = (host >> 16) & 0xFF;
    if (a == 10) return true;                        // 10.0.0.0/8
    if (a == 172 && b >= 16 && b <= 31) return true; // 172.16.0.0/12
    if (a == 192 && b == 168) return true;           // 192.168.0.0/16
    return false;
}

static bool isLoopbackIpv4(const uint32_t host)
{
    return ((host >> 24) & 0xFF) == 127;  // 127.0.0.0/8
}

static std::vector<std::string> getPrivateLanIpv4s() {
    std::vector<std::string> out;

#ifdef _WIN32
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG family = AF_INET;
    ULONG bufLen = 15 * 1024;
    std::vector<unsigned char> buf(bufLen);

    auto addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    auto rc = GetAdaptersAddresses(family, flags, nullptr, addrs, &bufLen);
    if (rc == ERROR_BUFFER_OVERFLOW)
    {
        buf.resize(bufLen);
        addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
        rc = GetAdaptersAddresses(family, flags, nullptr, addrs, &bufLen);
    }

    if (rc == NO_ERROR)
    {
        for (auto* a = addrs; a; a = a->Next)
        {
            if (a->OperStatus != IfOperStatusUp) continue;
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            for (auto* u = a->FirstUnicastAddress; u; u = u->Next)
            {
                if (!u->Address.lpSockaddr || u->Address.lpSockaddr->sa_family != AF_INET)
                	continue;

                auto* sin = reinterpret_cast<sockaddr_in*>(u->Address.lpSockaddr);
                uint32_t host = ntohl(sin->sin_addr.s_addr);
                if (isLoopbackIpv4(host) || !isPrivateIpv4(host))
                	continue;

                char s[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &sin->sin_addr, s, sizeof(s));
                out.emplace_back(s);
            }
        }
    }
#else
    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0)
    	return out;

    for (const ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        const auto* sin = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
        uint32_t host = ntohl(sin->sin_addr.s_addr);
        if (isLoopbackIpv4(host) || !isPrivateIpv4(host))
        	continue;

        char s[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &sin->sin_addr, s, sizeof(s));
        out.emplace_back(s);
    }

    freeifaddrs(ifaddr);
#endif

    return out;
}

const char* CSocket::getLocalIp()
{
	static char host[1025] = {};

	if (const auto ips = getPrivateLanIpv4s(); !ips.empty())
	{
		strncpy(host, ips[0].c_str(), sizeof(host) - 1);
		host[sizeof(host) - 1] = '\0';
	}

	return host;
}

int CSocket::socketSystemInit()
{
//	errorOut("debuglog.txt", ":: Initializing socket system...");
#if defined(_WIN32) || defined(_WIN64)
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		SLOG("Failed to initialize winsocks!\n");
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		SLOG("Failed to initialize winsocks!  Wasn't version 2.2!\n");
		WSACleanup();
		return 1;
	}
#endif

	CSocket::was_initiated = 1;
	return 0;
}

void CSocket::socketSystemDestroy()
{
	using namespace std::chrono_literals;
#if defined(_WIN32) || defined(_WIN64)
	int intTimeCheck = 0;

	while (intTimeCheck++ < 3)
	{
		if (WSACleanup() == 0)
			break;

		SLOG("[CSocket::socketSystemDestroy] WSACleanup() returned error: %s\n", errorMessage(identifyError()));
		std::this_thread::sleep_for(1s);
	}
#endif
}

const char* errorMessage(const int error)
{
	// These can happen a lot.  Don't display any errors about them.
	if (error == SOCK_EWOULDBLOCK || error == SOCK_EINPROGRESS)
		return nullptr;

	switch (error)
	{
#if defined(_WIN32) || defined(_WIN64)
		case WSANOTINITIALISED:
			return "WSANOTINITIALISED";
#endif
		case SOCK_ENETDOWN:
			return "ENETDOWN";
		case SOCK_EADDRINUSE:
			return "EADDRINUSE";
		case SOCK_EINTR:
			return "EINTR";
		//case SOCK_EINPROGRESS:
		//	return "EINPROGRESS";
		case SOCK_EALREADY:
			return "EALREADY";
		case SOCK_EADDRNOTAVAIL:
			return "EADDRNOTAVAIL";
		case SOCK_EAFNOSUPPORT:
			return "EAFNOSUPPORT";
		case SOCK_ECONNREFUSED:
			return "ECONNREFUSED";
		case SOCK_EFAULT:
			return "EFAULT";
		case SOCK_EINVAL:
			return "EINVAL";
		case SOCK_EISCONN:
			return "EISCONN";
		case SOCK_ENETUNREACH:
			return "ENETUNREACH";
		case SOCK_ENOBUFS:
			return "ENOBUFS";
		case SOCK_ENOTSOCK:
			return "ENOTSOCK";
		case SOCK_ETIMEDOUT:
			return "ETIMEDOUT";
		//case SOCK_EWOULDBLOCK:
		//	return "EWOULDBLOCK";
		case SOCK_EACCES:
			return "EACCES";
		case SOCK_ENOTCONN:
			return "ENOTCONN";
		case SOCK_ENETRESET:
			return "ENETRESET";
		case SOCK_EOPNOTSUPP:
			return "EOPNOTSUPP";
		case SOCK_ESHUTDOWN:
			return "ESHUTDOWN";
		case SOCK_EMSGSIZE:
			return "EMSGSIZE";
		case SOCK_ECONNABORTED:
			return "ECONNABORTED";
		case SOCK_ECONNRESET:
			return "ECONNRESET";
		case SOCK_EHOSTUNREACH:
			return "EHOSTUNREACH";
		default:
		{
			static char buf[32];
			//#ifdef __GNUC__
			//	__gnu_cxx::snprintf(buf, 32, "%d", error);
			//#else
				snprintf(buf, 32, "%d", error);
			//#endif
			return buf;
		}
	}
}

int identifyError()
{
#if defined(_WIN32) || defined(_WIN64)
	return WSAGetLastError();
#else
	return errno;
#endif
}

#undef SLOG
