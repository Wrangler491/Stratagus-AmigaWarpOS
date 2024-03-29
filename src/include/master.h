//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//      Stratagus - A free fantasy real time strategy game engine
//
/**@name master.h - The master server headerfile. */
//
//      (c) Copyright 2003-2004 by Tom Zickel and Jimmy Salmon
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
//      $Id: master.h,v 1.21 2004/06/23 22:42:03 jarod42 Exp $

#ifndef __MASTER_H__
#define __MASTER_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

/// @todo do it configurable.
#define MASTER_HOST "mohydine.no-ip.com"
#define MASTER_PORT 7775

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern int MetaServerInUse;

extern int MetaInit(void);
extern int MetaClose(void);
extern int MetaServerOK(char* reply);
extern int SendMetaCommand(char* command, char* format, ...);
extern int RecvMetaReply(char** reply);
extern int GetMetaParameter(char* reply, int pos, char** value);

//@}

#endif // !__MASTER_H__
