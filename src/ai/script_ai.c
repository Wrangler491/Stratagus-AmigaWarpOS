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
/**@name script_ai.c - The AI ccl functions. */
//
//      (c) Copyright 2000-2004 by Lutz Sammer, Ludovic Pollet,
//                                 and Jimmy Salmon
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
//      $Id: script_ai.c,v 1.112 2004/06/24 15:07:52 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "unittype.h"
#include "script.h"
#include "ai.h"
#include "pathfinder.h"
#include "ai_local.h"


/**
**  Setup AI helper table.
**
**  Expand the table if needed.
**
**  @param count  Pointer to number of elements in table.
**  @param table  Pointer to table with elements.
**  @param n      Index to insert new into table
*/
static void AiHelperSetupTable(int* count, AiUnitTypeTable*** table, int n)
{
	int i;

	++n;
	if (n > (i = *count)) {
		if (*table) {
			*table = realloc(*table, n * sizeof(AiUnitTypeTable*));
			memset((*table) + i, 0, (n - i) * sizeof(AiUnitTypeTable*));
		} else {
			*table = malloc(n * sizeof(AiUnitTypeTable*));
			memset(*table, 0, n * sizeof(AiUnitTypeTable*));
		}
		*count = n;
	}
}

/**
**  Insert new unit-type element.
**
**  @param tablep  Pointer to table with elements.
**  @param base    Base type to insert into table.
*/
static void AiHelperInsert(AiUnitTypeTable** tablep, UnitType* base)
{
	int i;
	int n;
	AiUnitTypeTable* table;

	//
	// New unit-type
	//
	if (!(table = *tablep)) {
		table = *tablep = malloc(sizeof(AiUnitTypeTable));
		table->Count = 1;
		table->Table[0] = base;
		return;
	}
	//
	// Look if already known.
	//
	n = table->Count;
	for (i = 0; i < n; ++i) {
		if (table->Table[i] == base) {
			return;
		}
	}

	//
	// Append new base unit-type to units.
	//
	table = *tablep = realloc(table, sizeof(AiUnitTypeTable) + sizeof(UnitType*) * n);
	table->Count = n + 1;
	table->Table[n] = base;
}

#if 0
/**
**  Print AI helper table.
*/
static void PrintAiHelperTable(void)
{
}
#endif

/**
**  Define helper for AI.
**
**  @param l  Lua state.
**
**  @todo  FIXME: the first unit could be a list see ../doc/ccl/ai.html
*/
static int CclDefineAiHelper(lua_State* l)
{
	const char* value;
	int what;
	UnitType* base;
	UnitType* type;
	Upgrade* upgrade;
	int cost;
	int args;
	int j;
	int subargs;
	int k;

#ifdef DEBUG
	type = NULL;
	upgrade = NULL;
	cost = 0;
#endif
	args = lua_gettop(l);
	for (j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		subargs = luaL_getn(l, j + 1);
		k = 0;
		lua_rawgeti(l, j + 1, k + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++k;

		//
		// Type build,train,research/upgrade.
		//
		if (!strcmp(value, "build")) {
			what = 0;
		} else if (!strcmp(value, "train")) {
			what = 1;
		} else if (!strcmp(value, "upgrade")) {
			what = 2;
		} else if (!strcmp(value, "research")) {
			what = 3;
		} else if (!strcmp(value, "unit-limit")) {
			what = 4;
		} else if (!strcmp(value, "unit-equiv")) {
			what = 5;
		} else if (!strcmp(value, "repair")) {
			what = 6;
		} else {
			LuaError(l, "unknown tag: %s" _C_ value);
			what = -1;
		}

		//
		// Get the base unit type, which could handle the action.
		//

		// FIXME: support value as list!
		lua_rawgeti(l, j + 1, k + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++k;
		base = UnitTypeByIdent(value);
		if (!base) {
			LuaError(l, "unknown unittype: %s" _C_ value);
		}

		//
		// Get the unit types, which could be produced
		//
		for (; k < subargs; ++k) {
			lua_rawgeti(l, j + 1, k + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			if (what == 3) {
				upgrade = UpgradeByIdent(value);
				if (!upgrade) {
					LuaError(l, "unknown upgrade: %s" _C_ value);
				}
			} else if (what == 4) {
				if (!strcmp("food", value)) {
					cost = 0;
				} else {
					LuaError(l, "unknown limit: %s" _C_ value);
				}
			} else {
				type = UnitTypeByIdent(value);
				if (!type) {
					LuaError(l, "unknown unittype: %s" _C_ value);
				}
			}

			switch (what) {
				case 0: // build
					AiHelperSetupTable(&AiHelpers.BuildCount, &AiHelpers.Build,
						type->Slot);
					AiHelperInsert(AiHelpers.Build + type->Slot, base);
					break;
				case 1: // train
					AiHelperSetupTable(&AiHelpers.TrainCount, &AiHelpers.Train,
						type->Slot);
					AiHelperInsert(AiHelpers.Train + type->Slot, base);
					break;
				case 2: // upgrade
					AiHelperSetupTable(&AiHelpers.UpgradeCount, &AiHelpers.Upgrade,
						type->Slot);
					AiHelperInsert(AiHelpers.Upgrade + type->Slot, base);
					break;
				case 3: // research
					AiHelperSetupTable(&AiHelpers.ResearchCount, &AiHelpers.Research,
						upgrade - Upgrades);
					AiHelperInsert(AiHelpers.Research + (upgrade - Upgrades), base);
					break;
				case 4: // unit-limit
					AiHelperSetupTable(&AiHelpers.UnitLimitCount, &AiHelpers.UnitLimit,
						cost);
					AiHelperInsert(AiHelpers.UnitLimit + cost, base);
					break;
				case 5: // equivalence
					AiHelperSetupTable(&AiHelpers.EquivCount, &AiHelpers.Equiv,
						base->Slot);
					AiHelperInsert(AiHelpers.Equiv + base->Slot, type);

					AiNewUnitTypeEquiv(base, type);
					break;
				case 6: // repair
					AiHelperSetupTable(&AiHelpers.RepairCount, &AiHelpers.Repair,
						type->Slot);
					AiHelperInsert(AiHelpers.Repair + type->Slot, base);
					break;
			}
		}
	}

	return 0;
}

/**
**  Define an AI engine.
**
**  @param l  Lua state.
**
**  @return   FIXME: docu
*/
static int CclDefineAi(lua_State* l)
{
	const char* value;
	AiType* aitype;
#ifdef DEBUG
	const AiType* ait;
#endif

	if (lua_gettop(l) != 4 || !lua_isfunction(l, 4)) {
		LuaError(l, "incorrect argument");
	}

	aitype = malloc(sizeof(AiType));
	aitype->Next = AiTypes;
	AiTypes = aitype;

	//
	// AI Name
	//
	aitype->Name = strdup(LuaToString(l, 1));

#ifdef DEBUG
	for (ait = AiTypes->Next; ait; ait = ait->Next) {
		if (!strcmp(aitype->Name, ait->Name)) {
			DebugPrint("Warning two or more AI's with the same name '%s'\n" _C_ ait->Name);
		}
	}
#endif

	//
	// AI Race
	//
	value = LuaToString(l, 2);
	if (*value != '*') {
		aitype->Race = strdup(value);
	} else {
		aitype->Race = NULL;
	}

	//
	// AI Class
	//
	aitype->Class = strdup(LuaToString(l, 3));

	//
	// AI Script
	//
	lua_pushstring(l, "_ai_scripts_");
	lua_gettable(l, LUA_GLOBALSINDEX);
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_pushstring(l, "_ai_scripts_");
		lua_newtable(l);
		lua_settable(l, LUA_GLOBALSINDEX);
		lua_pushstring(l, "_ai_scripts_");
		lua_gettable(l, LUA_GLOBALSINDEX);
	}
	aitype->Script = malloc(strlen(aitype->Name) +
		(aitype->Race ? strlen(aitype->Race) : 0) +
		strlen(aitype->Class) + 1);
	sprintf(aitype->Script, "%s%s%s", aitype->Name,
		(aitype->Race ? aitype->Race : ""), aitype->Class);
	lua_pushstring(l, aitype->Script);
	lua_pushvalue(l, 4);
	lua_rawset(l, 5);
	lua_pop(l, 1);

// Get name of function
	lua_pushstring(l, "debug");
	lua_gettable(l, LUA_GLOBALSINDEX);
	Assert(!lua_isnil(l, -1));
	lua_pushstring(l, "getinfo");
	lua_gettable(l, -2);
	Assert(lua_isfunction(l, -1));
	lua_pushvalue(l, 4);
	lua_call(l, 1, 1);
	lua_pushstring(l, "name");
	lua_gettable(l, -2);
	aitype->FunctionName = strdup(lua_tostring(l, -1));
	lua_pop(l, 2); // FIXME : check if this value is correct.
	// We can have opcode of this function with string.dump(function)
	// Problems are for sub functions...
	return 0;
}

/*----------------------------------------------------------------------------
--  AI script functions
----------------------------------------------------------------------------*/

/**
**  Append unit-type to request table.
**
**  @param type   Unit-type to be appended.
**  @param count  How many unit-types to build.
*/
static void InsertUnitTypeRequests(UnitType* type, int count)
{
	int n;

	if (AiPlayer->UnitTypeRequests) {
		n = AiPlayer->UnitTypeRequestsCount;
		AiPlayer->UnitTypeRequests = realloc(AiPlayer->UnitTypeRequests,
			(n + 1) * sizeof(*AiPlayer->UnitTypeRequests));
	} else {
		AiPlayer->UnitTypeRequests = malloc(sizeof(*AiPlayer->UnitTypeRequests));
		n = 0;
	}
	AiPlayer->UnitTypeRequests[n].Table[0] = type;
	AiPlayer->UnitTypeRequests[n].Count = count;
	AiPlayer->UnitTypeRequestsCount = n + 1;
}

/**
**  Find unit-type in request table.
**
**  @param type  Unit-type to be found.
*/
static AiUnitTypeTable* FindInUnitTypeRequests(const UnitType* type)
{
	int i;
	int n;

	n = AiPlayer->UnitTypeRequestsCount;
	for (i = 0; i < n; ++i) {
		if (AiPlayer->UnitTypeRequests[i].Table[0] == type) {
			return &AiPlayer->UnitTypeRequests[i];
		}
	}
	return NULL;
}

/**
**  Find unit-type in upgrade-to table.
**
**  @param type  Unit-type to be found.
*/
static int FindInUpgradeToRequests(const UnitType* type)
{
	int i;
	int n;

	n = AiPlayer->UpgradeToRequestsCount;
	for (i = 0; i < n; ++i) {
		if (AiPlayer->UpgradeToRequests[i] == type) {
			return 1;
		}
	}
	return 0;
}

/**
**  Append unit-type to request table.
**
**  @param type  Unit-type to be appended.
*/
static void InsertUpgradeToRequests(UnitType* type)
{
	int n;

	if (AiPlayer->UpgradeToRequests) {
		n = AiPlayer->UpgradeToRequestsCount;
		AiPlayer->UpgradeToRequests = realloc(AiPlayer->UpgradeToRequests,
			(n + 1) * sizeof(*AiPlayer->UpgradeToRequests));
	} else {
		AiPlayer->UpgradeToRequests = malloc(sizeof(*AiPlayer->UpgradeToRequests));
		n = 0;
	}
	AiPlayer->UpgradeToRequests[n] = type;
	AiPlayer->UpgradeToRequestsCount = n + 1;
}

/**
**  Append unit-type to request table.
**
**  @param upgrade  Upgrade to be appended.
*/
static void InsertResearchRequests(Upgrade* upgrade)
{
	int n;

	if (AiPlayer->ResearchRequests) {
		n = AiPlayer->ResearchRequestsCount;
		AiPlayer->ResearchRequests = realloc(AiPlayer->ResearchRequests,
			(n + 1) * sizeof(*AiPlayer->ResearchRequests));
	} else {
		AiPlayer->ResearchRequests = malloc(sizeof(*AiPlayer->ResearchRequests));
		n = 0;
	}
	AiPlayer->ResearchRequests[n] = upgrade;
	AiPlayer->ResearchRequestsCount = n + 1;
}

//----------------------------------------------------------------------------

/**
**  Get the race of the current AI player.
**
**  @param l  Lua state.
*/
static int CclAiGetRace(lua_State* l)
{
	if (lua_gettop(l) != 0) {
		LuaError(l, "incorrect argument");
	}
	lua_pushstring(l, AiPlayer->Player->RaceName);
	return 1;
}

/**
**  Get the number of cycles to sleep.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiGetSleepCycles(lua_State* l)
{
	if (lua_gettop(l) != 0) {
		LuaError(l, "incorrect argument");
	}
	lua_pushnumber(l, AiSleepCycles);
	return 1;
}

//----------------------------------------------------------------------------

/**
**  Set debugging flag of AI script
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiDebug(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	if (!LuaToBoolean(l, 1)) {
		AiPlayer->ScriptDebug = 0;
	} else {
		AiPlayer->ScriptDebug = 1;
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Activate AI debugging for the given player(s)
**  Player can be a number for a specific player
**    "self" for current human player (ai me)
**    "none" to disable
**
**  @param l  Lua State
**
**  @return   Number of return values
*/
static int CclAiDebugPlayer(lua_State* l)
{
	const char* item;
	int playerid;
	int args;
	int j;

	args = lua_gettop(l);
	for (j = 0; j < args; ++j) {
		if (lua_isstring(l, j + 1)) {
			item = LuaToString(l, j + 1);
		} else {
			item = NULL;
		}

		if (item && !strcmp(item, "none")) {
			for (playerid = 0; playerid < NumPlayers; ++playerid) {
				if (!Players[playerid].AiEnabled || !Players[playerid].Ai) {
					continue;
				}
				((PlayerAi*)Players[playerid].Ai)->ScriptDebug = 0;
			}
		} else {
			if (item && !strcmp(item, "self")) {
				if (!ThisPlayer) {
					continue;
				}
				playerid = ThisPlayer->Player;
			} else {
				playerid = LuaToNumber(l, j + 1);
			}

			if (!Players[playerid].AiEnabled || !Players[playerid].Ai) {
				continue;
			}
			((PlayerAi*)Players[playerid].Ai)->ScriptDebug = 1;
		}
	}
	return 0;
}

/**
**  Need a unit.
**
**  @param l  Lua state.
**
**  @return   Number of return values
*/
static int CclAiNeed(lua_State* l)
{
	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	InsertUnitTypeRequests(CclGetUnitType(l), 1);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Set the number of units.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiSet(lua_State* l)
{
	AiUnitTypeTable* autt;
	UnitType* type;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	type = CclGetUnitType(l);
	lua_pop(l, 1);
	if ((autt = FindInUnitTypeRequests(type))) {
		autt->Count = LuaToNumber(l, 2);
		// FIXME: 0 should remove it.
	} else {
		InsertUnitTypeRequests(type, LuaToNumber(l, 2));
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for an unit.
**
**  @param l  Lua State.
**
**  @return   Number of return values
*/
static int CclAiWait(lua_State* l)
{
	const AiUnitTypeTable* autt;
	const UnitType* type;
	const int* unit_types_count;
	int j;
	int n;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	type = CclGetUnitType(l);
	unit_types_count = AiPlayer->Player->UnitTypesCount;
	if (!(autt = FindInUnitTypeRequests(type))) {
		//
		// Look if we have this unit-type.
		//
		if (unit_types_count[type->Slot]) {
			lua_pushboolean(l, 0);
			return 1;
		}

		//
		// Look if we have equivalent unit-types.
		//
		if (type->Slot < AiHelpers.EquivCount && AiHelpers.Equiv[type->Slot]) {
			for (j = 0; j < AiHelpers.Equiv[type->Slot]->Count; ++j) {
				if (unit_types_count[AiHelpers.Equiv[type->Slot]->Table[j]->Slot]) {
					lua_pushboolean(l, 0);
					return 1;
				}
			}
		}
		//
		// Look if we have an upgrade-to request.
		//
		if (FindInUpgradeToRequests(type)) {
			lua_pushboolean(l, 1);
			return 1;
		}
		DebugPrint("Broken? waiting on %s which wasn't requested.\n" _C_ type->Ident);
		lua_pushboolean(l, 0);
		return 1;
	}
	//
	// Add equivalent units
	//
	n = unit_types_count[type->Slot];
	if (type->Slot < AiHelpers.EquivCount && AiHelpers.Equiv[type->Slot]) {
		for (j = 0; j < AiHelpers.Equiv[type->Slot]->Count; ++j) {
			n += unit_types_count[AiHelpers.Equiv[type->Slot]->Table[j]->Slot];
		}
	}
	// units available?

	if (n >= autt->Count) {
		lua_pushboolean(l, 0);
		return 1;
	}

	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Define a force, a groups of units.
**
**  @param l  Lua state.
*/
static int CclAiForce(lua_State* l)
{
	AiUnitType** prev;
	AiUnitType* aiut;
	UnitType* type;
	int count;
	int force;
	int args;
	int j;

	if (lua_gettop(l) != 2 || !lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCES) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}

	args = luaL_getn(l, 2);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, 2, j + 1);
		type = CclGetUnitType(l);
		lua_pop(l, 1);
		++j;
		lua_rawgeti(l, 2, j + 1);
		count = LuaToNumber(l, -1);
		lua_pop(l, 1);

		if (!type) { // bulletproof
			continue;
		}

		// Use the equivalent unittype.
		type = UnitTypes[UnitTypeEquivs[type->Slot]];

		//
		// Look if already in force.
		//
		for (prev = &AiPlayer->Force[force].UnitTypes; (aiut = *prev);
				prev = &aiut->Next) {
			if (aiut->Type->Slot == type->Slot) { // found
				if (count) {
					aiut->Want = count;
				} else {
					*prev = aiut->Next;
					free(aiut);
				}
				break;
			}
		}

		//
		// New type append it.
		//
		if (!aiut) {
			*prev = aiut = malloc(sizeof(*aiut));
			aiut->Next = NULL;
			aiut->Want = count;
			aiut->Type = type;
		}
	}

	AiAssignFreeUnitsToForce();

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Define the role of a force.
**
**  @param l  Lua state.
*/
static int CclAiForceRole(lua_State* l)
{
	int force;
	const char* flag;

	if (lua_gettop(l) != 2) {
		LuaError(l, "incorrect argument");
	}
	force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCES) {
		LuaError(l, "Force %i out of range" _C_ force);
	}
	flag = LuaToString(l, 2);
	if (!strcmp(flag, "attack")) {
		AiPlayer->Force[force].Role = AiForceRoleAttack;
	} else if (!strcmp(flag, "defend")) {
		AiPlayer->Force[force].Role = AiForceRoleDefend;
	} else {
		LuaError(l, "Unknown force role '%s'" _C_ flag);
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Check if a force ready.
**
**  @param l  Lua state.
*/
static int CclAiCheckForce(lua_State* l)
{
	int force;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCES) {
		lua_pushfstring(l, "Force out of range: %d", force);
	}
	if (AiPlayer->Force[force].Completed) {
		lua_pushboolean(l, 1);
		return 1;
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a force ready.
**
**  @param l  Lua state.
*/
static int CclAiWaitForce(lua_State* l)
{
	int force;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCES) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	if (AiPlayer->Force[force].Completed) {
		lua_pushboolean(l, 0);
		return 1;
	}

#if 0
	// Debuging
	AiCleanForces();
	Assert(!AiPlayer->Force[force].Completed);
#endif

	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Attack with force.
**
**  @param l  Lua state.
*/
static int CclAiAttackWithForce(lua_State* l)
{
	int force;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCES) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}

	AiAttackWithForce(force);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Sleep n cycles.
**
**  @param l  Lua state.
*/
static int CclAiSleep(lua_State* l)
{
	int i;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}

	if (AiPlayer->SleepCycles) {
		if (AiPlayer->SleepCycles < GameCycle) {
			AiPlayer->SleepCycles = 0;
			lua_pushboolean(l, 0);
			return 1;
		}
	} else {
		i = LuaToNumber(l, 1);
		AiPlayer->SleepCycles = GameCycle + i;
	}

	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Research an upgrade.
**
**  @param l  Lua state.
*/
static int CclAiResearch(lua_State* l)
{
	const char* str;
	Upgrade* upgrade;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	if ((str = LuaToString(l, 1))) {
		upgrade = UpgradeByIdent(str);
	} else {
		LuaError(l, "Upgrade needed");
		upgrade = 0;
	}

	InsertResearchRequests(upgrade);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Upgrade an unit to an new unit-type.
**
**  @param l  Lua state.
*/
static int CclAiUpgradeTo(lua_State* l)
{
	UnitType* type;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	type = CclGetUnitType(l);
	InsertUpgradeToRequests(type);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Return the player of the running AI.
**
**  @param l  Lua state.
**
**  @return  Player number of the AI.
*/
static int CclAiPlayer(lua_State* l)
{
	if (lua_gettop(l) != 0) {
		LuaError(l, "incorrect argument");
	}
	lua_pushnumber(l, AiPlayer->Player->Player);
	return 1;
}

/**
**  Set AI player resource reserve.
**
**  @param l  Lua state.
**
**  @return     Old resource vector
*/
static int CclAiSetReserve(lua_State* l)
{
	int i;

	if (lua_gettop(l) != 1 || !lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	lua_newtable(l);
	for (i = 0; i < MaxCosts; ++i) {
		lua_pushnumber(l, AiPlayer->Reserve[i]);
		lua_rawseti(l, -2, i + 1);
	}
	for (i = 0; i < MaxCosts; ++i) {
		lua_rawgeti(l, 1, i + 1);
		AiPlayer->Reserve[i] = LuaToNumber(l, -1);
		lua_pop(l, 1);
	}
	return 1;
}

/**
**  Set AI player resource collect percent.
**
**  @param l  Lua state.
**
**  @return     Old resource vector
*/
static int CclAiSetCollect(lua_State* l)
{
	int i;

	if (lua_gettop(l) != 1 || !lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	// FIXME: use key/value pairs
	lua_newtable(l);
	for (i = 0; i < MaxCosts; ++i) {
		lua_pushnumber(l, AiPlayer->Collect[i]);
		lua_rawseti(l, -2, i + 1);
	}
	for (i = 0; i < MaxCosts; ++i) {
		lua_rawgeti(l, 1, i + 1);
		AiPlayer->Collect[i] = LuaToNumber(l, -1);
		lua_pop(l, 1);
	}
	return 1;
}

/**
**  Dump some AI debug informations.
**
**  @param l  Lua state.
*/
static int CclAiDump(lua_State* l)
{
	int i;
	int n;
	const AiUnitType* aut;
	const AiBuildQueue* queue;

	if (lua_gettop(l) != 0) {
		LuaError(l, "incorrect argument");
	}
	//
	// Script
	//
	printf("------\n");
	for (i = 0; i < MaxCosts; ++i) {
		printf("%s(%4d) ", DefaultResourceNames[i], AiPlayer->Player->Resources[i]);
	}
	printf("\n");
	printf("%d:", AiPlayer->Player->Player);
#if 0
	gh_display(gh_car(AiPlayer->Script));
#endif
	//
	// Requests
	//
	n = AiPlayer->UnitTypeRequestsCount;
	printf("UnitTypeRequests(%d):\n", n);
	for (i = 0; i < n; ++i) {
		printf("%s ", AiPlayer->UnitTypeRequests[i].Table[0]->Ident);
	}
	printf("\n");
	n = AiPlayer->UpgradeToRequestsCount;
	printf("UpgradeToRequests(%d):\n", n);
	for (i = 0; i < n; ++i) {
		printf("%s ", AiPlayer->UpgradeToRequests[i]->Ident);
	}
	printf("\n");
	n = AiPlayer->ResearchRequestsCount;
	printf("ResearchRequests(%d):\n", n);
	for (i = 0; i < n; ++i) {
		printf("%s ", AiPlayer->ResearchRequests[i]->Ident);
	}
	printf("\n");

	//
	// Building queue
	//
	printf("Building queue:\n");
	for (queue = AiPlayer->UnitTypeBuilded; queue; queue = queue->Next) {
		printf("%s(%d/%d) ", queue->Type->Ident, queue->Made, queue->Want);
	}
	printf("\n");

	//
	// PrintForce
	//
	for (i = 0; i < AI_MAX_FORCES; ++i) {
		printf("Force(%d%s%s):\n", i,
			AiPlayer->Force[i].Completed ? ",complete" : ",recruit",
			AiPlayer->Force[i].Attacking ? ",attack" : "");
		for (aut = AiPlayer->Force[i].UnitTypes; aut; aut = aut->Next) {
			printf("%s(%d) ", aut->Type->Ident, aut->Want);
		}
		printf("\n");
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Define AI mapping from original number to internal symbol
**
**  @param l  Lua state.
*/
static int CclDefineAiWcNames(lua_State* l)
{
	int i;
	int j;
	char** cp;

	if ((cp = AiTypeWcNames)) { // Free all old names
		while (*cp) {
			free(*cp++);
		}
		free(AiTypeWcNames);
	}

	//
	// Get new table.
	//
	i = lua_gettop(l);
	AiTypeWcNames = cp = malloc((i + 1) * sizeof(char*));
	if (!cp) {
		fprintf(stderr, "out of memory.\n");
		ExitFatal(-1);
	}

	for (j = 0; j < i; ++j) {
		*cp++ = strdup(LuaToString(l, j + 1));
	}
	*cp = NULL;

	return 0;
}

/**
**  Get the default resource number
**
**  @param name  Resource name.
**
**  @return   The number of the resource in DefaultResourceNames
*/
static int DefaultResourceNumber(const char* name)
{
	int i;

	for (i = 0; i < MaxCosts; ++i) {
		if (!strcmp(DefaultResourceNames[i], name)) {
			return i;
		}
	}
	// Resource not found, should never happen
	Assert(0);
	return -1;
}

/**
** Define an AI player.
**
**  @param l  Lua state.
*/
static int CclDefineAiPlayer(lua_State* l)
{
	const char* value;
	int i;
	PlayerAi* ai;
	int args;
	int j;
	int subargs;
	int k;

	args = lua_gettop(l);
	j = 0;

	i = LuaToNumber(l, j + 1);
	++j;

	Assert(i >= 0 && i <= PlayerMax);
	DebugPrint("%p %d\n" _C_ Players[i].Ai _C_ Players[i].AiEnabled );
	// FIXME: lose this:
	// Assert(!Players[i].Ai && Players[i].AiEnabled);

	ai = Players[i].Ai = calloc(1, sizeof(PlayerAi));
	ai->Player = &Players[i];

	//
	// Parse the list: (still everything could be changed!)
	//
	for (; j < args; ++j) {
		value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "ai-type")) {
			AiType* ait;

			value = LuaToString(l, j + 1);
			for (ait = AiTypes; ait; ait = ait->Next) {
				if (!strcmp(ait->Name, value)) {
					break;
				}
			}
			if (!ait) {
				lua_pushfstring(l, "ai-type not found: %s", value);
			}
			ai->AiType = ait;
			ai->Script = ait->Script;
		} else if (!strcmp(value, "script")) {
			ai->Script = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "script-debug")) {
			ai->ScriptDebug = LuaToBoolean(l, j + 1);
		} else if (!strcmp(value, "sleep-cycles")) {
			ai->SleepCycles = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "force")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			k = 0;
			lua_rawgeti(l, j + 1, k + 1);
			i = LuaToNumber(l, -1);
			lua_pop(l, 1);
			++k;
			for (; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				if (!strcmp(value, "complete")) {
					ai->Force[i].Completed = 1;
					--k;
				} else if (!strcmp(value, "recruit")) {
					ai->Force[i].Completed = 0;
					--k;
				} else if (!strcmp(value, "attack")) {
					ai->Force[i].Attacking = 1;
					--k;
				} else if (!strcmp(value, "defend")) {
					ai->Force[i].Defending = 1;
					--k;
				} else if (!strcmp(value, "role")) {
					lua_rawgeti(l, j + 1, k + 1);
					value = LuaToString(l, -1);
					lua_pop(l, 1);
					if (!strcmp(value, "attack")) {
						ai->Force[i].Role = AiForceRoleAttack;
					} else if (!strcmp(value, "defend")) {
						ai->Force[i].Role = AiForceRoleDefend;
					} else {
						LuaError(l, "Unsupported force tag: %s" _C_ value);
					}
				} else if (!strcmp(value, "types")) {
					AiUnitType** queue;
					int subsubargs;
					int subk;

					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					subsubargs = luaL_getn(l, -1);
					queue = &ai->Force[i].UnitTypes;
					for (subk = 0; subk < subsubargs; ++subk) {
						int num;
						const char* ident;

						lua_rawgeti(l, -1, subk + 1);
						num = LuaToNumber(l, -1);
						lua_pop(l, 1);
						++subk;
						lua_rawgeti(l, -1, subk + 1);
						ident = LuaToString(l, -1);
						lua_pop(l, 1);
						*queue = malloc(sizeof(AiUnitType));
						(*queue)->Next = NULL;
						(*queue)->Want = num;
						(*queue)->Type = UnitTypeByIdent(ident);
						queue = &(*queue)->Next;
					}
					lua_pop(l, 1);
				} else if (!strcmp(value, "units")) {
					AiUnit** queue;
					int subsubargs;
					int subk;

					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					subsubargs = luaL_getn(l, -1);
					queue = &ai->Force[i].Units;
					for (subk = 0; subk < subsubargs; ++subk) {
						int num;
						const char* ident;

						lua_rawgeti(l, -1, subk + 1);
						num = LuaToNumber(l, -1);
						lua_pop(l, 1);
						++subk;
						lua_rawgeti(l, -1, subk + 1);
						ident = LuaToString(l, -1);
						lua_pop(l, 1);
						*queue = malloc(sizeof(AiUnit));
						(*queue)->Next = NULL;
						(*queue)->Unit = UnitSlots[num];
						queue = &(*queue)->Next;
					}
					lua_pop(l, 1);
				}
			}
		} else if (!strcmp(value, "reserve")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				const char* type;
				int num;

				lua_rawgeti(l, j + 1, k + 1);
				type = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				num = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->Reserve[DefaultResourceNumber(type)] = num;
			}
		} else if (!strcmp(value, "used")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				const char* type;
				int num;

				lua_rawgeti(l, j + 1, k + 1);
				type = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				num = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->Used[DefaultResourceNumber(type)] = num;
			}
		} else if (!strcmp(value, "needed")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				const char* type;
				int num;

				lua_rawgeti(l, j + 1, k + 1);
				type = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				num = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->Needed[DefaultResourceNumber(type)] = num;
			}
		} else if (!strcmp(value, "collect")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				const char* type;
				int num;

				lua_rawgeti(l, j + 1, k + 1);
				type = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				num = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->Collect[DefaultResourceNumber(type)] = num;
			}
		} else if (!strcmp(value, "need-mask")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				const char* type;

				lua_rawgeti(l, j + 1, k + 1);
				type = LuaToString(l, -1);
				lua_pop(l, 1);
				ai->NeededMask |= (1 << DefaultResourceNumber(type));
			}
		} else if (!strcmp(value, "need-supply")) {
			ai->NeedSupply = 1;
			--j;
		} else if (!strcmp(value, "exploration")) {
			AiExplorationRequest** queue;

			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			queue = &ai->FirstExplorationRequest;
			for (k = 0; k < subargs; ++k) {
				int x;
				int y;
				int mask;

				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1) || luaL_getn(l, -1) != 3) {
					LuaError(l, "incorrect argument");
				}
				lua_rawgeti(l, -1, 1);
				x = LuaToNumber(l, -1);
				lua_pop(l, 1);
				lua_rawgeti(l, -1, 2);
				y = LuaToNumber(l, -1);
				lua_pop(l, 1);
				lua_rawgeti(l, -1, 3);
				mask = LuaToNumber(l, -1);
				lua_pop(l, 1);
				lua_pop(l, 1);
				*queue = malloc(sizeof(AiExplorationRequest));
				(*queue)->Next = NULL;
				(*queue)->X = x;
				(*queue)->Y = y;
				(*queue)->Mask = mask;
				queue = &(*queue)->Next;
			}
		} else if (!strcmp(value, "last-exploration-cycle")) {
			ai->LastExplorationGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "transport")) {
			AiTransportRequest** queue;

			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			queue = &ai->TransportRequests;
			for (k = 0; k < subargs; ++k) {
				int unit;

				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
					LuaError(l, "incorrect argument");
				}
				lua_rawgeti(l, -1, 1);
				unit = LuaToNumber(l, -1);
				lua_pop(l, 1);
				*queue = malloc(sizeof(AiTransportRequest));
				(*queue)->Next = NULL;
				(*queue)->Unit = UnitSlots[unit];
				lua_rawgeti(l, -1, 2);
				CclParseOrder(l, &(*queue)->Order);
				lua_pop(l, 1);
				queue = &(*queue)->Next;
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "last-can-not-move-cycle")) {
			ai->LastCanNotMoveGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-type")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			i = 0;
			if (subargs) {
				ai->UnitTypeRequests = malloc(subargs / 2 * sizeof(AiUnitTypeTable));
			}
			for (k = 0; k < subargs; ++k) {
				const char* ident;
				int count;

				lua_rawgeti(l, j + 1, k + 1);
				ident = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				count = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->UnitTypeRequests[i].Table[0] = UnitTypeByIdent(ident);
				ai->UnitTypeRequests[i].Count = count;
				++i;
			}
			ai->UnitTypeRequestsCount = i;
		} else if (!strcmp(value, "upgrade")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			i = 0;
			if (subargs) {
				ai->UpgradeToRequests = malloc(subargs * sizeof(UnitType*));
			}
			for (k = 0; k < subargs; ++k) {
				const char* ident;

				lua_rawgeti(l, j + 1, k + 1);
				ident = LuaToString(l, -1);
				lua_pop(l, 1);
				ai->UpgradeToRequests[i] = UnitTypeByIdent(ident);
				++i;
			}
			ai->UpgradeToRequestsCount = i;
		} else if (!strcmp(value, "research")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			i = 0;
			if (subargs) {
				ai->ResearchRequests = malloc(subargs * sizeof(Upgrade*));
			}
			for (k = 0; k < subargs; ++k) {
				const char* ident;

				lua_rawgeti(l, j + 1, k + 1);
				ident = LuaToString(l, -1);
				lua_pop(l, 1);
				ai->ResearchRequests[i] = UpgradeByIdent(ident);
				++i;
			}
			ai->ResearchRequestsCount = i;
		} else if (!strcmp(value, "building")) {
			AiBuildQueue** queue;

			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			queue = &ai->UnitTypeBuilded;
			for (k = 0; k < subargs; ++k) {
				const char* ident;
				int made;
				int want;

				lua_rawgeti(l, j + 1, k + 1);
				ident = LuaToString(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				made = LuaToNumber(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				want = LuaToNumber(l, -1);
				lua_pop(l, 1);
				*queue = malloc(sizeof(AiBuildQueue));
				(*queue)->Next = NULL;
				(*queue)->Type = UnitTypeByIdent(ident);
				(*queue)->Want = want;
				(*queue)->Made = made;
				queue = &(*queue)->Next;
			}
		} else if (!strcmp(value, "repair-building")) {
			ai->LastRepairBuilding = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "repair-workers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, j + 1);
			for (k = 0; k < subargs; ++k) {
				int num;
				int workers;

				lua_rawgeti(l, j + 1, k + 1);
				num = LuaToNumber(l, -1);
				lua_pop(l, 1);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				workers = LuaToNumber(l, -1);
				lua_pop(l, 1);
				ai->TriedRepairWorkers[num] = workers;
				++i;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Register CCL features for unit-type.
*/
void AiCclRegister(void)
{
	// FIXME: Need to save memory here.
	// Loading all into memory isn't necessary.

	lua_register(Lua, "DefineAiHelper", CclDefineAiHelper);
	lua_register(Lua, "DefineAi", CclDefineAi);

	lua_register(Lua, "AiGetRace", CclAiGetRace);
	lua_register(Lua, "AiGetSleepCycles", CclAiGetSleepCycles);

	lua_register(Lua, "AiDebug", CclAiDebug);
	lua_register(Lua, "AiDebugPlayer", CclAiDebugPlayer);
	lua_register(Lua, "AiNeed", CclAiNeed);
	lua_register(Lua, "AiSet", CclAiSet);
	lua_register(Lua, "AiWait", CclAiWait);

	lua_register(Lua, "AiForce", CclAiForce);

	lua_register(Lua, "AiForceRole", CclAiForceRole);
	lua_register(Lua, "AiCheckForce", CclAiCheckForce);
	lua_register(Lua, "AiWaitForce", CclAiWaitForce);

	lua_register(Lua, "AiAttackWithForce", CclAiAttackWithForce);
	lua_register(Lua, "AiSleep", CclAiSleep);
	lua_register(Lua, "AiResearch", CclAiResearch);
	lua_register(Lua, "AiUpgradeTo", CclAiUpgradeTo);

	lua_register(Lua, "AiPlayer", CclAiPlayer);
	lua_register(Lua, "AiSetReserve", CclAiSetReserve);
	lua_register(Lua, "AiSetCollect", CclAiSetCollect);

	lua_register(Lua, "AiDump", CclAiDump);

	lua_register(Lua, "DefineAiWcNames", CclDefineAiWcNames);

	lua_register(Lua, "DefineAiPlayer", CclDefineAiPlayer);
}


//@}
