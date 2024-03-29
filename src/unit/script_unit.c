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
/**@name script_unit.c - The unit ccl functions. */
//
//      (c) Copyright 2001-2004 by Lutz Sammer and Jimmy Salmon
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
//      $Id: script_unit.c,v 1.120 2004/06/26 13:51:17 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "script.h"
#include "spells.h"
#include "pathfinder.h"
#include "trigger.h"
#include "actions.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

	/// Get resource by name
extern unsigned CclGetResourceByName(lua_State* l);

/**
**  Set xp damage
**
**  @param l  Lua state.
**
**  @return   The old state of the xp damage
*/
static int CclSetXpDamage(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	old = XpDamage;
	XpDamage = LuaToBoolean(l, 1);

	lua_pushboolean(l, old);
	return 1;
}

/**
**  Set training queue
**
**  @param l  Lua state.
**
**  @return  The old state of the training queue
*/
static int CclSetTrainingQueue(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	old = EnableTrainingQueue;
	EnableTrainingQueue = LuaToBoolean(l, 1);

	lua_pushboolean(l, old);
	return 1;
}

/**
**  Set capture buildings
**
**  @param l  Lua state.
**
**  @return   The old state of the flag
*/
static int CclSetBuildingCapture(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	old = EnableBuildingCapture;
	EnableBuildingCapture = LuaToBoolean(l, 1);

	lua_pushboolean(l, old);
	return 1;
}

/**
**  Set reveal attacker
**
**  @param l  Lua state.
**
**  @return   The old state of the flag
*/
static int CclSetRevealAttacker(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	old = RevealAttacker;
	RevealAttacker = LuaToBoolean(l, 1);

	lua_pushboolean(l, old);
	return 1;
}

/**
**  Get a unit pointer
**
**  @param l  Lua state.
**
**  @return   The unit pointer
*/
static Unit* CclGetUnit(lua_State* l)
{
	return UnitSlots[(int)LuaToNumber(l, -1)];
}

/**
**  Parse order
**
**  @param l      Lua state.
**  @param order  OUT: resulting order.
*/
void CclParseOrder(lua_State* l, Order* order)
{
	const char* value;
	int args;
	int j;

	//
	// Parse the list: (still everything could be changed!)
	//
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		if (!strcmp(value, "action-none")) {
			order->Action = UnitActionNone;
		} else if (!strcmp(value, "action-still")) {
			order->Action = UnitActionStill;
		} else if (!strcmp(value, "action-stand-ground")) {
			order->Action = UnitActionStandGround;
		} else if (!strcmp(value, "action-follow")) {
			order->Action = UnitActionFollow;
		} else if (!strcmp(value, "action-move")) {
			order->Action = UnitActionMove;
		} else if (!strcmp(value, "action-attack")) {
			order->Action = UnitActionAttack;
		} else if (!strcmp(value, "action-attack-ground")) {
			order->Action = UnitActionAttackGround;
		} else if (!strcmp(value, "action-die")) {
			order->Action = UnitActionDie;
		} else if (!strcmp(value, "action-spell-cast")) {
			order->Action = UnitActionSpellCast;
		} else if (!strcmp(value, "action-train")) {
			order->Action = UnitActionTrain;
		} else if (!strcmp(value, "action-upgrade-to")) {
			order->Action = UnitActionUpgradeTo;
		} else if (!strcmp(value, "action-research")) {
			order->Action = UnitActionResearch;
		} else if (!strcmp(value, "action-builded")) {
			order->Action = UnitActionBuilded;
		} else if (!strcmp(value, "action-board")) {
			order->Action = UnitActionBoard;
		} else if (!strcmp(value, "action-unload")) {
			order->Action = UnitActionUnload;
		} else if (!strcmp(value, "action-patrol")) {
			order->Action = UnitActionPatrol;
		} else if (!strcmp(value, "action-build")) {
			order->Action = UnitActionBuild;
		} else if (!strcmp(value, "action-repair")) {
			order->Action = UnitActionRepair;
		} else if (!strcmp(value, "action-resource")) {
			order->Action = UnitActionResource;
		} else if (!strcmp(value, "action-return-goods")) {
			order->Action = UnitActionReturnGoods;

		} else if (!strcmp(value, "flags")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Flags = LuaToNumber(l, -1);
			lua_pop(l, 1);

		} else if (!strcmp(value, "range")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Range = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "min-range")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->MinRange = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "width")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Width = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "height")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Height = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "goal")) {
			int slot;

			++j;
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);

			slot = strtol(value + 1, NULL, 16);
			order->Goal = UnitSlots[slot];
			if (!UnitSlots[slot]) {
				DebugPrint("FIXME: Forward reference not supported\n");
			}
			//++UnitSlots[slot]->Refs;

		} else if (!strcmp(value, "tile")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, -1, 1);
			order->X = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 2);
			order->Y = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_pop(l, 1);

		} else if (!strcmp(value, "type")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Type = UnitTypeByIdent(LuaToString(l, -1));
			lua_pop(l, 1);

		} else if (!strcmp(value, "patrol")) {
			int x1;
			int x2;

			++j;
			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, -1, 1);
			x1 = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 2);
			x2 = LuaToNumber(l, -1);
			lua_pop(l, 1);
			order->Arg1 = (void*)((x1 << 16) | x2);
			lua_pop(l, 1);

		} else if (!strcmp(value, "spell")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Arg1 = SpellTypeByIdent(LuaToString(l, -1));
			lua_pop(l, 1);

		} else if (!strcmp(value, "upgrade")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Arg1 = UpgradeByIdent(LuaToString(l, -1));
			lua_pop(l, 1);

		} else if (!strcmp(value, "mine")) {
			int x1;
			int x2;

			++j;
			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, -1, 1);
			x1 = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 2);
			x2 = LuaToNumber(l, -1);
			lua_pop(l, 1);
			order->Arg1 = (void*)((x1 << 16) | x2);
			lua_pop(l, 1);

		} else if (!strcmp(value, "arg1")) {
			++j;
			lua_rawgeti(l, -1, j + 1);
			order->Arg1 = (void*)(int)LuaToNumber(l, -1);
			lua_pop(l, 1);

		} else {
		   // This leaves a half initialized unit
		   LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
}

/**
**  Parse orders.
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should get the orders.
*/
static void CclParseOrders(lua_State* l, Unit* unit)
{
	int args;
	int j;

	args = luaL_getn(l, -1);
	Assert(args == MAX_ORDERS);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		CclParseOrder(l, &unit->Orders[j]);
		lua_pop(l, 1);
	}
}

/**
**  Parse builded
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseBuilded(lua_State* l, Unit* unit)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "worker")) {
			int slot;

			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			slot = strtol(value + 1, NULL, 16);
			Assert(UnitSlots[slot]);
			unit->Data.Builded.Worker = UnitSlots[slot];
			//++UnitSlots[slot]->Refs;
		} else if (!strcmp(value, "progress")) {
			lua_rawgeti(l, -1, j + 1);
			unit->Data.Builded.Progress = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "cancel")) {
			unit->Data.Builded.Cancel = 1;
			--j;
		} else if (!strcmp(value, "frame")) {
			int frame;
			ConstructionFrame* cframe;

			lua_rawgeti(l, -1, j + 1);
			frame = LuaToNumber(l, -1);
			lua_pop(l, 1);
			cframe = unit->Type->Construction->Frames;
			while (frame--) {
				cframe = cframe->Next;
			}
			unit->Data.Builded.Frame = cframe;
		}
	}
}

/**
**  Parse res worker data
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseResWorker(lua_State* l, Unit* unit)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "time-to-harvest")) {
			lua_rawgeti(l, -1, j + 1);
			unit->Data.ResWorker.TimeToHarvest = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "done-harvesting")) {
			unit->Data.ResWorker.DoneHarvesting = 1;
			--j;
		}
	}
}

/**
**  Parse research
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseResearch(lua_State* l, Unit* unit)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "ident")) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			unit->Data.Research.Upgrade = UpgradeByIdent(value);
		}
	}
}

/**
**  Parse upgrade to
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseUpgradeTo(lua_State* l, Unit* unit)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "ticks")) {
			lua_rawgeti(l, -1, j + 1);
			unit->Data.UpgradeTo.Ticks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		}
	}
}

/**
**  Parse stored data for train order
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseTrain(lua_State* l, Unit* unit)
{
	const char* value;
	int i;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "ticks")) {
			lua_rawgeti(l, -1, j + 1);
			unit->Data.Train.Ticks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "count")) {
			lua_rawgeti(l, -1, j + 1);
			unit->Data.Train.Count = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "queue")) {
			int subargs;
			int k;

			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, -1);
			for (i = 0, k = 0; i < MAX_UNIT_TRAIN && k < subargs; ++i, ++k) {
				lua_rawgeti(l, -1, k + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				if (!strcmp(value, "unit-none")) {
					unit->Data.Train.What[i] = NULL;
				} else {
					unit->Data.Train.What[i] = UnitTypeByIdent(value);
				}
			}
			lua_pop(l, 1);
		}
	}
}

/**
**  Parse stored data for move order
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should be filled with the data.
*/
static void CclParseMove(lua_State* l, Unit* unit)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "fast")) {
			unit->Data.Move.Fast = 1;
			--j;
		} else if (!strcmp(value, "path")) {
			int subargs;
			int k;

			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, -1);
			for (k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				unit->Data.Move.Path[k] = LuaToNumber(l, -1);
				lua_pop(l, 1);
			}
			unit->Data.Move.Length = subargs;
			lua_pop(l, 1);
		}
	}
}

/**
**  Parse unit
**
**  @param l  Lua state.
*/
static int CclUnit(lua_State* l)
{
	const char* value;
	Unit* unit;
	UnitType* type;
	UnitType* seentype;
	Player* player;
	int slot;
	int i;
	const char* s;
	int args;
	int j;

	args = lua_gettop(l);
	j = 0;

	slot = LuaToNumber(l, j + 1);
	++j;

	unit = NULL;
	type = NULL;
	seentype = NULL;
	player = NULL;
	i = 0;

	//
	// Parse the list: (still everything could be changed!)
	//
	for (; j < args; ++j) {
		value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "type")) {
			type = UnitTypeByIdent(LuaToString(l, j + 1));
		} else if (!strcmp(value, "seen-type")) {
			seentype = UnitTypeByIdent(LuaToString(l, j + 1));
		} else if (!strcmp(value, "player")) {
			player = &Players[(int)LuaToNumber(l, j + 1)];

			// During a unit's death animation (when action is "die" but the
			// unit still has its original type, i.e. it's still not a corpse)
			// the unit is already removed from map and from player's
			// unit list (=the unit went through LetUnitDie() which
			// calls RemoveUnit() and UnitLost()).  Such a unit should not
			// be put on player's unit list!  However, this state is not
			// easily detected from this place.  It seems that it is
			// characterized by unit->HP==0 and by
			// unit->Orders[0].Action==UnitActionDie so we have to wait
			// until we parsed at least Unit::Orders[].
			Assert(type);
			unit = UnitSlots[slot];
			InitUnit(unit, type);
			unit->Seen.Type = seentype;
			unit->Active = 0;
			unit->Removed = 0;
			unit->Reset = 0; // JOHNS ????
			Assert(unit->Slot == slot);
		} else if (!strcmp(value, "next")) {
			unit->Next = UnitSlots[(int)LuaToNumber(l, j + 1)];
		} else if (!strcmp(value, "current-sight-range")) {
			unit->CurrentSightRange = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "refs")) {
			unit->Refs = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "host-info")) {
			int x;
			int y;
			int w;
			int h;

			if (!lua_istable(l, j + 1) || luaL_getn(l, j + 1) != 4) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, j + 1, 1);
			x = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 2);
			y = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 3);
			w = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 4);
			h = LuaToNumber(l, -1);
			lua_pop(l, 1);
			MapMarkSight(player, x, y, w, h, unit->CurrentSightRange);
		} else if (!strcmp(value, "tile")) {
			if (!lua_istable(l, j + 1) || luaL_getn(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, j + 1, 1);
			unit->X = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 2);
			unit->Y = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "stats")) {
			unit->Stats = &type->Stats[(int)LuaToNumber(l, j + 1)];
		} else if (!strcmp(value, "pixel")) {
			if (!lua_istable(l, j + 1) || luaL_getn(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, j + 1, 1);
			unit->IX = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 2);
			unit->IY = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "seen-pixel")) {
			if (!lua_istable(l, j + 1) || luaL_getn(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, j + 1, 1);
			unit->Seen.IX = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 2);
			unit->Seen.IY = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "frame")) {
			unit->Frame = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "seen")) {
			unit->Seen.Frame = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "not-seen")) {
			unit->Seen.Frame = UnitNotSeen;
			--j;
		} else if (!strcmp(value, "direction")) {
			unit->Direction = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "attacked")) {
			// FIXME : unsigned long should be better handled
			unit->Attacked = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "burning")) {
			unit->Burning = 1;
			--j;
		} else if (!strcmp(value, "destroyed")) {
			unit->Destroyed = 1;
			--j;
		} else if (!strcmp(value, "removed")) {
			unit->Removed = 1;
			--j;
		} else if (!strcmp(value, "selected")) {
			unit->Selected = 1;
			--j;
		} else if (!strcmp(value, "rescued-from")) {
			unit->RescuedFrom = &Players[(int)LuaToNumber(l, j + 1)];
		} else if (!strcmp(value, "seen-by-player")) {
			s = LuaToString(l, j + 1);
			unit->Seen.ByPlayer = 0;
			for (i = 0; i < PlayerMax && *s; ++i, ++s) {
				if (*s == '-' || *s == '_' || *s == ' ') {
					unit->Seen.ByPlayer &= ~(1 << i);
				} else {
					unit->Seen.ByPlayer |= (1 << i);
				}
			}
		} else if (!strcmp(value, "seen-destroyed")) {
			s = LuaToString(l, j + 1);
			unit->Seen.Destroyed = 0;
			for (i = 0; i < PlayerMax && *s; ++i, ++s) {
				if (*s == '-' || *s == '_' || *s == ' ') {
					unit->Seen.Destroyed &= ~(1 << i);
				} else {
					unit->Seen.Destroyed |= (1 << i);
				}
			}
		} else if (!strcmp(value, "constructed")) {
			unit->Constructed = 1;
			--j;
		} else if (!strcmp(value, "seen-constructed")) {
			unit->Seen.Constructed = 1;
			--j;
		} else if (!strcmp(value, "seen-state")) {
			unit->Seen.State = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "active")) {
			unit->Active = 1;
			--j;
		} else if (!strcmp(value, "resource-active")) {
			unit->Data.Resource.Active = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "mana")) {
			unit->Mana = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "hp")) {
			unit->HP = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "xp")) {
			unit->XP = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "kills")) {
			unit->Kills = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "ttl")) {
			// FIXME : unsigned long should be better handled
			unit->TTL = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "bloodlust")) {
			unit->Bloodlust = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "haste")) {
			unit->Haste = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "slow")) {
			unit->Slow = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "invisible")) {
			unit->Invisible = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "flame-shield")) {
			unit->FlameShield = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unholy-armor")) {
			unit->UnholyArmor = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "group-id")) {
			unit->GroupId = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "last-group")) {
			unit->LastGroup = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "value")) {
			unit->Value = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "current-resource")) {
			lua_pushvalue(l, j + 1);
			unit->CurrentResource = CclGetResourceByName(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "sub-action")) {
			unit->SubAction = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "wait")) {
			unit->Wait = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "state")) {
			unit->State = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "reset")) {
			unit->Reset = 1;
			--j;
		} else if (!strcmp(value, "blink")) {
			unit->Blink = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "moving")) {
			unit->Moving = 1;
			--j;
		} else if (!strcmp(value, "boarded")) {
			unit->Boarded = 1;
			--j;
		} else if (!strcmp(value, "rs")) {
			unit->Rs = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "units-boarded-count")) {
			unit->BoardCount = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "units-contained")) {
			int subargs;
			int k;

			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				slot = strtol(value + 1, NULL, 16);
				AddUnitInContainer(UnitSlots[slot], unit);
				Assert(UnitSlots[slot]);
				//++UnitSlots[slot]->Refs;
			}
		} else if (!strcmp(value, "order-count")) {
			unit->OrderCount = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "order-flush")) {
			unit->OrderFlush = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "orders")) {
			int hp;

			lua_pushvalue(l, j + 1);
			CclParseOrders(l, unit);
			lua_pop(l, 1);
			// now we know unit's action so we can assign it to a player
			hp = unit->HP;
			AssignUnitToPlayer (unit, player);
			unit->HP = hp;
			if (unit->Orders[0].Action == UnitActionBuilded) {
				DebugPrint("HACK: the building is not ready yet\n");
				// HACK: the building is not ready yet
				unit->Player->UnitTypesCount[type->Slot]--;
			}
		} else if (!strcmp(value, "saved-order")) {
			lua_pushvalue(l, j + 1);
			CclParseOrder(l, &unit->SavedOrder);
			lua_pop(l, 1);
		} else if (!strcmp(value, "new-order")) {
			lua_pushvalue(l, j + 1);
			CclParseOrder(l, &unit->NewOrder);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-builded")) {
			lua_pushvalue(l, j + 1);
			CclParseBuilded(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-res-worker")) {
			lua_pushvalue(l, j + 1);
			CclParseResWorker(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-research")) {
			lua_pushvalue(l, j + 1);
			CclParseResearch(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-upgrade-to")) {
			lua_pushvalue(l, j + 1);
			CclParseUpgradeTo(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-train")) {
			lua_pushvalue(l, j + 1);
			CclParseTrain(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "data-move")) {
			lua_pushvalue(l, j + 1);
			CclParseMove(l, unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "goal")) {
			unit->Goal = UnitSlots[(int)LuaToNumber(l, j + 1)];
		} else if (!strcmp(value, "auto-cast")) {
			s = LuaToString(l, j + 1);
			Assert(SpellTypeByIdent(s));
			if (!unit->AutoCastSpell) {
				unit->AutoCastSpell = malloc(SpellTypeCount);
				memset(unit->AutoCastSpell, 0, SpellTypeCount);
			}
			unit->AutoCastSpell[SpellTypeByIdent(s)->Slot] = 1;
		} else {
		   LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	if (!unit->Player) {
		AssignUnitToPlayer (unit, player);
		UpdateForNewUnit(unit, 0);
		unit->HP = unit->Type->_HitPoints;
	}

	//  Revealers are units that can see while removed
	if (unit->Removed && unit->Type->Revealer) {
		MapMarkUnitSight(unit);
	}

	// Place on map
	if (!unit->Removed && !unit->Destroyed && !unit->Type->Vanishes) {
		unit->Removed = 1;
		PlaceUnit(unit, unit->X, unit->Y);
	}

	if (unit->UnitSlot && (unit->Destroyed || unit->Orders->Action == UnitActionDie)) {
		UnitCacheRemove(unit);
		UnitCacheInsert(unit);
		UnitCountSeen(unit);
	}

	// Units Dieing may have sight
	if (unit->Removed && unit->Orders[0].Action == UnitActionDie) {
		MapMarkUnitSight(unit);
	}

/* if (unit->Moving) {
		NewResetPath(unit);
	}*/
	// Fix Colors for rescued units.
	if (unit->RescuedFrom) {
		unit->Colors = &unit->RescuedFrom->UnitColors;
	}

	return 0;
}

/**
**  Make a unit.
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclMakeUnit(lua_State* l)
{
	UnitType* unittype;
	Unit* unit;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unittype = CclGetUnitType(l);
	lua_pop(l, 1);
	unit = MakeUnit(unittype, &Players[(int)LuaToNumber(l, 2)]);

	lua_pushnumber(l, unit->Slot);
	return 1;
}

/**
**  Place a unit on map.
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made placed.
*/
static int CclPlaceUnit(lua_State* l)
{
	Unit* unit;

	if (lua_gettop(l) != 3) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unit = CclGetUnit(l);
	lua_pop(l, 1);
	PlaceUnit(unit, LuaToNumber(l, 2), LuaToNumber(l, 3));
	lua_pushvalue(l, 1);
	return 1;
}

/**
**  Create a unit and place it on the map
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclCreateUnit(lua_State* l)
{
	UnitType* unittype;
	Unit* unit;
	int heading;
	int playerno;
	int mask;
	int ix;
	int iy;

	if (lua_gettop(l) != 3) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unittype = CclGetUnitType(l);
	lua_pop(l, 1);
	if (!lua_istable(l, 3) || luaL_getn(l, 3) != 2) {
		LuaError(l, "incorrect argument");
	}
	lua_rawgeti(l, 3, 1);
	ix = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 3, 2);
	iy = LuaToNumber(l, -1);
	lua_pop(l, 1);

	heading = SyncRand() % 256;
	lua_pushvalue(l, 2);
	playerno = TriggerGetPlayer(l);
	lua_pop(l, 1);
	if (playerno == -1) {
		printf("CreateUnit: You cannot use 'any in create-unit, specify a player\n");
		LuaError(l, "bad player");
		return 0;
	}
	unit = MakeUnit(unittype, &Players[playerno]);
	mask = UnitMovementMask(unit);
	if (CheckedCanMoveToMask(ix, iy, mask)) {
		unit->Wait = 1;
		PlaceUnit(unit, ix, iy);
	} else {
		unit->X = ix;
		unit->Y = iy;
		DropOutOnSide(unit, heading, unittype->TileWidth, unittype->TileHeight);
	}

	lua_pushnumber(l, unit->Slot);
	return 1;
}

/**
**  Order a unit
**
**  @param l  Lua state.
**
**  OrderUnit(player, unit-type, sloc, dloc, order)
*/
static int CclOrderUnit(lua_State* l)
{
	int plynr;
	int x1;
	int y1;
	int x2;
	int y2;
	int dx1;
	int dy1;
	int dx2;
	int dy2;
	const UnitType* unittype;
	Unit* table[UnitMax];
	Unit* unit;
	int an;
	int j;
	const char* order;

	if (lua_gettop(l) != 5) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 2);
	unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	if (!lua_istable(l, 3)) {
		LuaError(l, "incorrect argument");
	}
	lua_rawgeti(l, 3, 1);
	x1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 3, 2);
	y1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	if (luaL_getn(l, 3) == 4) {
		lua_rawgeti(l, 3, 3);
		x2 = LuaToNumber(l, -1);
		lua_pop(l, 1);
		lua_rawgeti(l, 3, 4);
		y2 = LuaToNumber(l, -1);
		lua_pop(l, 1);
	} else {
		x2 = x1;
		y2 = y1;
	}
	if (!lua_istable(l, 4)) {
		LuaError(l, "incorrect argument");
	}
	lua_rawgeti(l, 4, 1);
	dx1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 4, 2);
	dy1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	if (luaL_getn(l, 4) == 4) {
		lua_rawgeti(l, 4, 3);
		dx2 = LuaToNumber(l, -1);
		lua_pop(l, 1);
		lua_rawgeti(l, 4, 4);
		dy2 = LuaToNumber(l, -1);
		lua_pop(l, 1);
	} else {
		dx2 = dx1;
		dy2 = dy1;
	}
	order = LuaToString(l, 5);

	an = UnitCacheSelect(x1, y1, x2 + 1, y2 + 1, table);
	for (j = 0; j < an; ++j) {
		unit = table[j];
		if (unittype == ANY_UNIT ||
				(unittype == ALL_FOODUNITS && !unit->Type->Building) ||
				(unittype == ALL_BUILDINGS && unit->Type->Building) ||
				unittype == unit->Type) {
			if (plynr == -1 || plynr == unit->Player->Player) {
				if (!strcmp(order,"move")) {
					CommandMove(unit, (dx1 + dx2) / 2, (dy1 + dy2) / 2, 1);
				} else if (!strcmp(order, "attack")) {
					Unit* attack;

					attack=TargetOnMap(unit, dx1, dy1, dx2, dy2);
					CommandAttack(unit, (dx1 + dx2) / 2, (dy1 + dy2) / 2, attack, 1);
				} else if (!strcmp(order, "patrol")) {
					CommandPatrolUnit(unit, (dx1 + dx2) / 2, (dy1 + dy2) / 2, 1);
				} else {
					LuaError(l, "Unsupported order: %s" _C_ order);
				}
			}
		}
	}

	return 0;
}

/**
**  Kill a unit
**
**  @param l  Lua state.
**
**  @return   Returns true if a unit was killed.
*/
static int CclKillUnit(lua_State* l)
{
	int j;
	int plynr;
	const UnitType* unittype;
	Unit* unit;
	Unit** table;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	plynr = TriggerGetPlayer(l);
	if (plynr == -1) {
		table = Units;
		j = NumUnits - 1;
	} else {
		table = Players[plynr].Units;
		j = Players[plynr].TotalNumUnits - 1;
	}

	for (; j >= 0; --j) {
		unit = table[j];
		if (unittype == ANY_UNIT ||
				(unittype == ALL_FOODUNITS && !unit->Type->Building) ||
				(unittype == ALL_BUILDINGS && unit->Type->Building) ||
				unittype == unit->Type) {
			LetUnitDie(unit);
			lua_pushboolean(l, 1);
			return 1;
		}
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Kill a unit at a location
**
**  @param l  Lua state.
**
**  @return   Returns the number of units killed.
*/
static int CclKillUnitAt(lua_State* l)
{
	int plynr;
	int q;
	int x1;
	int y1;
	int x2;
	int y2;
	const UnitType* unittype;
	Unit* table[UnitMax];
	Unit* unit;
	int an;
	int j;
	int s;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 2);
	plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	q = LuaToNumber(l, 3);
	lua_pushvalue(l, 1);
	unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	if (!lua_istable(l, 4)) {
		LuaError(l, "incorrect argument");
	}
	lua_rawgeti(l, 4, 1);
	x1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 4, 2);
	y1 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 4, 3);
	x2 = LuaToNumber(l, -1);
	lua_pop(l, 1);
	lua_rawgeti(l, 4, 4);
	y2 = LuaToNumber(l, -1);
	lua_pop(l, 1);

	an = UnitCacheSelect(x1, y1, x2 + 1, y2 + 1, table);
	for (j = s = 0; j < an && s < q; ++j) {
		unit = table[j];
		if (unittype == ANY_UNIT ||
				(unittype == ALL_FOODUNITS && !unit->Type->Building) ||
				(unittype == ALL_BUILDINGS && unit->Type->Building) ||
				unittype==unit->Type) {
			if (plynr == -1 || plynr == unit->Player->Player) {
				LetUnitDie(unit);
				++s;
			}
		}
	}

	lua_pushnumber(l, s);
	return 1;
}

/**
**  Get a player's units
**
**  @param l  Lua state.
**
**  @return   Array of units.
*/
static int CclGetUnits(lua_State* l)
{
	int plynr;
	int i;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}

	plynr = TriggerGetPlayer(l);

	lua_newtable(l);
	if (plynr == -1) {
		for (i = 0; i < NumUnits; ++i) {
			lua_pushnumber(l, Units[i]->Slot);
			lua_rawseti(l, -2, i + 1);
		}
	} else {
		for (i = 0; i < Players[plynr].TotalNumUnits; ++i) {
			lua_pushnumber(l, Players[plynr].Units[i]->Slot);
			lua_rawseti(l, -2, i + 1);
		}
	}
	return 1;
}

/**
**  Get the mana of the unit structure.
**
**  @param l  Lua state.
**
**  @return   The mana of the unit.
*/
static int CclGetUnitMana(lua_State* l)
{
	const Unit* unit;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}

	unit = CclGetUnit(l);
	lua_pushnumber(l, unit->Mana);
	return 1;
}

/**
**  Set the mana of the unit structure.
**
**  @param l  Lua state.
**
**  @return The new mana of the unit.
*/
static int CclSetUnitMana(lua_State* l)
{
	Unit* unit;
	int mana;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unit = CclGetUnit(l);
	lua_pop(l, 1);
	mana = LuaToNumber(l, 2);
	if (unit->Type->CanCastSpell && unit->Type->_MaxMana) {
		if (mana > unit->Type->_MaxMana) {
			unit->Mana = unit->Type->_MaxMana;
		} else {
			unit->Mana = mana;
		}
	}

	lua_pushnumber(l, mana);
	return 1;
}

/**
**  Get the unholy-armor of the unit structure.
**
**  @param l  Lua state.
**
**  @return   The unholy-armor of the unit.
*/
static int CclGetUnitUnholyArmor(lua_State* l)
{
	const Unit* unit;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}

	unit = CclGetUnit(l);
	lua_pushnumber(l, unit->UnholyArmor);
	return 1;
}

/**
**  Set the unholy-armor of the unit structure.
**
**  @param l  Lua state.
**
**  @return   The value of the unit.
*/
static int CclSetUnitUnholyArmor(lua_State* l)
{
	Unit* unit;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}

	lua_pushvalue(l, 1);
	unit = CclGetUnit(l);
	lua_pop(l, 1);
	unit->UnholyArmor = LuaToNumber(l, 2);

	lua_pushvalue(l, 2);
	return 1;
}

/**
**  FIXME: docu
**
**  @param l  Lua state.
*/
static int CclSlotUsage(lua_State* l)
{
#define SLOT_LEN MAX_UNIT_SLOTS / 8 + 1
	unsigned char SlotUsage[SLOT_LEN];
	int i;
	int prev;
	const char* value;
	int args;
	int j;

	memset(SlotUsage, 0, SLOT_LEN);
	prev = -1;
	args = lua_gettop(l);
	for (j = 0; j < args; ++j) {
		if (lua_type(l, j + 1) == LUA_TSTRING &&
				!strcmp((value = LuaToString(l, j + 1)), "-")) {
			int range_end;

			++j;
			range_end = LuaToNumber(l, j + 1);
			for (i = prev; i <= range_end; ++i) {
				SlotUsage[i / 8] |= 1 << (i % 8);
			}
			prev = -1;
		} else {
			if (prev >= 0) {
				SlotUsage[prev / 8] |= 1 << (prev % 8);
			}
			prev = LuaToNumber(l, j + 1);
		}
	}
	if (prev >= 0) {
		SlotUsage[prev / 8] |= 1 << (prev % 8);
	}

	/* now walk through the bitfield and create the needed unit slots */
	for (i = 0; i < SLOT_LEN * 8; ++i) {
		if (SlotUsage[i / 8] & (1 << i % 8)) {
			Unit* new_unit = (Unit*)calloc(1, sizeof(Unit));
			UnitSlotFree = (void*)UnitSlots[i];
			UnitSlots[i] = new_unit;
			new_unit->Slot = i;
		}
	}
	return 0;
#undef SLOT_LEN
}

/**
** Load the unit allocator state.
** We need to do this in order to make sure that the game allocates units
**  in the exact same way.
**
**  @param l  Lua state.
*/
static int CclUnitAllocQueue(lua_State* l)
{
	int i;
	int args;
	const char* key;
	Unit* unit;

	ReleasedHead = ReleasedTail = 0;
	args = lua_gettop(l);
	for (i = 1; i <= args; ++i) {
		unit = malloc(sizeof(Unit));

		lua_pushnil(l);
		while (lua_next(l, i)) {
			key = LuaToString(l, -2);
			if (!strcmp(key, "Slot")) {
				unit->Slot = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(key, "FreeCycle")) {
				unit->Refs = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else {
				LuaError(l, "Wrong key %s" _C_ key);
				return 0;
			}
		}

		unit->Next = 0;
		if (ReleasedHead) {
			ReleasedTail->Next = unit;
			ReleasedTail = unit;
		} else {
			ReleasedHead = ReleasedTail = unit;
		}
	}
	return 0;
}

/**
**  Register CCL features for unit.
*/
void UnitCclRegister(void)
{
	lua_register(Lua, "SetXPDamage", CclSetXpDamage);
	lua_register(Lua, "SetTrainingQueue", CclSetTrainingQueue);
	lua_register(Lua, "SetBuildingCapture", CclSetBuildingCapture);
	lua_register(Lua, "SetRevealAttacker", CclSetRevealAttacker);

	lua_register(Lua, "Unit", CclUnit);

	lua_register(Lua, "MakeUnit", CclMakeUnit);
	lua_register(Lua, "PlaceUnit", CclPlaceUnit);
	lua_register(Lua, "CreateUnit", CclCreateUnit);
	lua_register(Lua, "OrderUnit", CclOrderUnit);
	lua_register(Lua, "KillUnit", CclKillUnit);
	lua_register(Lua, "KillUnitAt", CclKillUnitAt);

	lua_register(Lua, "GetUnits", CclGetUnits);

	// unit member access functions
	lua_register(Lua, "GetUnitMana", CclGetUnitMana);
	lua_register(Lua, "SetUnitMana", CclSetUnitMana);
	lua_register(Lua, "GetUnitUnholyArmor", CclGetUnitUnholyArmor);
	lua_register(Lua, "SetUnitUnholyArmor", CclSetUnitUnholyArmor);

	lua_register(Lua, "SlotUsage", CclSlotUsage);
	lua_register(Lua, "UnitAllocQueue", CclUnitAllocQueue);
}

//@}
