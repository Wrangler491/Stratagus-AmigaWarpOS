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
/**@name net_lowlevel.h - The network low level header file. */
//
//      (c) Copyright 1998-2004 by Lutz Sammer
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
//      $Id: net_lowlevel.h,v 1.37 2004/04/06 21:50:02 jarod42 Exp $

#ifndef __NET_LOWLEVEL_H
#define __NET_LOWLEVEL_H

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/
#ifdef BSDAMIGA
#include <devices/timer.h>
#endif

#ifdef USE_SDLA
#include <SDL_net.h>
#endif


#ifndef _MSC_VER
#include <errno.h>
#include <time.h>
#endif

// Include system network headers
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32)

#define USE_WINSOCK

#ifdef __MINGW32__
#define DrawIcon WinDrawIcon
#define EndMenu WinEndMenu
#endif

#include <winsock2.h>

#include <windows.h>
#include <winsock.h>
//#include <ws2tcpip.h>

#ifdef __MINGW32__
#undef DrawIcon
#undef EndMenu
#undef FindResource
#endif

// MS Knowledge base fix for SIO_GET_INTERFACE_LIST with NT4.0 ++
#define SIO_GET_INTERFACE_LIST 0x4004747F
#define IFF_UP 1
#define IFF_LOOPBACK 4
typedef struct _OLD_INTERFACE_INFO
{
  unsigned long iiFlags; /* Interface flags */
  SOCKADDR   iiAddress;  /* Interface address */
  SOCKADDR   iiBroadcastAddress; /* Broadcast address */
  SOCKADDR   iiNetmask;  /* Network mask */
} OLD_INTERFACE_INFO;
#define INTERFACE_INFO OLD_INTERFACE_INFO

#else // UNIX
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifndef __BEOS__
#include <net/if.h>
#include <arpa/inet.h>
#endif
#define INVALID_SOCKET -1
#endif // !WIN32

#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define NIPQUAD(ad) \
	(int)(((ad) >> 24) & 0xff), (int)(((ad) >> 16) & 0xff), \
	(int)(((ad) >> 8) & 0xff), (int)((ad) & 0xff)

#ifdef USE_WIN32
typedef SOCKET Socket;
#else
typedef int Socket;
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern int NetLastSocket;              ///< Last socket
extern unsigned long NetLastHost;      ///< Last host number (net format)
extern int NetLastPort;                ///< Last port number (net format)
extern unsigned long NetLocalAddrs[];  ///< Local IP-Addrs of this host (net format)

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

	/// Hardware dependend network init.
extern int NetInit(void);
	/// Hardware dependend network exit.
extern void NetExit(void);
	/// Resolve host in name or or colon dot notation.
extern unsigned long NetResolveHost(const char* host);
	/// Get local IP from network file descriptor
extern int NetSocketAddr(const Socket sock);
	/// Open a UDP Socket port.
extern Socket NetOpenUDP(int port);
	/// Open a TCP Socket port.
extern Socket NetOpenTCP(int port);
	/// Close a UDP socket port.
extern void NetCloseUDP(Socket sockfd);
	/// Close a TCP socket port.
extern void NetCloseTCP(Socket sockfd);
	/// Set socket to non-blocking
extern int NetSetNonBlocking(Socket sockfd);
	/// Open a TCP connection.
extern int NetConnectTCP(Socket sockfd, unsigned long addr, int port);
	/// Send through a UPD socket to a host:port.
extern int NetSendUDP(Socket sockfd, unsigned long host, int port
	, const void* buf, int len);
	/// Send through a TCP socket
extern int NetSendTCP(Socket sockfd, const void* buf, int len);
	/// Wait for socket ready.
extern int NetSocketReady(Socket sockfd, int timeout);
	/// Receive from a UDP socket.
extern int NetRecvUDP(Socket sockfd, void* buf, int len);
	/// Receive from a TCP socket.
extern int NetRecvTCP(Socket sockfd, void* buf, int len);
	/// Listen for connections on a TCP socket
extern int NetListenTCP(Socket sockfd);
	/// Accept a connection on a TCP socket
extern Socket NetAcceptTCP(Socket sockfd);

//@}

#endif // !__NET_LOWLEVEL_H
