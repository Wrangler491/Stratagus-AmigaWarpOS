//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//   T H E   W A R   B E G I N S
//    Stratagus - A free fantasy real time strategy game engine
//
/**@name script_spell.c - The spell script functions.. */
//
// (c) Copyright 1998-2003 by Joris Dauphin and Crestez Leonard
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
// $Id: script_spell.c,v 1.57 2004/06/25 17:39:05 jarod42 Exp $
//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
/*
#include "video.h"
#include "tileset.h"
#include "unittype.h"
*/
#include "spells.h"
#include "script_sound.h"
#include "script.h"

// **************************************************************************
// Action parsers for spellAction
// **************************************************************************

/**
**  Parse the missile location description for a spell action.
**
**  @param l         Lua state.
**  @param location  Pointer to missile location description.
**
**  @note This is only here to avoid code duplication. You don't have
**        any reason to USE this:)
*/
static void CclSpellMissileLocation(lua_State* l, SpellActionMissileLocation* location)
{
	const char* value;
	int args;
	int j;

	Assert(location != NULL);
	memset(location, 0, sizeof(*location));

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	j = 0;

	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "base")) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			if (!strcmp(value, "caster")) {
				location->Base = LocBaseCaster;
			} else if (!strcmp(value, "target")) {
				location->Base = LocBaseTarget;
			} else {
				LuaError(l, "Unsupported missile location base flag: %s" _C_ value);
			}
		} else if (!strcmp(value, "add-x")) {
			lua_rawgeti(l, -1, j + 1);
			location->AddX = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "add-y")) {
			lua_rawgeti(l, -1, j + 1);
			location->AddY = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "add-rand-x")) {
			lua_rawgeti(l, -1, j + 1);
			location->AddRandX = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "add-rand-y")) {
			lua_rawgeti(l, -1, j + 1);
			location->AddRandY = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else {
			LuaError(l, "Unsupported missile location description flag: %s" _C_ value);
		}
	}
}

/**
**  Parse the action for spell.
**
**  @param l            Lua state.
**  @param spellaction  Pointer to spellaction.
*/
static void CclSpellAction(lua_State* l, SpellActionType* spellaction)
{
	const char* value;
	int args;
	int j;

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	j = 0;

	lua_rawgeti(l, -1, j + 1);
	value = LuaToString(l, -1);
	lua_pop(l, 1);
	++j;

	if (!strcmp(value, "spawn-missile")) {
		spellaction->CastFunction = CastSpawnMissile;
		spellaction->Data.SpawnMissile.StartPoint.Base = LocBaseCaster;
		spellaction->Data.SpawnMissile.EndPoint.Base = LocBaseTarget;
		spellaction->Data.SpawnMissile.TTL = -1;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "damage")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.SpawnMissile.Damage = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "delay")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.SpawnMissile.Delay = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "ttl")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.SpawnMissile.TTL = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "start-point")) {
				lua_rawgeti(l, -1, j + 1);
				CclSpellMissileLocation(l, &spellaction->Data.SpawnMissile.StartPoint);
				lua_pop(l, 1);
			} else if (!strcmp(value, "end-point")) {
				lua_rawgeti(l, -1, j + 1);
				CclSpellMissileLocation(l, &spellaction->Data.SpawnMissile.EndPoint);
				lua_pop(l, 1);
			} else if (!strcmp(value, "missile")) {
				lua_rawgeti(l, -1, j + 1);
				value = LuaToString(l, -1);
				spellaction->Data.SpawnMissile.Missile = MissileTypeByIdent(value);
				if (spellaction->Data.SpawnMissile.Missile == NULL) {
					DebugPrint("in spawn-missile : missile %s does not exist\n" _C_ value);
				}
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported spawn-missile tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Data.SpawnMissile.Missile == NULL) {
			LuaError(l, "Use a missile for spawn-missile (with missile)");
		}
	} else if (!strcmp(value, "area-adjust-vitals")) {
		spellaction->CastFunction = CastAreaAdjustVitals;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "hit-points")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaAdjustVitals.HP = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "mana-points")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaAdjustVitals.Mana = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported area-adjust-vitals tag: %s" _C_ value);
			}
		}
	} else if (!strcmp(value, "area-bombardment")) {
		spellaction->CastFunction = CastAreaBombardment;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "fields")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaBombardment.Fields = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "shards")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaBombardment.Shards = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "damage")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaBombardment.Damage = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "start-offset-x")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaBombardment.StartOffsetX = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "start-offset-y")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AreaBombardment.StartOffsetY = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "missile")) {
				lua_rawgeti(l, -1, j + 1);
				value = LuaToString(l, -1);
				spellaction->Data.AreaBombardment.Missile = MissileTypeByIdent(value);
				if (spellaction->Data.AreaBombardment.Missile == NULL) {
					DebugPrint("in area-bombardement : missile %s does not exist\n" _C_ value);
				}
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported area-bombardment tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Data.AreaBombardment.Missile == NULL) {
			LuaError(l, "Use a missile for area-bombardment (with missile)");
		}
	} else if (!strcmp(value, "demolish")) {
		spellaction->CastFunction = CastDemolish;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "range")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.Demolish.Range = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "damage")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.Demolish.Damage = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported demolish tag: %s" _C_ value);
			}
		}
	} else if (!strcmp(value, "adjust-buffs")) {
		spellaction->CastFunction = CastAdjustBuffs;
		spellaction->Data.AdjustBuffs.HasteTicks = BUFF_NOT_AFFECTED;
		spellaction->Data.AdjustBuffs.SlowTicks = BUFF_NOT_AFFECTED;
		spellaction->Data.AdjustBuffs.BloodlustTicks = BUFF_NOT_AFFECTED;
		spellaction->Data.AdjustBuffs.InvisibilityTicks = BUFF_NOT_AFFECTED;
		spellaction->Data.AdjustBuffs.InvincibilityTicks = BUFF_NOT_AFFECTED;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "haste-ticks")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustBuffs.HasteTicks = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "slow-ticks")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustBuffs.SlowTicks = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "bloodlust-ticks")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustBuffs.BloodlustTicks = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "invisibility-ticks")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustBuffs.InvisibilityTicks = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "invincibility-ticks")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustBuffs.InvincibilityTicks = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported adjust-buffs tag: %s" _C_ value);
			}
		}
	} else if (!strcmp(value, "summon")) {
		spellaction->CastFunction = CastSummon;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "unit-type")) {
				lua_rawgeti(l, -1, j + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				spellaction->Data.Summon.UnitType = UnitTypeByIdent(value);
				if (!spellaction->Data.Summon.UnitType) {
					spellaction->Data.Summon.UnitType = 0;
					DebugPrint("unit type \"%s\" not found for summon spell.\n" _C_ value);
				}
			} else if (!strcmp(value, "time-to-live")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.Summon.TTL = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "require-corpse")) {
				spellaction->Data.Summon.RequireCorpse = 1;
				--j;
			} else {
				LuaError(l, "Unsupported summon tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Data.Summon.UnitType == NULL) {
			LuaError(l, "Use a unittype for summon (with unit-type)");
		}
	} else if (!strcmp(value, "spawn-portal")) {
		spellaction->CastFunction = CastSpawnPortal;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "portal-type")) {
				lua_rawgeti(l, -1, j + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				spellaction->Data.SpawnPortal.PortalType = UnitTypeByIdent(value);
				if (!spellaction->Data.SpawnPortal.PortalType) {
					spellaction->Data.SpawnPortal.PortalType = 0;
					DebugPrint("unit type \"%s\" not found for spawn-portal.\n" _C_ value);
				}
			} else {
				LuaError(l, "Unsupported spawn-portal tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Data.SpawnPortal.PortalType == NULL) {
			LuaError(l, "Use a unittype for spawn-portal (with portal-type)");
		}
	} else if (!strcmp(value, "polymorph")) {
		spellaction->CastFunction = CastPolymorph;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "new-form")) {
				lua_rawgeti(l, -1, j + 1);
				value = LuaToString(l, -1);
				lua_pop(l, 1);
				spellaction->Data.Polymorph.NewForm = UnitTypeByIdent(value);
				if (!spellaction->Data.Polymorph.NewForm) {
					spellaction->Data.Polymorph.NewForm= 0;
					DebugPrint("unit type \"%s\" not found for polymorph spell.\n" _C_ value);
				}
				// FIXME: temp polymorphs? hard to do.
			} else if (!strcmp(value, "player-neutral")) {
				spellaction->Data.Polymorph.PlayerNeutral = 1;
				--j;
			} else {
				LuaError(l, "Unsupported polymorph tag: %s" _C_ value);
			}
		}
		// Now, checking value.
		if (spellaction->Data.Polymorph.NewForm == NULL) {
			LuaError(l, "Use a unittype for polymorph (with new-form)");
		}
	} else if (!strcmp(value, "adjust-vitals")) {
		spellaction->CastFunction = CastAdjustVitals;
		for (; j < args; ++j) {
			lua_rawgeti(l, -1, j + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++j;
			if (!strcmp(value, "hit-points")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustVitals.HP = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "mana-points")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustVitals.Mana = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "max-multi-cast")) {
				lua_rawgeti(l, -1, j + 1);
				spellaction->Data.AdjustVitals.MaxMultiCast = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else {
				LuaError(l, "Unsupported adjust-vitals tag: %s" _C_ value);
			}
		}
	} else {
		LuaError(l, "Unsupported action type: %s" _C_ value);
	}
}

/**
**  Get a condition value from a scm object.
**
**  @param l      Lua state.
**  @param value  scm value to convert.
**
**  @return CONDITION_TRUE, CONDITION_FALSE, CONDITION_ONLY or -1 on error.
**  @note This is a helper function to make CclSpellCondition shorter
**        and easier to understand.
*/
char Ccl2Condition(lua_State* l, const char* value)
{
	if (!strcmp(value, "true")) {
		return CONDITION_TRUE;
	} else if (!strcmp(value, "false")) {
		return CONDITION_FALSE;
	} else if (!strcmp(value, "only")) {
		return CONDITION_ONLY;
	} else {
		LuaError(l, "Bad condition result: %s" _C_ value);
		return -1;
	}
}

/**
**  Parse the Condition for spell.
**
**  @param l          Lua state.
**  @param condition  pointer to condition to fill with data.
**
**  @note Conditions must be allocated. All data already in is LOST.
*/
static void CclSpellCondition(lua_State* l, ConditionInfo* condition)
{
	const char* value;
	int i;
	int args;
	int j;

	//
	// Initializations:
	//

	// Set everything to 0:
	memset(condition, 0, sizeof(ConditionInfo));
	// Flags are defaulted to 0(CONDITION_TRUE)
	condition->BoolFlag = calloc(NumberBoolFlag, sizeof(*condition->BoolFlag));
	// Initialize min/max stuff to values with no effect.
	condition->MinHpPercent = -10;
	condition->MaxHpPercent = 1000;
	condition->MinManaPercent = -10;
	condition->MaxManaPercent = 1000;
	//  Buffs too.
	condition->MaxHasteTicks = 0xFFFFFFF;
	condition->MaxSlowTicks = 0xFFFFFFF;
	condition->MaxBloodlustTicks = 0xFFFFFFF;
	condition->MaxInvisibilityTicks = 0xFFFFFFF;
	condition->MaxInvincibilityTicks = 0xFFFFFFF;
	//  Now parse the list and set values.
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	args = luaL_getn(l, -1);
	for (j = 0; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		value = LuaToString(l, -1);
		lua_pop(l, 1);
		++j;
		if (!strcmp(value, "coward")) {
			lua_rawgeti(l, -1, j + 1);
			condition->Coward = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "alliance")) {
			lua_rawgeti(l, -1, j + 1);
			condition->Alliance = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "opponent")) {
			lua_rawgeti(l, -1, j + 1);
			condition->Opponent = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "building")) {
			lua_rawgeti(l, -1, j + 1);
			condition->Building = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "self")) {
			lua_rawgeti(l, -1, j + 1);
			condition->TargetSelf = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "min-hp-percent")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MinHpPercent = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-hp-percent")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxHpPercent = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "min-mana-percent")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MinManaPercent = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-mana-percent")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxManaPercent = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-slow-ticks")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxSlowTicks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-haste-ticks")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxHasteTicks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-bloodlust-ticks")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxBloodlustTicks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-invisibility-ticks")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxInvisibilityTicks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "max-invincibility-ticks")) {
			lua_rawgeti(l, -1, j + 1);
			condition->MaxInvincibilityTicks = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else {
			for (i = 0; i < NumberBoolFlag; ++i) { // User defined flags
				if (!strcmp(value, BoolFlagName[i])) {
					lua_rawgeti(l, -1, j + 1);
					condition->BoolFlag[i] = Ccl2Condition(l, LuaToString(l, -1));
					lua_pop(l, 1);
					break;
				}
			}
			if (i != NumberBoolFlag) {
				continue;
			}
			LuaError(l, "Unsuported condition tag: %s" _C_ value);
		}
	}
}

/*
** Parse the Condition for spell.
**
** @param list SCM object to parse
** @param autocast pointer to autocast to fill with data.
**
** @notes: autocast must be allocated. All data already in is LOST.
*/
static void CclSpellAutocast(lua_State* l, AutoCastInfo* autocast)
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
		if (!strcmp(value, "range")) {
			lua_rawgeti(l, -1, j + 1);
			autocast->Range = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "combat")) {
			lua_rawgeti(l, -1, j + 1);
			autocast->Combat = Ccl2Condition(l, LuaToString(l, -1));
			lua_pop(l, 1);
		} else if (!strcmp(value, "condition")) {
			if (!autocast->Condition) {
				autocast->Condition = (ConditionInfo*)malloc(sizeof(ConditionInfo));
			}
			lua_rawgeti(l, -1, j + 1);
			CclSpellCondition(l, autocast->Condition);
			lua_pop(l, 1);
		} else {
			LuaError(l, "Unsupported autocast tag: %s" _C_ value);
		}
	}
}

/**
**  Parse Spell.
**
**  @param l  Lua state.
*/
static int CclDefineSpell(lua_State* l)
{
	const char* identname;
	SpellType* spell;
	const char* value;
	SpellActionType* act;
	int args;
	int i;

	args = lua_gettop(l);
	identname = LuaToString(l, 1);
	spell = SpellTypeByIdent(identname);
	if (spell != NULL) {
		DebugPrint("Redefining spell-type `%s'\n" _C_ identname);
	} else {
		SpellTypeTable = realloc(SpellTypeTable, (1 + SpellTypeCount) * sizeof(SpellType*));
		spell = SpellTypeTable[SpellTypeCount] = malloc(sizeof(SpellType));
		memset(spell, 0, sizeof(SpellType));
		spell->Slot = SpellTypeCount;
		spell->Ident = strdup(identname);
		spell->DependencyId = -1;
		for (i = 0; i < NumUnitTypes; ++i) { // adjust array for caster already defined
			if (UnitTypes[i]->CanCastSpell) {
				UnitTypes[i]->CanCastSpell = realloc(UnitTypes[i]->CanCastSpell,
					SpellTypeCount * sizeof((*UnitTypes)->CanCastSpell));
				UnitTypes[i]->CanCastSpell[SpellTypeCount] = 0;
			}
			if (UnitTypes[i]->AutoCastActive) {
				UnitTypes[i]->AutoCastActive = realloc(UnitTypes[i]->AutoCastActive,
					SpellTypeCount * sizeof((*UnitTypes)->AutoCastActive));
				UnitTypes[i]->AutoCastActive[SpellTypeCount] = 0;
			}
		}
		SpellTypeCount++;
	}
	for (i = 1; i < args; ++i) {
		value = LuaToString(l, i + 1);
		++i;
		if (!strcmp(value, "showname")) {
			if (spell->Name) {
					free(spell->Name);
			}
			spell->Name = strdup(LuaToString(l, i + 1));
		} else if (!strcmp(value, "manacost")) {
			spell->ManaCost = LuaToNumber(l, i + 1);
		} else if (!strcmp(value, "range")) {
			if (!lua_isstring(l, i + 1) && !lua_isnumber(l, i + 1)) {
				LuaError(l, "incorrect argument");
			}
			if (lua_isstring(l, i + 1) && !strcmp(lua_tostring(l, i + 1), "infinite")) {
				spell->Range = INFINITE_RANGE;
			} else if (lua_isnumber(l, i + 1)) {
				spell->Range = lua_tonumber(l, i + 1);
			} else {
				LuaError(l, "Invalid range");
			}
		} else if (!strcmp(value, "repeat-cast")) {
			spell->RepeatCast = 1;
			--i;
		} else if (!strcmp(value, "target")) {
			value = LuaToString(l, i + 1);
			if (!strcmp(value, "self")) {
				spell->Target = TargetSelf;
			} else if (!strcmp(value, "unit")) {
				spell->Target = TargetUnit;
			} else if (!strcmp(value, "position")) {
				spell->Target = TargetPosition;
			} else {
				LuaError(l, "Unsupported spell target type tag: %s" _C_ value);
			}
		} else if (!strcmp(value, "action")) {
			int subargs;
			int k;

			spell->Action = (SpellActionType*)malloc(sizeof(SpellActionType));
			act = spell->Action;
			memset(act, 0, sizeof(SpellActionType));
			if (!lua_istable(l, i + 1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = luaL_getn(l, i + 1);
			k = 0;
			lua_rawgeti(l, i + 1, k + 1);
			CclSpellAction(l, act);
			lua_pop(l, 1);
			++k;
			for (; k < subargs; ++k) {
				act->Next = (SpellActionType*)malloc(sizeof(SpellActionType));
				act = act->Next;
				memset(act, 0, sizeof(SpellActionType));
				lua_rawgeti(l, i + 1, k + 1);
				CclSpellAction(l, act);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "condition")) {
			if (!spell->Condition) {
				spell->Condition = (ConditionInfo*)malloc(sizeof(ConditionInfo));
			}
			lua_pushvalue(l, i + 1);
			CclSpellCondition(l, spell->Condition);
			lua_pop(l, 1);
		} else if (!strcmp(value, "autocast")) {
			if (!spell->AutoCast) {
				spell->AutoCast = (AutoCastInfo*)malloc(sizeof(AutoCastInfo));
				memset(spell->AutoCast, 0, sizeof(AutoCastInfo));
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->AutoCast);
			lua_pop(l, 1);
		} else if (!strcmp(value, "ai-cast")) {
			if (!spell->AICast) {
				spell->AICast = (AutoCastInfo*)malloc(sizeof(AutoCastInfo));
				memset(spell->AICast, 0, sizeof(AutoCastInfo));
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->AICast);
			lua_pop(l, 1);
		} else if (!strcmp(value, "sound-when-cast")) {
			//  Free the old name, get the new one
			if (spell->SoundWhenCast.Name) {
				free(spell->SoundWhenCast.Name);
			}
			spell->SoundWhenCast.Name = strdup(LuaToString(l, i + 1));
			spell->SoundWhenCast.Sound = SoundIdForName(spell->SoundWhenCast.Name);
			//  Check for sound.
			if (!spell->SoundWhenCast.Sound) {
				free(spell->SoundWhenCast.Name);
				spell->SoundWhenCast.Name = 0;
			}
		} else if (!strcmp(value, "depend-upgrade")) {
			value = LuaToString(l, i + 1);
			spell->DependencyId = UpgradeIdByIdent(value);
			if (spell->DependencyId == -1) {
				lua_pushfstring(l, "Bad upgrade name: %s", value);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
** Register CCL features for Spell.
*/
void SpellCclRegister(void)
{
	lua_register(Lua, "DefineSpell", CclDefineSpell);
}

#if 0 // Use old ccl config.

/**
** Save a spell action to a file.
**
**  @param file File pointer to save to
** @param action Pointer to action to save.
*/
static void SaveSpellAction(CLFile* file, SpellActionType* action)
{
	SpellActionMissileLocation* loc;

	if (action->CastFunction == CastAreaBombardment) {
		CLprintf(file, "(area-bombardment fields %d shards %d damage %d start-offset-x %d start-offset-y %d)",
				action->Data.AreaBombardment.Fields,
				action->Data.AreaBombardment.Shards,
				action->Data.AreaBombardment.Damage,
				action->Data.AreaBombardment.StartOffsetX,
				action->Data.AreaBombardment.StartOffsetY);
	} else if (action->CastFunction == CastAreaAdjustVitals) {
		CLprintf(file, "(area-adjust-vitals");
		if (action->Data.AreaAdjustVitals.HP) {
			CLprintf(file, " hit-points %d", action->Data.AdjustVitals.HP);
		}
		if (action->Data.AreaAdjustVitals.Mana) {
			CLprintf(file, " mana-points %d", action->Data.AdjustVitals.Mana);
		}
		CLprintf(file, ")\n");
	} else if (action->CastFunction == CastSpawnMissile) {
		CLprintf(file, "(spawn-missile delay %d ttl %d damage %d ",
				action->Data.SpawnMissile.Delay,
				action->Data.SpawnMissile.TTL,
				action->Data.SpawnMissile.Damage);
		//
		// Save start-point
		//
		loc=&action->Data.SpawnMissile.StartPoint;
		CLprintf(file, "start-point (base ");
		if (loc->Base==LocBaseCaster) {
			CLprintf(file, "caster");
		} else {
			CLprintf(file, "target");
		}
		CLprintf(file, " add-x %d add-y %d add-rand-x %d add-rand-y %d) ",
				loc->AddX,loc->AddY,loc->AddRandX,loc->AddRandY);
		//
		// Save end-point
		//
		loc=&action->Data.SpawnMissile.EndPoint;
		CLprintf(file, "end-point (base ");
		if (loc->Base==LocBaseCaster) {
			CLprintf(file, "caster");
		} else {
			CLprintf(file, "target");
		}
		CLprintf(file, " add-x %d add-y %d add-rand-x %d add-rand-y %d)",
				loc->AddX,loc->AddY,loc->AddRandX,loc->AddRandY);
		CLprintf(file, ")");
	} else if (action->CastFunction == CastAdjustVitals) {
		CLprintf(file, "(adjust-vitals");
		if (action->Data.AdjustVitals.HP) {
			CLprintf(file, " hit-points %d", action->Data.AdjustVitals.HP);
		}
		if (action->Data.AdjustVitals.Mana) {
			CLprintf(file, " mana-points %d", action->Data.AdjustVitals.Mana);
		}
		if (action->Data.AdjustVitals.MaxMultiCast) {
			CLprintf(file, " max-multi-cast %d", action->Data.AdjustVitals.MaxMultiCast);
		}
		CLprintf(file, ")\n");
	} else if (action->CastFunction == CastSummon) {
		CLprintf(file, "(summon unit-type %s time-to-live %d",
				action->Data.Summon.UnitType->Ident,
				action->Data.Summon.TTL);
		if (action->Data.Summon.RequireCorpse) {
			CLprintf(file, " require-corpse ");
		}
		CLprintf(file, ")\n");
	} else if (action->CastFunction == CastDemolish) {
		CLprintf(file, "(demolish range %d damage %d)\n",
				action->Data.Demolish.Range,
				action->Data.Demolish.Damage);
	} else if (action->CastFunction == CastAdjustBuffs) {
		CLprintf(file, "(adjust-buffs");
		if (action->Data.AdjustBuffs.HasteTicks != BUFF_NOT_AFFECTED) {
			CLprintf(file, " haste-ticks %d", action->Data.AdjustBuffs.HasteTicks);
		}
		if (action->Data.AdjustBuffs.SlowTicks != BUFF_NOT_AFFECTED) {
			CLprintf(file, " slow-ticks %d", action->Data.AdjustBuffs.SlowTicks);
		}
		if (action->Data.AdjustBuffs.BloodlustTicks != BUFF_NOT_AFFECTED) {
			CLprintf(file, " bloodlust-ticks %d", action->Data.AdjustBuffs.BloodlustTicks);
		}
		if (action->Data.AdjustBuffs.InvisibilityTicks != BUFF_NOT_AFFECTED) {
			CLprintf(file, " invisibility-ticks %d", action->Data.AdjustBuffs.InvisibilityTicks);
		}
		if (action->Data.AdjustBuffs.InvincibilityTicks != BUFF_NOT_AFFECTED) {
			CLprintf(file, " invincibility-ticks %d", action->Data.AdjustBuffs.InvincibilityTicks);
		}
		CLprintf(file, ")");
	} else if (action->CastFunction == CastPolymorph) {
		CLprintf(file, "(polymorph new-form %s)",
				action->Data.Polymorph.NewForm->Ident);
	} else if (action->CastFunction == CastSpawnPortal) {
		CLprintf(file, "(spawn-portal portal-type %s)",
				action->Data.SpawnPortal.PortalType->Ident);
	}
}

/**
**  Save a spell condition to a file.
**
**  @param file       File pointer to save to
**  @param condition  Pointer to condition to save.
*/
static void SaveSpellCondition(CLFile* file, ConditionInfo* condition)
{
	char condstrings[3][10] = {
		"true", /// CONDITION_TRUE
		"false", /// CONDITION_FALSE
		"only" /// CONDITION_ONLY
	};
	int i;
	Assert(file);
	Assert(condition);

	CLprintf(file, "( ");
	//
	// First save data related to flags.
	// NOTE: (int) is there to keep compilers happy.
	//
	if (condition->Coward != CONDITION_TRUE) {
		CLprintf(file, "coward %s ", condstrings[(int)condition->Coward]);
	}
	if (condition->Alliance != CONDITION_TRUE) {
		CLprintf(file, "alliance %s ", condstrings[(int)condition->Alliance]);
	}
	if (condition->Building != CONDITION_TRUE) {
		CLprintf(file, "building %s ", condstrings[(int)condition->Building]);
	}
	if (condition->TargetSelf != CONDITION_TRUE) {
		CLprintf(file, "self %s ", condstrings[(int)condition->TargetSelf]);
	}
	for (i = 0; i < NumberBoolFlag; ++i) { // User defined flags
		if (condition->BoolFlag[i] != CONDITION_TRUE) {
			CLprintf(file, "%s %s ",
				BoolFlagName[i], condstrings[(int)condition->BoolFlag[i]]);
		}
	}
	//
	// Min/Max vital percents
	//
	CLprintf(file, "min-hp-percent %d ", condition->MinHpPercent);
	CLprintf(file, "max-hp-percent %d ", condition->MaxHpPercent);
	CLprintf(file, "min-mana-percent %d ", condition->MinManaPercent);
	CLprintf(file, "max-mana-percent %d ", condition->MaxManaPercent);
	//
	// Max buff ticks stuff
	//
	CLprintf(file, "max-slow-ticks %d ", condition->MaxSlowTicks);
	CLprintf(file, "max-haste-ticks %d ", condition->MaxHasteTicks);
	CLprintf(file, "max-bloodlust-ticks %d ", condition->MaxBloodlustTicks);
	CLprintf(file, "max-invisibility-ticks %d ", condition->MaxInvisibilityTicks);
	CLprintf(file, "max-invincibility-ticks %d ", condition->MaxInvincibilityTicks);
	//
	// The end.
	//
	CLprintf(file, ")\n");
}

/**
** Save autocast info to a CCL file
**
** @param file The file to save to.
** @param autocast Auocastinfo to save.
*/
void SaveSpellAutoCast(CLFile* file, AutoCastInfo* autocast)
{
	char condstrings[3][10] = {
		"true", /// CONDITION_TRUE
		"false", /// CONDITION_FALSE
		"only" /// CONDITION_ONLY
	};

	CLprintf(file, "( range %d ", autocast->Range);
	if (autocast->Combat != CONDITION_TRUE) {
		CLprintf(file, "combat %s ", condstrings[(int)autocast->Combat]);
	}
	if (autocast->Condition) {
		CLprintf(file, " condition ");
		SaveSpellCondition(file, autocast->Condition);
	}
	CLprintf(file, " )\n");
}

#endif

//@}
