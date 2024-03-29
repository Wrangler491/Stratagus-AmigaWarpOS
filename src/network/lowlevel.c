//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name lowlevel.c - The network lowlevel. */
//
//      (c) Copyright 2000-2004 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id: lowlevel.c,v 1.49 2004/06/24 18:07:53 jarod42 Exp $

//@{

//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#ifdef USE_SDLA
#include <unistd.h>
#include <sys/file.h>
#pragma pack(2)
#include <proto/exec.h>

#include <powerup/ppcinline/socket.h>
#include <amitcp/socketbasetags.h>
#pragma pack()
#endif

#ifndef _MSC_VER
#include <signal.h>
#endif

#include "stratagus.h"
#include "net_lowlevel.h"
#include "network.h"
#ifdef USE_SDLA
#include "netconnect.h"
#include <SDL_net.h>
extern struct library *SocketBase; //= NULL;
#define SOCKETVERSION 4
#endif

//----------------------------------------------------------------------------
//  Declarations
//----------------------------------------------------------------------------

#define MAX_LOC_IP 10
#define MAXSOCKS 512

//----------------------------------------------------------------------------
//  Variables
//----------------------------------------------------------------------------

int NetLastSocket;         ///< Last socket
unsigned long NetLastHost; ///< Last host number (net format)
int NetLastPort;           ///< Last port number (net format)

unsigned long NetLocalAddrs[MAX_LOC_IP]; ///< Local IP-Addrs of this host (net format)

#ifdef USE_SDLA
static UDPsocket Socklist[MAXSOCKS];
#endif

//----------------------------------------------------------------------------
//  Low level functions
//----------------------------------------------------------------------------

#ifdef USE_SDLA
	/// Hardware dependend network init.
int NetInit(void)
{
	return SDLNet_Init();
}

	/// Hardware dependend network exit.
void NetExit(void)
{
	SDLNet_Quit();
}

	/// Resolve host in name or or colon dot notation.
unsigned long NetResolveHost(const char* host)
{
	IPaddress address;
	int retval;

	if(host == NULL) {
		return INADDR_NONE;
	}
	retval = SDLNet_ResolveHost(&address, host, NetworkDefaultPort);
	if(retval) {
		return INADDR_NONE;
	}
	return (address.host);
}

	/// Get local IP from network file descriptor
int NetSocketAddr(const Socket sock)
{
	UDPsocket socket;
	IPaddress *address;

	socket = Socklist[sock];
	if(socket==NULL) {
		return -1;
	}
	address = SDLNet_UDP_GetPeerAddress(socket, -1);
	if(!address) {
		printf("SDLNet_UDP_GetPeerAddress: %s\n", SDLNet_GetError());
		return -1;
	}

	return (address->host);
}

	/// Open a UDP Socket port.
Socket NetOpenUDP(int port)
{
	IPaddress *address;
	UDPsocket socket;
	int retval;
	Socket i;

	for(i=0;i<MAXSOCKS;++i) {
		if(Socklist[i] == NULL) {
			socket = SDLNet_UDP_Open(port);
			if(socket){
				Socklist[i] = socket;
				address = SDLNet_UDP_GetPeerAddress(socket, -1);
				if(address == NULL) {
					printf("Error obtaining IP address\n");
					NetCloseUDP(i);
					return -1;
				}
				NetLastHost = address->host;
				NetLastPort = address->port;
				return i;
			}else{
				printf(" Open port failed\n");
				return -1;
			}
		}
	}
	printf("Unable to open another socket\n");
	return -1;

}

	/// Open a TCP Socket port.
Socket NetOpenTCP(int port)
{
}

	/// Close a UDP socket port.
void NetCloseUDP(Socket sockfd)
{
	UDPsocket socket;
	socket = Socklist[sockfd];
	if(socket!=NULL) {
		SDLNet_UDP_Close(socket);
		Socklist[sockfd] = NULL;
	}
}

	/// Close a TCP socket port.
void NetCloseTCP(Socket sockfd)
{
}

	/// Set socket to non-blocking
int NetSetNonBlocking(Socket sockfd)
{
}

	/// Open a TCP connection.
int NetConnectTCP(Socket sockfd, unsigned long addr, int port)
{
}

	/// Send through a UDP socket to a host:port.
int NetSendUDP(Socket sockfd, unsigned long host, int port, const void* buf, int len)
{
	UDPsocket socket;
	int retval;
	IPaddress address;

	socket = Socklist[sockfd];
	if(socket == NULL) {
		return -1;
	}
	address.host = host;
	address.port = port;

	UDPpacket *packet;
	packet = SDLNet_AllocPacket(sizeof(UDPpacket) + len);
	if ( packet == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	
	memcpy(packet->data, buf, len);

	packet->len = len;
	packet->maxlen = len;
	packet->address = address;

	retval = SDLNet_UDP_Send(socket, -1, packet);

	SDLNet_FreePacket(packet);

	return retval;
}

	/// Send through a TCP socket
int NetSendTCP(Socket sockfd, const void* buf, int len)
{
}

	/// Wait for socket ready.
int NetSocketReady(Socket sockfd, int timeout) 
{
	UDPsocket socket;
	SDLNet_SocketSet set;
	int retval;

	socket = Socklist[sockfd];
	if(socket == NULL) {
		return -1;
	}

	set = SDLNet_AllocSocketSet(1);
	if(set == NULL) {
		printf("Error opening socketset\n");
		return -1;
	}
	retval = SDLNet_UDP_AddSocket(set, socket);
	if (retval == -1) {
		printf("Error adding socket to set\n");
		SDLNet_FreeSocketSet(set);
		return -1;
	}
	retval = SDLNet_CheckSockets(set, timeout);
	if (retval == -1) {
		printf("Error checking socket\n");
	}

	SDLNet_FreeSocketSet(set);
	return retval;
}

	/// Receive from a UDP socket.
int NetRecvUDP(Socket sockfd, void* buf, int len)
{
	int retval;
	UDPsocket socket;

	socket = Socklist[sockfd];
	if(socket == NULL) {
		return -1;
	}
	
	UDPpacket *packet;
	packet = SDLNet_AllocPacket(len);
	if (packet == NULL) {
		SDL_OutOfMemory();
		return(-1);
	}

	retval = SDLNet_UDP_Recv(socket, packet);
	
	if(retval == 1) {
		memcpy(buf, packet->data, packet->len);
		retval = packet->len;
		NetLastHost = packet->address.host;
		NetLastPort = packet->address.port;
	}

	SDLNet_FreePacket(packet);

	return retval;
}

	/// Receive from a TCP socket.
int NetRecvTCP(Socket sockfd, void* buf, int len)
{
}

	/// Listen for connections on a TCP socket
int NetListenTCP(Socket sockfd)
{
}

	/// Accept a connection on a TCP socket
Socket NetAcceptTCP(Socket sockfd)
{
}

#else //not SDLA

#ifdef USE_WINSOCK // {

/**
** Hardware dependend network init.
*/
int NetInit(void)
{
	WSADATA wsaData;

	// Start up the windows networking
	// ARI: well, I need winsock2 for SIO_GET_INTERFACE_LIST..
	// some day this needs to be rewritten using wsock32.dll's WsControl(),
	// so that we can support Windows 95 with only winsock 1.1..
	// For now ws2_32.dll has to do..
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		fprintf(stderr, "Couldn't initialize Winsock 2\n");
		return -1;
	}
#if 0 // sorry, Winsock 1 not sufficient yet //
	if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
		fprintf(stderr, "Couldn't initialize Winsock 1.1\n");
		return -1;
	}
#endif
	return 0;
}

/**
** Hardware dependend network exit.
*/
void NetExit(void)
{
	// Clean up windows networking
	if (WSACleanup() == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEINPROGRESS) {
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
}

/**
** Close an UDP socket port.
**
** @param sockfd Socket fildes
*/
void NetCloseUDP(Socket sockfd)
{
	closesocket(sockfd);
}

/**
** Close a TCP socket port.
**
** @param sockfd Socket fildes
*/
void NetCloseTCP(Socket sockfd)
{
	closesocket(sockfd);
}

#endif // } !USE_WINSOCK

#if !defined(USE_WINSOCK) // {

/**
** Hardware dependend network init.
*/
int NetInit(void)
{
	return 0;
}

/**
** Hardware dependend network exit.
*/
void NetExit(void)
{
}

/**
** Close an UDP socket port.
**
** @param sockfd Socket fildes
*/
void NetCloseUDP(Socket sockfd)
{
	close(sockfd);
}

/**
** Close a TCP socket port.
**
** @param sockfd Socket fildes
*/
void NetCloseTCP(Socket sockfd)
{
	close(sockfd);
}

#endif // } !USE_WINSOCK

/**
** Set socket to non-blocking.
**
** @param sockfd Socket
**
** @return 0 for success, -1 for error
*/
#ifdef USE_WINSOCK
int NetSetNonBlocking(Socket sockfd)
{
	unsigned long opt;

	opt = 1;
	return ioctlsocket(sockfd, FIONBIO, &opt);
}
#else
int NetSetNonBlocking(Socket sockfd)
{
	int flags;

	flags = fcntl(sockfd, F_GETFL, 0);
	return fcntl(sockfd, F_SETFL,flags | O_NONBLOCK);
}
#endif

/**
** Resolve host in name or dotted quad notation.
**
** @param host Host name (f.e. 192.168.0.0 or stratagus.net)
*/
unsigned long NetResolveHost(const char* host)
{
	unsigned long addr;

	if (host) {
		addr = inet_addr(host); // try dot notation
		if (addr == INADDR_NONE) {
			struct hostent *he;

			he = 0;
			he = gethostbyname(host);
			if (he) {
				addr = 0;
				Assert(he->h_length == 4);
				memcpy(&addr, he->h_addr, he->h_length);
			}
		}
		return addr;
	}
	return INADDR_NONE;
}

/**
** Get IP-addrs of local interfaces from Network file descriptor
** and store them in the NetLocalAddrs array.
**
** @param sock local socket.
**
** @return number of IP-addrs found.
*/
#ifdef USE_WINSOCK // {
// ARI: MS documented this for winsock2, so I finally found it..
// I also found a way for winsock1.1 (= win95), but
// that one was too complex to start with.. -> trouble
// Lookout for INTRFC.EXE on the MS web site...
int NetSocketAddr(const Socket sock)
{
	INTERFACE_INFO localAddr[MAX_LOC_IP];  // Assume there will be no more than MAX_LOC_IP interfaces
	DWORD bytesReturned;
	SOCKADDR_IN* pAddrInet;
	u_long SetFlags;
	int i;
	int nif;
	int wsError;
	int numLocalAddr;

	nif = 0;
	if (sock != (Socket)-1) {
		wsError = WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, &localAddr,
			sizeof(localAddr), &bytesReturned, NULL, NULL);
		if (wsError == SOCKET_ERROR) {
			DebugPrint("SIOCGIFCONF:WSAIoctl(SIO_GET_INTERFACE_LIST) - errno %d\n" _C_
				WSAGetLastError());
		}

		// parse interface information
		numLocalAddr = (bytesReturned / sizeof(INTERFACE_INFO));
		for (i = 0; i < numLocalAddr; ++i) {
			SetFlags = localAddr[i].iiFlags;
			if ((SetFlags & IFF_UP) == 0) {
				continue;
			}
			if ((SetFlags & IFF_LOOPBACK)) {
				continue;
			}
			pAddrInet = (SOCKADDR_IN*)&localAddr[i].iiAddress;
			NetLocalAddrs[nif] = pAddrInet->sin_addr.s_addr;
			++nif;
			if (nif == MAX_LOC_IP) {
				break;
			}
		}
	}
	return nif;
}
#else // } { !USE_WINSOCK
#ifdef unix // {
// ARI: I knew how to write this for a unix environment,
// but am quite certain that porting this can cause you
// trouble..
int NetSocketAddr(const Socket sock)
{
	char buf[4096];
	char* cp;
	char* cplim;
	struct ifconf ifc;
	struct ifreq ifreq;
	struct ifreq* ifr;
	struct sockaddr_in *sap;
	struct sockaddr_in sa;
	int i;
	int nif;

	nif = 0;
	if (sock != (Socket)-1) {
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		if (ioctl(sock, SIOCGIFCONF, (char*)&ifc) < 0) {
			DebugPrint("SIOCGIFCONF - errno %d\n" _C_ errno);
			return 0;
		}
		// with some inspiration from routed..
		ifr = ifc.ifc_req;
		cplim = buf + ifc.ifc_len; // skip over if's with big ifr_addr's
		for (cp = buf; cp < cplim;
				cp += sizeof(ifr->ifr_name) + sizeof(ifr->ifr_ifru)) {
			ifr = (struct ifreq*)cp;
			ifreq = *ifr;
			if (ioctl(sock, SIOCGIFFLAGS, (char*)&ifreq) < 0) {
				DebugPrint("%s: SIOCGIFFLAGS - errno %d\n" _C_
					ifr->ifr_name _C_ errno);
				continue;
			}
			if ((ifreq.ifr_flags & IFF_UP) == 0 ||
					ifr->ifr_addr.sa_family == AF_UNSPEC) {
				continue;
			}
			// argh, this'll have to change sometime
			if (ifr->ifr_addr.sa_family != AF_INET) {
				continue;
			}
			if (ifreq.ifr_flags & IFF_LOOPBACK) {
				continue;
			}
			sap = (struct sockaddr_in*)&ifr->ifr_addr;
			sa = *sap;
			NetLocalAddrs[nif] = sap->sin_addr.s_addr;
			if (ifreq.ifr_flags & IFF_POINTOPOINT) {
				if (ioctl(sock, SIOCGIFDSTADDR, (char*)&ifreq) < 0) {
					DebugPrint("%s: SIOCGIFDSTADDR - errno %d\n" _C_
						ifr->ifr_name _C_ errno);
					// failed to obtain dst addr - ignore
					continue;
				}
				if (ifr->ifr_addr.sa_family == AF_UNSPEC) {
					continue;
				}
			}
			// avoid p-t-p links with common src
			if (nif) {
				for (i = 0; i < nif; ++i) {
					if (sa.sin_addr.s_addr == NetLocalAddrs[i]) {
						i = -1;
						break;
					}
				}
				if (i == -1) {
					continue;
				}
			}
			++nif;
			if (nif == MAX_LOC_IP) {
				break;
			}
		}
	}
	return nif;
}
#else // } !unix
// Beos?? Mac??
int NetSocketAddr(const Socket sock)
{
	NetLocalAddrs[0] = htonl(0x7f000001);
	return 1;
}
#endif
#endif // } !USE_WINSOCK

/**
** Open an UDP Socket port.
**
** @param port !=0 Port to bind in host notation.
**
** @return If success the socket fildes, -1 otherwise.
*/
Socket NetOpenUDP(int port)
{
	Socket sockfd;

	// open the socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == INVALID_SOCKET) {
		return -1;
	}
	// bind local port
	if (port) {
		struct sockaddr_in sock_addr;

		memset(&sock_addr, 0, sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = INADDR_ANY;
		sock_addr.sin_port = htons(port);
		// Bind the socket for listening
		if (bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
			fprintf(stderr, "Couldn't bind to local port\n");
			NetCloseUDP(sockfd);
			return -1;
		}
		NetLastHost = sock_addr.sin_addr.s_addr;
		NetLastPort = sock_addr.sin_port;
	}
	return sockfd;
}

/**
** Open a TCP socket
**
** @param port Bind socket to a specific port number
**
** @return If success the socket fildes, -1 otherwise
*/
Socket NetOpenTCP(int port)
{
	Socket sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == INVALID_SOCKET) {
		return (Socket)-1;
	}
	// bind local port
	if (port) {
		struct sockaddr_in sock_addr;
		int opt;

		memset(&sock_addr, 0, sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = INADDR_ANY;
		sock_addr.sin_port = htons(port);

		opt = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));

		if (bind(sockfd,(struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
			fprintf(stderr, "Couldn't bind to local port\n");
			NetCloseTCP(sockfd);
			return (Socket)-1;
		}
		NetLastHost = sock_addr.sin_addr.s_addr;
		NetLastPort = sock_addr.sin_port;
	}
	NetLastSocket = sockfd;
	return sockfd;
}

/**
** Open a TCP connection
**
** @param sockfd  An open socket to use
** @param addr    Address returned from NetResolveHost
** @param port    Port on remote host to connect to
**
** @return 0 if success, -1 if failure
*/
int NetConnectTCP(Socket sockfd, unsigned long addr, int port)
{
	struct sockaddr_in sa;
#ifndef __BEOS__
	int opt;

	opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&opt, sizeof(opt));
	opt = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void*)&opt, sizeof(opt));
#endif

	if (addr == INADDR_NONE) {
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	memcpy(&sa.sin_addr, &addr, sizeof(addr));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		fprintf(stderr, "connect to %d.%d.%d.%d:%d failed\n",
			NIPQUAD(ntohl(addr)), port);
		return -1;
	}

	return sockfd;
}

/**
** Wait for socket ready.
**
** @param sockfd   Socket fildes to probe.
** @param timeout  Timeout in 1/1000 seconds.
**
** @return 1 if data is available, 0 if not, -1 if failure.
*/
int NetSocketReady(Socket sockfd, int timeout)
{
	int retval;
	struct timeval tv;
	fd_set mask;

	// Check the file descriptors for available data
	do {
		// Set up the mask of file descriptors
		FD_ZERO(&mask);
		FD_SET(sockfd, &mask);

		// Set up the timeout
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		// Data available?
		retval = select(sockfd + 1, &mask, NULL, NULL, &tv);
#ifdef USE_WINSOCK
	} while (retval == SOCKET_ERROR && WSAGetLastError() == WSAEINTR);
#else
	} while (retval == -1 && errno == EINTR);
#endif

	return retval;
}

/**
** Receive from a UDP socket.
**
** @param sockfd   Socket
** @param buf      Receive message buffer.
** @param len      Receive message buffer length.
**
** @return Number of bytes placed in buffer, or -1 if failure.
*/
int NetRecvUDP(Socket sockfd, void* buf, int len)
{
	int n;
	int l;
	struct sockaddr_in sock_addr;

	n = sizeof(struct sockaddr_in);
	if ((l = recvfrom(sockfd, buf, len, 0, (struct sockaddr*)&sock_addr, &n)) < 0) {
		PrintFunction();
		fprintf(stdout, "Could not read from UDP socket\n");
		return -1;
	}

	// FIXME: ARI: verify that it _really_ is from one of our hosts...
	// imagine what happens when an udp port scan hits the port...

	NetLastHost = sock_addr.sin_addr.s_addr;
	NetLastPort = sock_addr.sin_port;

	return l;
}

/**
** Receive from a TCP socket.
**
** @param sockfd   Socket
** @param buf      Receive message buffer.
** @param len      Receive message buffer length.
**
** @return Number of bytes placed in buffer or -1 if failure.
*/
int NetRecvTCP(Socket sockfd, void* buf, int len)
{
	NetLastSocket = sockfd;
	return recv(sockfd, buf, len, 0);
}

/**
** Send through a UPD socket to a host:port.
**
** @param sockfd    Socket
** @param host      Host to send to (network byte order).
** @param port      Port of host to send to (network byte order).
** @param buf       Send message buffer.
** @param len       Send message buffer length.
**
** @return Number of bytes sent.
*/
int NetSendUDP(Socket sockfd,unsigned long host, int port,
	const void* buf, int len)
{
	int n;
	struct sockaddr_in sock_addr;

	n = sizeof(struct sockaddr_in);
	sock_addr.sin_addr.s_addr = host;
	sock_addr.sin_port = port;
	sock_addr.sin_family = AF_INET;

	// if (MyRand() % 7) { return 0; }

	return sendto(sockfd, buf, len, 0, (struct sockaddr*)&sock_addr, n);
}

/**
** Send through a TCP socket.
**
** @param sockfd    Socket
** @param buf       Send message buffer.
** @param len       Send message buffer length.
**
** @return Number of bytes sent.
*/
int NetSendTCP(Socket sockfd, const void* buf, int len)
{
	return send(sockfd, buf, len, 0);
}

/**
** Listen for connections on a TCP socket.
**
** @param sockfd    Socket
**
** @return 0 for success, -1 for error
*/
int NetListenTCP(Socket sockfd)
{
	return listen(sockfd, PlayerMax);
}

/**
** Accept a connection on a TCP socket.
**
** @param sockfd Socket
**
** @return If success the new socket fildes, -1 otherwise.
*/
Socket NetAcceptTCP(Socket sockfd)
{
	struct sockaddr_in sa;
	int len;

	len = sizeof(struct sockaddr_in);
	NetLastSocket = accept(sockfd, (struct sockaddr*)&sa, &len);
	NetLastHost = sa.sin_addr.s_addr;
	NetLastPort = sa.sin_port;
	return NetLastSocket;
}

#endif

//@}
