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
/**@name trigger.h - The game trigger headerfile. */
//
// (c) Copyright 2002-2003 by Lutz Sammer and Jimmy Salmon
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
// $Id: trigger.h,v 1.21 2004/06/24 16:35:02 jarod42 Exp $

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include "script.h"

/*----------------------------------------------------------------------------
-- Declarations
----------------------------------------------------------------------------*/

/**
** Timer structure
*/
typedef struct _timer_ {
	char Init;                  ///< timer is initialized
	char Running;               ///< timer is running
	char Increasing;            ///< increasing or decreasing
	long Cycles;                ///< current value in game cycles
	unsigned long LastUpdate;   ///< GameCycle of last update
} Timer;

#define ANY_UNIT ((const UnitType*)0)
#define ALL_UNITS ((const UnitType*)-1)
#define ALL_FOODUNITS ((const UnitType*)-2)
#define ALL_BUILDINGS ((const UnitType*)-3)

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern Timer GameTimer; ///< the game timer

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern int TriggerGetPlayer(lua_State* l);///< get player number.
extern const UnitType* TriggerGetUnitType(lua_State* l); ///< get the unit-type
extern void TriggersEachCycle(void);    ///< test triggers

extern void TriggerCclRegister(void);   ///< Register ccl features
extern void SaveTriggers(CLFile* file); ///< Save the trigger module
extern void InitTriggers(void);         ///< Setup triggers
extern void CleanTriggers(void);        ///< Cleanup the trigger module

//@}

#endif // !__TRIGGER_H__
