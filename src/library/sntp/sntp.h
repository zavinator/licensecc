/*
 * sntp.h
 *
 *  Created on: Sep 11, 2026
 *      Author: GC
 *
 * Minimal header only SNTP (rfc 4330 / rfc 5905) client. It sends a single udp datagram to an
 * NTP server and returns the server time, so that the library can detect a system clock that has
 * been deliberately set in the past to keep using an expired license.
 *
 * The simple network time protocol is not authenticated: it protects against a casual clock
 * manipulation, not against an attacker able to intercept or spoof the udp traffic.
 *
 * NOTE: on windows this header includes <winsock2.h>, so it must be included before any header
 * pulling in <windows.h> (eg. licensecc/datatypes.h), otherwise the deprecated <winsock.h> will
 * be included first and the two versions of the api will conflict.
 */

#ifndef SRC_LIBRARY_SNTP_SNTP_H_
#define SRC_LIBRARY_SNTP_SNTP_H_

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef SOCKET lcc_socket_t;
// winsock doesn't define socklen_t, and the resolution result type is called ADDRINFO
typedef int lcc_socklen_t;
typedef ADDRINFO lcc_addrinfo;
#define LCC_INVALID_SOCKET INVALID_SOCKET
#define LCC_SOCKET_ERROR SOCKET_ERROR
#else
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int lcc_socket_t;
typedef socklen_t lcc_socklen_t;
typedef addrinfo lcc_addrinfo;
#define LCC_INVALID_SOCKET (-1)
#define LCC_SOCKET_ERROR (-1)
#endif

#include <time.h>

namespace license {
namespace sntp {

/**
 * Size in bytes of an SNTP packet, both request and response.
 */
static const size_t SNTP_PACKET_SIZE = 48;
/**
 * Default time in ms the client waits for the server response before giving up.
 */
static const unsigned int SNTP_DEFAULT_TIMEOUT_MS = 2000;

/**
 * \brief Owns a dns resolution result, releasing it with freeaddrinfo.
 */
class AddrInfoGuard {
public:
	explicit AddrInfoGuard(lcc_addrinfo* info) : m_info(info) {}

	~AddrInfoGuard() {
		if (m_info != nullptr) {
			freeaddrinfo(m_info);
		}
	}

	const lcc_addrinfo* get() const { return m_info; }

private:
	AddrInfoGuard(const AddrInfoGuard&);
	AddrInfoGuard& operator=(const AddrInfoGuard&);

	lcc_addrinfo* m_info;
};

/**
 * \brief Owns a udp socket, closing it at the end of the scope also in case of errors.
 */
class SocketGuard {
public:
	SocketGuard() : m_socket(LCC_INVALID_SOCKET) {}

	~SocketGuard() { close_socket(); }

	lcc_socket_t get() const { return m_socket; }

	bool is_open() const { return m_socket != LCC_INVALID_SOCKET; }

	/// Takes ownership of a newly created socket.
	void reset(lcc_socket_t sock) {
		close_socket();
		m_socket = sock;
	}

private:
	SocketGuard(const SocketGuard&);
	SocketGuard& operator=(const SocketGuard&);

	void close_socket() {
		if (m_socket != LCC_INVALID_SOCKET) {
#ifdef _WIN32
			closesocket(m_socket);
#else
			::close(m_socket);
#endif
			m_socket = LCC_INVALID_SOCKET;
		}
	}

	lcc_socket_t m_socket;
};

#ifdef _WIN32
/**
 * \brief Initializes the winsock library once per process, cleaning it up at process exit.
 */
class WinsockInitializer {
public:
	WinsockInitializer() : m_initialized(false) {
		WSADATA wsadata;
		m_initialized = (WSAStartup(MAKEWORD(2, 2), &wsadata) == 0);
	}

	~WinsockInitializer() {
		if (m_initialized) {
			WSACleanup();
		}
	}

	bool is_initialized() const { return m_initialized; }

private:
	WinsockInitializer(const WinsockInitializer&);
	WinsockInitializer& operator=(const WinsockInitializer&);

	bool m_initialized;
};
#endif

inline bool init_sockets() {
#ifdef _WIN32
	static const WinsockInitializer initializer;
	return initializer.is_initialized();
#else
	return true;
#endif
}

inline uint32_t read_seconds(const unsigned char* buffer) {
	return ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
}

inline void write_seconds(unsigned char* buffer, uint32_t seconds) {
	buffer[0] = (unsigned char)((seconds >> 24) & 0xff);
	buffer[1] = (unsigned char)((seconds >> 16) & 0xff);
	buffer[2] = (unsigned char)((seconds >> 8) & 0xff);
	buffer[3] = (unsigned char)(seconds & 0xff);
}

inline bool set_receive_timeout(lcc_socket_t sock, unsigned int timeout_ms) {
#ifdef _WIN32
	const DWORD timeout = (DWORD)timeout_ms;
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == 0;
#else
	timeval timeout;
	timeout.tv_sec = (time_t)(timeout_ms / 1000);
	timeout.tv_usec = (suseconds_t)((timeout_ms % 1000) * 1000);
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0;
#endif
}

inline bool build_request(unsigned char* packet, uint32_t origin_seconds) {
	memset(packet, 0, SNTP_PACKET_SIZE);
	// leap indicator 0 (no warning), version 4, mode 3 (client)
	packet[0] = (unsigned char)(0x23);
	write_seconds(&packet[40], origin_seconds);
	return true;
}

/**
 * Discards the responses that are not usable to synchronize a clock: wrong mode, unsynchronized
 * servers (kiss o' death packets), excessive root delay/dispersion, timestamps out of the current
 * NTP era or responses not referring to the request we sent.
 */
inline bool parse_response(const unsigned char* packet, size_t received, uint32_t origin_seconds,
						   time_t& out_server_time) {
	static const uint32_t NTP_EPOCH_OFFSET = 2208988800U;
	static const uint32_t MAX_ROOT_DELAY = 0x00010000U;	 // 1 second
	static const uint32_t MAX_ROOT_DISPERSION = 0x00080000U;  // 8 seconds

	if (received < SNTP_PACKET_SIZE) {
		return false;
	}
	const unsigned char leap_indicator_mode = packet[0];
	if ((leap_indicator_mode & 0x07) != 4) {
		// not a server response
		return false;
	}
	if ((leap_indicator_mode >> 6) == 3) {
		// alarm condition, the server clock is not synchronized
		return false;
	}
	const unsigned char stratum = packet[1];
	if (stratum == 0 || stratum > 15) {
		return false;
	}
	if (packet[3] == 0) {
		// invalid precision
		return false;
	}
	if (read_seconds(&packet[4]) > MAX_ROOT_DELAY || read_seconds(&packet[8]) > MAX_ROOT_DISPERSION) {
		return false;
	}
	if (read_seconds(&packet[24]) != origin_seconds) {
		// the response doesn't refer to the request we sent
		return false;
	}
	const uint32_t transmit_seconds = read_seconds(&packet[40]);
	if (transmit_seconds <= NTP_EPOCH_OFFSET) {
		return false;
	}
	out_server_time = (time_t)(transmit_seconds - NTP_EPOCH_OFFSET);
	return true;
}

/**
 * Queries an NTP server over udp and returns the server time.
 *
 * @param server_name host name or address of the NTP server, port 123 is used.
 * @param out_server_time time reported by the server, expressed as seconds since the unix epoch.
 *        Meaningful only when the function returns true.
 * @param out_reference_time local time captured just before the request was sent. Subtracting it
 *        from out_server_time gives the offset of the local clock, with the network latency
 *        included.
 * @param timeout_ms time to wait for the server response.
 * @return true if a valid response has been received, false in any other case. This function
 *         never throws.
 */
inline bool query(const char* server_name, time_t& out_server_time, time_t& out_reference_time,
				  unsigned int timeout_ms = SNTP_DEFAULT_TIMEOUT_MS) {
	if (server_name == nullptr || !init_sockets()) {
		return false;
	}

	lcc_addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	lcc_addrinfo* resolved = nullptr;
	if (getaddrinfo(server_name, "123", &hints, &resolved) != 0 || resolved == nullptr) {
		return false;
	}
	const AddrInfoGuard addresses(resolved);

	for (const lcc_addrinfo* address = addresses.get(); address != nullptr; address = address->ai_next) {
		SocketGuard sock;
		sock.reset(socket(address->ai_family, address->ai_socktype, address->ai_protocol));
		if (!sock.is_open()) {
			continue;
		}
		if (!set_receive_timeout(sock.get(), timeout_ms)) {
			continue;
		}

		out_reference_time = time(nullptr);
		unsigned char request[SNTP_PACKET_SIZE];
		const uint32_t origin_seconds = (uint32_t)out_reference_time + 2208988800U;
		build_request(request, origin_seconds);

		const int sent = sendto(sock.get(), (const char*)request, (int)SNTP_PACKET_SIZE, 0, address->ai_addr,
								(int)address->ai_addrlen);
		if (sent != (int)SNTP_PACKET_SIZE) {
			continue;
		}

		unsigned char response[SNTP_PACKET_SIZE];
		sockaddr_storage from;
		lcc_socklen_t from_length = (lcc_socklen_t)sizeof(from);
		memset(&from, 0, sizeof(from));

		const int received =
			recvfrom(sock.get(), (char*)response, (int)SNTP_PACKET_SIZE, 0, (sockaddr*)&from, &from_length);
		if (received == LCC_SOCKET_ERROR) {
			continue;
		}
		if (parse_response(response, (size_t)received, origin_seconds, out_server_time)) {
			return true;
		}
	}
	return false;
}

} /* namespace sntp */
} /* namespace license */

#endif /* SRC_LIBRARY_SNTP_SNTP_H_ */
