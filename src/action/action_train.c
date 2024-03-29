//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//           Stratagus - A free fantasy real time strategy game engine
//
/**@name action_train.c - The building train action. */
//
//      (c) Copyright 1998,2000-2004 by Lutz Sammer
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
//      $Id: action_train.c,v 1.70 2004/06/03 05:18:36 nehalmistry Exp $

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "stratagus.h"
#include "video.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "actions.h"
#include "missile.h"
#include "sound.h"
#include "ai.h"
#include "interface.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Unit can handle order.
**
**  @param unit   Newly trained unit.
**  @param order  New order for the unit.
**
**  @return  1 if the the unit can do it, 0 otherwise.
*/
int CanHandleOrder(Unit* unit, Order* order)
{
	if (order->Action == UnitActionResource) {
		//  Check if new unit can harvest.
		if (!unit->Type->Harvester) {
			return 0;
		}
		//  Also check if new unit can harvest this specific resource.
		if (order->Goal && !unit->Type->ResInfo[order->Goal->Type->GivesResource]) {
			return 0;
		}
		return 1;
	}
	if (order->Action == UnitActionAttack && !unit->Type->CanAttack) {
		return 0;
	}
	if (order->Action == UnitActionBoard && unit->Type->UnitType != UnitTypeLand) {
		return 0;
	}
	return 1;
}

/**
**  Unit trains unit!
**
**  @param unit  Unit that trains.
*/
void HandleActionTrain(Unit* unit)
{
	Unit* nunit;
	const UnitType* type;
	Player* player;
	int food;

	player = unit->Player;
	//
	// First entry
	//
	if (!unit->SubAction) {
		unit->Data.Train.Ticks = 0;
		unit->Data.Train.What[0] = unit->Orders[0].Type;
		unit->Data.Train.Count = 1;
		unit->SubAction = 1;
	}
	unit->Data.Train.Ticks += SpeedTrain;
	// FIXME: Should count down
	if (unit->Data.Train.Ticks >=
		unit->Data.Train.What[0]->Stats[player->Player].Costs[TimeCost]) {
		//
		// Check if there are still unit slots.
		//
		if (NumUnits >= UnitMax) {
			unit->Data.Train.Ticks =
				unit->Data.Train.What[0]->Stats[player->Player].Costs[TimeCost];
			unit->Reset = 1;
			unit->Wait = CYCLES_PER_SECOND / 6;
			return;
		}

		//
		// Check if enough supply available.
		//
		food = PlayerCheckLimits(player, unit->Data.Train.What[0]);
		if (food < 0) {
			if (food == -3 && unit->Player->AiEnabled) {
				AiNeedMoreSupply(unit, unit->Orders[0].Type);
			}

			unit->Data.Train.Ticks =
				unit->Data.Train.What[0]->Stats[player->Player].Costs[TimeCost];
			unit->Reset = 1;
			unit->Wait = CYCLES_PER_SECOND / 6;
			return;
		}

		nunit = MakeUnit(unit->Data.Train.What[0], player);
		nunit->X = unit->X;
		nunit->Y = unit->Y;
		type = unit->Type;

		// Some guy made DropOutOnSide set unit to belong to the building
		// training it. This was an ugly hack, setting X and Y is enough,
		// no need to add the unit only to be removed.
		nunit->X = unit->X;
		nunit->Y = unit->Y;

		DropOutOnSide(nunit, LookingW, type->TileWidth, type->TileHeight);

		// Set life span
		if (type->DecayRate) {
			nunit->TTL = GameCycle + type->DecayRate * 6 * CYCLES_PER_SECOND;
		}

		NotifyPlayer(player, NotifyYellow, nunit->X, nunit->Y,
			"New %s ready", nunit->Type->Name);
		if (player == ThisPlayer) {
			PlayUnitSound(nunit, VoiceReady);
		}
		if (unit->Player->AiEnabled) {
			AiTrainingComplete(unit, nunit);
		}

		unit->Reset = unit->Wait = 1;

		if (--unit->Data.Train.Count) {
			int z;
			for (z = 0; z < unit->Data.Train.Count; ++z) {
				unit->Data.Train.What[z] = unit->Data.Train.What[z + 1];
			}
			unit->Data.Train.Ticks = 0;
		} else {
			unit->Orders[0].Action = UnitActionStill;
			unit->SubAction = 0;
		}

		if (!CanHandleOrder(nunit, &unit->NewOrder)) {
			DebugPrint("Wrong order for unit\n");
#if 0
			nunit->Orders[0].Action = UnitActionStandStill;
#endif
			// Tell the unit to move instead of trying any funny stuff.
			nunit->Orders[0] = unit->NewOrder;
			nunit->Orders[0].Action = UnitActionMove;
			if (nunit->Orders[0].Goal) {
				RefsIncrease(nunit->Orders->Goal);
			}

		} else {
			if (unit->NewOrder.Goal) {
				if (unit->NewOrder.Goal->Destroyed) {
					// FIXME: perhaps we should use another goal?
					DebugPrint("Destroyed unit in train unit\n");
					RefsDecrease(unit->NewOrder.Goal);
					unit->NewOrder.Goal = NoUnitP;
					unit->NewOrder.Action = UnitActionStill;
				}
			}

			nunit->Orders[0] = unit->NewOrder;

			//
			// FIXME: Pending command uses any references?
			//
			if (nunit->Orders[0].Goal) {
				RefsIncrease(nunit->Orders->Goal);
			}
		}

		if (IsOnlySelected(unit)) {
			UpdateButtonPanel();
		}

		return;
	}

	unit->Reset = 1;
	unit->Wait = CYCLES_PER_SECOND / 6;
}

//@}
