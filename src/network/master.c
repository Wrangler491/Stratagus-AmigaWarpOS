//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
// T H E   W A R   B E G I N S
// Stratagus - A free fantasy real time strategy game engine
//
/**@name master.c - The master server. */
//
// (c) Copyright 2003 by Tom Zickel and Jimmy Salmon
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
// $Id: master.c,v 1.29 2004/06/24 18:07:53 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>

#ifndef _MSC_VER
#include <fcntl.h>
#endif

#include "stratagus.h"

#include "SDL.h"

#include "iocompat.h"
#include "network.h"
#include "netconnect.h"
#include "script.h"
#include "master.h"
#include "net_lowlevel.h"


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//###### For Magnant META SERVER
static Socket MetaServerFildes;  // This is a TCP socket.
int MetaServerInUse;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


/**
**  Initialize the TCP connection to the Meta Server.
**
**  @return  -1 fail, 0 success.
**  @todo Make a dynamic port allocation.
*/
int MetaInit(void)
{
	int i;
	char* reply;
	int port_range_min;
	int port_range_max;

	port_range_min = 1234;
	port_range_max = 1244;
	reply = NULL;
	MetaServerFildes = NetworkFildes;
	for (i = port_range_min; i < port_range_max; ++i) {
		MetaServerFildes = NetOpenTCP(i);  //FIXME: need to make a dynamic port allocation there...if (!MetaServerFildes) {...}
		if (MetaServerFildes != (Socket)-1) {
			if (NetConnectTCP(MetaServerFildes, NetResolveHost(MASTER_HOST), MASTER_PORT) != -1) {
				break;
			}
		}
	}
	if (i == port_range_max) {
		return -1;
	}

	if (SendMetaCommand("Login", "") == -1) {
		return -1;
	}

	if (RecvMetaReply(&reply) == -1) {
		return -1;
	} else {
		if (MetaServerOK(reply)) {
			free(reply);
			return 0;
		} else {
			free(reply);
			return -1;
		}
	}

	return 0;
}

/**
**  Close Connection to Master Server
**
**  @return  nothing
*/
int MetaClose(void)
{
	NetCloseTCP(MetaServerFildes);
	return 0;
}

/**
**  Checks if a Message was OK or ERR
**
**  @return 1 OK, 0 Error.
*/
int MetaServerOK(char* reply)
{
	return !strcmp("OK\r\n", reply) || !strcmp("OK\n", reply);
}

/**
**  Retrieves the value of the parameter at position paramNumber
**
**  @param reply    The reply from the metaserver
**  @param pos      The parameter number
**  @param value    The returned value
**
**  @returns -1 if error.
*/
int GetMetaParameter(char* reply, int pos, char** value)
{
	char* endline;

	// Take Care for OK/ERR
	*value = strchr(reply, '\n');
	(*value)++;

	while (pos-- && *value) {
		(*value) = strchr((*value), '\n');
		if (*value) {
			(*value)++;
		}
	}

	if (!*value) {
		// Parameter our of bounds
		return -1;
	}

	if (*value[0] == '\n') {
		(*value)++;
	}

	endline = strchr(*value, '\n');

	if (!endline) {
		return -1;
	}

	*endline = '\0';
	*value = strdup(*value);
	*endline = '\n';
	return 0;
}


/**
**  Send a command to the meta server
**
**  @param command   command to send
**  @param format    format of parameters
**  @param ...       parameters
**
**  @returns  -1 fail, length of command
*/
int SendMetaCommand(char* command, char* format, ...)
{
	int n;
	int size;
	int ret;
	char* p;
	char* s;
	va_list ap;

	size = strlen(GameName) + strlen(LocalPlayerName) + strlen(command) + 100;
	ret = -1;
	if ((p = malloc(size)) == NULL) {
		return -1;
	}
	if ((s = malloc(size)) == NULL) {
		return -1;
	}

	// Message Structure
	// <Stratagus> if for Magnant Compatibility, it may be removed
	// Player Name, Game Name, VERSION, Command, **Paramaters**
	sprintf(s, "<Stratagus>\n%s\n%s\n%s\n%s\n",
		LocalPlayerName, GameName, VERSION, command);

	// Commands
	// Login - password
	// Logout - 0
	// AddGame - IP,Port,Description,Map,Players,FreeSpots
	// JoinGame - Nick of Hoster
	// ChangeGame - Description,Map,Players,FreeSpots
	// GameList - 0
	// NextGameInList - 0
	// StartGame - 0
	// PlayerScore - Player,Score,Win (Add razings...)
	// EndGame - Called after PlayerScore.
	// AbandonGame - 0
	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, format);
		n = vsnprintf(p, size, format, ap);
		va_end(ap);
		/* If that worked, string was processed. */
		if (n > -1 && n < size) {
			break;
		}
		/* Else try again with more space. */
		if (n > -1) { /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		} else {              /* glibc 2.0 */
			size *= 2;    /* twice the old size */
		}
		if ((p = realloc(p, size)) == NULL) {
			return -1;
		}
	}
	// Allocate the correct size
	if ((s = realloc(s, size + strlen(s))) == NULL ) {
		free(p);
		return -1;
	}
	strcat(s, p);
	size = strlen(s);
	ret = NetSendTCP(MetaServerFildes, s, size);
	free(p);
	free(s);
	return ret;
}

/**
**  Receive reply from Meta Server
**
**  @param  reply  Text of the reply
**
**  @return error or number of bytes
*/
int RecvMetaReply(char** reply)
{
	int n;
	char* p;
	char buf[1024];

	if (NetSocketReady(MetaServerFildes, 5000) == -1) {
		return -1;
	}

	p = NULL;

	// FIXME: Allow for large packets
	n = NetRecvTCP(MetaServerFildes, &buf, 1024);
	if (!(p = malloc(n + 1))) {
		return -1;
	}

	// We know we now have the whole command.
	// Convert to standard notation
	buf[n - 1] = '\0';
	buf[n - 2] = '\n';
	strcpy(p, buf);

	*reply = p;
	return n;
}

//@}
