//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________      ______________________
//   T H E   W A R   B E G I N S
//    Stratagus - A free fantasy real time strategy game engine
//
/**@name selection.c - The units' selection. */
//
// (c) Copyright 1999-2003 by Patrice Fortier, Lutz Sammer
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
// $Id: selection.c,v 1.79 2004/06/25 17:39:05 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "interface.h"
#include "map.h"
#include "ui.h"
#include "commands.h"
#include "network.h"

#include "script.h"

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

int NumSelected;                 ///< Number of selected units
int TeamNumSelected[PlayerMax];  ///< How many units selected
int MaxSelectable;               ///< Maximum number of selected units
Unit** Selected;                 ///< All selected units
Unit** TeamSelected[PlayerMax];  ///< teams currently selected units


static unsigned GroupId;         ///< Unique group # for automatic groups

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

/**
** Unselect all the units in the current selection
*/
void UnSelectAll(void)
{
	Unit* unit;

	while (!++GroupId) { // Advance group id, but keep non zero
	}

	while (NumSelected) {
		unit = Selected[--NumSelected];
		Selected[NumSelected] = NoUnitP; // FIXME: only needed for old code
		unit->Selected = 0;
	}

}

/**
** Handle a suicide unit click
**
** @param unit suicide unit.
*/
static void HandleSuicideClick(Unit* unit)
{
	Assert(unit->Type->ClicksToExplode);
	if (GameObserve) {
		return;
	}

	if (NumSelected == 1 && Selected[0] == unit) {
		unit->Value++;
	} else {
		unit->Value = 1;
	}

	// FIXME: make this configurable
	if (unit->Value == unit->Type->ClicksToExplode) {
		SendCommandDismiss(unit);
		unit->Value = 0;
	}
}

/**
** Replace a group of selected units by an other group of units.
**
** @param units    Array of units to be selected.
** @param count    Number of units in array to be selected.
*/
void ChangeSelectedUnits(Unit** units,int count)
{
	Unit* unit;
	int i;
	int n;

	Assert(count <= MaxSelectable);

	if (count == 1 && units[0]->Type->ClicksToExplode &&
		!units[0]->Type->Decoration) {
		HandleSuicideClick(units[0]);
	}

	UnSelectAll();
	NetworkSendSelection(units, count);
	for (n = i = 0; i < count; ++i) {
		if (!units[i]->Removed && !units[i]->TeamSelected && !units[i]->Type->Decoration) {
			Selected[n++] = unit = units[i];
			unit->Selected = 1;
			if (count > 1) {
				unit->LastGroup = GroupId;
			}
		}
	}
	NumSelected = n;
}

/**
**  Change A Unit Selection from my Team
**
**  @param player  The Player who is selecting the units
**  @param units   The Units to add/remove
**  @param adjust  0 = reset, 1 = remove units, 2 = add units
**  @param count   the number of units to be adjusted
*/
void ChangeTeamSelectedUnits(Player* player, Unit** units, int adjust, int count)
{
	int i;
	int n;
	Unit* unit;

	switch (adjust) {
		case 0:
			// UnSelectAllTeam(player);
			while (TeamNumSelected[player->Player]) {
				unit = TeamSelected[player->Player][--TeamNumSelected[player->Player]];
				unit->TeamSelected &= ~(1 << player->Player);
				TeamSelected[player->Player][TeamNumSelected[player->Player]] = NoUnitP; // FIXME: only needed for old code
			}
			// FALL THROUGH
		case 2:
			for (i = 0; i < count; ++i) {
				if (!units[i]->Removed && !units[i]->Type->Decoration) {
					TeamSelected[player->Player][TeamNumSelected[player->Player]++] = units[i];
					units[i]->TeamSelected |= 1 << player->Player;
				}
			}
			Assert(TeamNumSelected[player->Player] <= MaxSelectable);
			break;
		case 1:
			for (n = 0; n < TeamNumSelected[player->Player]; ++n) {
				for (i = 0; i < count; ++i) {
					if (units[i] == TeamSelected[player->Player][n]) {
						TeamSelected[player->Player][n] =
							TeamSelected[player->Player][TeamNumSelected[player->Player]--];
					}
				}
			}
			Assert(TeamNumSelected[player->Player] >= 0);
			break;
		default:
			Assert(0);
	}
}

/**
** Add a unit to the other selected units.
**
** @param unit Pointer to unit to add.
** @return true if added to selection, false otherwise
** (if NumSelected == MaxSelectable or
** unit is already selected or unselectable)
*/
int SelectUnit(Unit* unit)
{
	if (unit->Type->Revealer) { // Revealers cannot be selected
		DebugPrint("Selecting revealer?\n");
		return 0;
	}

	if (unit->Removed) { // Removed cannot be selected
		DebugPrint("Selecting removed?\n");
		return 0;
	}

	if (NumSelected == MaxSelectable) {
		return 0;
	}

	if (unit->Selected) {
		return 0;
	}

	if (unit->Type->Decoration && GameRunning) {
		return 0;
	}

	Selected[NumSelected++] = unit;
	unit->Selected = 1;
	if (NumSelected > 1) {
		Selected[0]->LastGroup = unit->LastGroup = GroupId;
	}
	return 1;
}

/**
** Select a single unit, unselecting the previous ones
**
** @param unit Pointer to unit to be selected.
*/
void SelectSingleUnit(Unit* unit)
{
	ChangeSelectedUnits(&unit, 1);
}

/**
** Unselect unit
**
** @param unit Pointer to unit to be unselected.
*/
void UnSelectUnit(Unit* unit)
{
	int i;
	int j;

	if (unit->TeamSelected) {
		for (i = 0; i < PlayerMax; ++i) {
			if (unit->TeamSelected & (1 << i)) {
				for (j = 0; TeamSelected[i][j] != unit; ++i) {
					;
				}
				Assert(j < TeamNumSelected[i]);

				if (j < --TeamNumSelected[i]) {

					TeamSelected[i][j] = TeamSelected[i][TeamNumSelected[i]];
				}
				unit->TeamSelected &= ~(1 << i);
			}
		}
	}
	if (!unit->Selected) {
		return;
	}

	for (i = 0; Selected[i] != unit; ++i) {
		;
	}
	Assert(i < NumSelected);

	if (i < --NumSelected) {
		Selected[i] = Selected[NumSelected];
	}

	if (NumSelected > 1) { // Assign new group to remaining units
		while (!++GroupId) { // Advance group id, but keep non zero
		}
		for (i = 0; i < NumSelected; ++i) {
			Selected[i]->LastGroup = GroupId;
		}
	}

	Selected[NumSelected] = NoUnitP; // FIXME: only needed for old code
	unit->Selected = 0;
}

/**
** Toggle the selection of a unit in a group of selected units
**
** @param unit Pointer to unit to be toggled.
** @return 0 if unselected, 1 otherwise
*/
int ToggleSelectUnit(Unit* unit)
{
	if (unit->Selected) {
		UnSelectUnit(unit);
		return 0;
	}
	SelectUnit(unit);
	return 1;
}

/**
 ** Select units from a particular type and belonging to the local player.
 **
 ** The base is included in the selection and defines
 ** the type of the other units to be selected.
 **
 ** @param base Select all units of same type.
 ** @return Number of units found, 0 means selection unchanged
 **
 ** @todo
 ** FIXME: 0 can't happen. Maybe when scripting will use it?
 **
 ** FIXME: should always select the nearest 9 units to the base!
 */
int SelectUnitsByType(Unit* base)
{
	Unit* unit;
	Unit* table[UnitMax];
	const UnitType* type;
	int r;
	int i;
	const Viewport* vp;

	Assert(TheUI.MouseViewport);

	type = base->Type;

	// select all visible units.
	// StephanR: should be (MapX,MapY,MapX+MapWidth-1,MapY+MapHeight-1) ???
	/* FIXME: this should probably be cleaner implemented if SelectUnitsByType()
	 * took parameters of the selection rectangle as arguments */
	vp = TheUI.MouseViewport;
	r = UnitCacheSelect(vp->MapX - 1, vp->MapY - 1, vp->MapX + vp->MapWidth + 1,
		vp->MapY + vp->MapHeight + 1, table);

	// if unit is a cadaver or hidden (not on map)
	// no unit can be selected.
	if (base->Removed || base->Orders[0].Action == UnitActionDie) {
		return 0;
	}

	if (base->Type->ClicksToExplode) {
		HandleSuicideClick(base);
	}

	if (base->Type->Decoration && GameRunning) {
		return 0;
	}
	if (base->TeamSelected) { // Somebody else onteam has this unit
		return 0;
	}

	UnSelectAll();
	Selected[0] = base;
	base->Selected = 1;
	NumSelected = 1;

	// if unit isn't belonging to the player or allied player, or is a static unit
	// (like a building), only 1 unit can be selected at the same time.
	if (!CanSelectMultipleUnits(base->Player) || !type->SelectableByRectangle) {
		return NumSelected;
	}

	//
	// Search for other visible units of the same type
	//
	// FIXME: peon/peasant with gold/wood & co are considered from
	//   different type... idem for tankers
	for (i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) || unit->Type != type) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit == base) {  // no need to have the same unit twice :)
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		Selected[NumSelected++] = unit;
		unit->Selected = 1;
		if (NumSelected == MaxSelectable) {
			break;
		}
	}

	if (NumSelected > 1) {
		for (i = 0; i < NumSelected; ++i) {
			Selected[i]->LastGroup = GroupId;
		}
	}

	NetworkSendSelection(Selected, NumSelected);
	return NumSelected;
}

/**
** Toggle units from a particular type and belonging to the local player.
**
** The base is included in the selection and defines
** the type of the other units to be selected.
**
** @param base Toggle all units of same type.
** @return Number of units found, 0 means selection unchanged
**
** @todo
** FIXME: toggle not written
**
** FIXME: should always select the nearest 9 units to the base!
*/
int ToggleUnitsByType(Unit* base)
{
	Unit* unit;
	Unit* table[UnitMax];
	const UnitType* type;
	int r;
	int i;

	type = base->Type;

	// select all visible units.
	// StephanR: should be (MapX,MapY,MapX+MapWidth-1,MapY+MapHeight-1) ???
	// FIXME: this should probably be cleaner implemented if SelectUnitsByType()
	// took parameters of the selection rectangle as arguments */
	r = UnitCacheSelect(TheUI.MouseViewport->MapX - 1,
		TheUI.MouseViewport->MapY - 1,
		TheUI.MouseViewport->MapX + TheUI.MouseViewport->MapWidth + 1,
		TheUI.MouseViewport->MapY + TheUI.MouseViewport->MapHeight + 1, table);

	// if unit is a cadaver or hidden (not on map)
	// no unit can be selected.
	if (base->Removed || base->Orders[0].Action == UnitActionDie) {
		return 0;
	}
	// if unit isn't belonging to the player, or is a static unit
	// (like a building), only 1 unit can be selected at the same time.
	if (!CanSelectMultipleUnits(base->Player) || !type->SelectableByRectangle) {
		return 0;
	}

	if (!SelectUnit(base)) { // Add base to selection
		return 0;
	}
	//
	//  Search for other visible units of the same type
	//
	// FIXME: peon/peasant with gold/wood & co are considered from
	// different type... idem for tankers
	for (i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) || unit->Type != type) {
			continue;
		}
		if (UnitUnusable(unit)) { // guess SelectUnits doesn't check this
			continue;
		}
		if (unit == base) { // no need to have the same unit twice
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		if (!SelectUnit(unit)) { // add unit to selection
			return NumSelected;
		}
	}

	NetworkSendSelection(Selected, NumSelected);
	return NumSelected;
}

/**
** Change selected units to units from group #group_number
** Doesn't change the selection if the group has no unit.
**
** @param group_number number of the group to be selected.
** @return number of units in the group.
*/
int SelectGroup(int group_number)
{
	int nunits;

	Assert(group_number <= NUM_GROUPS);

	if (!(nunits = GetNumberUnitsOfGroup(group_number))) {
		return 0;
	}

	ChangeSelectedUnits(GetUnitsOfGroup(group_number), nunits);
	return NumSelected;
}

/**
** Add units from group of a particular unit to selection.
**
** @param unit unit belonging to the group to be selected.
** @return 0 if the unit doesn't belong to a group,
** or the number of units in the group.
*/
int AddGroupFromUnitToSelection(Unit* unit)
{
	int i;
	int group;

	if (!(group = unit->LastGroup)) { // belongs to no group
		return 0;
	}

	for (i = 0; i < NumUnits; ++i) {
		if (Units[i]->LastGroup == group && !Units[i]->Removed) {
			SelectUnit(Units[i]);
			if (NumSelected == MaxSelectable) {
				return NumSelected;
			}
		}
	}
	return NumSelected;
}

/**
** Select units from group of a particular unit.
** Doesn't change the selection if the group has no unit,
** or the unit doesn't belong to any group.
**
** @param unit unit belonging to the group to be selected.
** @return 0 if the unit doesn't belong to a group,
** or the number of units in the group.
*/
int SelectGroupFromUnit(Unit* unit)
{
	if (!unit->LastGroup) { // belongs to no group
		return 0;
	}

	UnSelectAll();
	return AddGroupFromUnitToSelection(unit);
}

/**
 ** Select the units selecteable by rectangle in a local table.
 ** Act like a filter: The source table is modified.
 ** Return the original table if no unit is found.
 **
 ** @param table        Input/Output table of units.
 ** @param num_units    Number of units in input table.
 ** @return             the number of units found.
 */
static int SelectOrganicUnitsInTable(Unit** table,int num_units)
{
	Unit* unit;
	int n;
	int i;

	for (n = i = 0; i < num_units; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) || !unit->Type->SelectableByRectangle) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		table[n++] = unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	return n;
}

/**
** Selects units from the table whose sprite is at least partially
** covered by the rectangle. The rectangle is determined by coordinates
** of its upper left and lower right corner expressed in screen map
** coordinate system.
**
** @param sx0          x-coord of upper left corner of the rectangle
** @param sy0          y-coord of upper left corner of the rectangle
** @param sx1          x-coord of lower right corner of the rectangle
** @param sy1          y-coord of lower right corner of the rectangle
** @param table        table of units
** @param num_units    number of units in table
**
** @return number of units found
*/
static int SelectSpritesInsideRectangle (int sx0, int sy0, int sx1, int sy1,
	Unit** table, int num_units)
{
	int n;
	int i;

	for (i = n = 0; i < num_units; ++i) {
		int sprite_x;
		int sprite_y;
		Unit* unit;
		const UnitType* type;

		unit = table[i];
		type = unit->Type;
		sprite_x = unit->X * TileSizeX + unit->IX;
		sprite_x -= (type->BoxWidth - TileSizeX * type->TileWidth) / 2;
		sprite_y = unit->Y*TileSizeY + unit->IY;
		sprite_y -= (type->BoxHeight - TileSizeY * type->TileHeight) / 2;
		if (sprite_x + type->BoxWidth < sx0) {
			continue;
		}
		if (sprite_x > sx1) {
			continue;
		}
		if (sprite_y + type->BoxHeight < sy0) {
			continue;
		}
		if (sprite_y > sy1) {
			continue;
		}
		table[n++] = unit;
	}
	return n;
}

/**
 ** Add the units in the rectangle to the current selection
 **
 ** @param x0     X start of selection rectangle in tile coordinates
 ** @param y0     Y start of selection rectangle in tile coordinates
 ** @param x1     X start of selection rectangle in tile coordinates
 ** @param y1     Y start of selection rectangle in tile coordinates
 ** @return       the _total_ number of units selected.
 */
int AddSelectedUnitsInRectangle(int x0, int y0, int x1, int y1)
{
	Unit* table[UnitMax];
	int toggle_num;
	int n;
	int i;

	// If there is no selected unit yet, do a simple selection.
	if (!NumSelected) {
		return SelectUnitsInRectangle(x0, y0, x1, y1);
	}

	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (NumSelected == 1 &&
			(!CanSelectMultipleUnits(Selected[0]->Player) ||
				!Selected[0]->Type->SelectableByRectangle)) {
		return NumSelected;
	}

	// If no unit in rectangle area... do nothing
	toggle_num = UnitCacheSelect((x0 / TileSizeX) - 2, (y0 / TileSizeY) - 2,
		(x1 / TileSizeX) + 2 + 1, (y1 / TileSizeX) + 2 + 1, table);
	if (!toggle_num) {
		return NumSelected;
	}
	toggle_num = SelectSpritesInsideRectangle(x0, y0, x1, y1, table, toggle_num);
	if (!toggle_num) {
		return NumSelected;
	}

	// Now we should only have mobile (organic) units belonging to us,
	// so if there's no such units in the rectangle, do nothing.
	if (!(n = SelectOrganicUnitsInTable(table, toggle_num))) {
		return NumSelected;
	}

	for (i = 0; i < n && NumSelected < MaxSelectable; ++i) {
		SelectUnit(table[i]);
	}
	return NumSelected;
}

/**
** Select units in a rectangle.
** Proceed in order in none found:
**   @li select local player mobile units
**   @li select one local player static unit (random)
**   @li select one neutral unit (critter, mine...)
**   @li select one enemy unit (random)
**
** @param sx0    X start of selection rectangle in tile coordinates
** @param sy0    Y start of selection rectangle in tile coordinates
** @param sx1    X start of selection rectangle in tile coordinates
** @param sy1    Y start of selection rectangle in tile coordinates
**
** @return       the number of units found.
*/
int SelectUnitsInRectangle (int sx0, int sy0, int sx1, int sy1)
{
	Unit* unit;
	Unit* table[UnitMax];
	UnitType* type;
	int r;
	int n;
	int i;
	int tx0;
	int ty0;
	int tx1;
	int ty1;

	tx0 = sx0 / TileSizeX;
	ty0 = sy0 / TileSizeY;
	tx1 = sx1 / TileSizeX;
	ty1 = sy1 / TileSizeY;

	r = UnitCacheSelect(tx0 - 2, ty0 - 2, tx1 + 2 + 1, ty1 + 2 + 1, table);
	r = SelectSpritesInsideRectangle(sx0, sy0, sx1, sy1, table, r);

	//
	// 1) search for the player units selectable with rectangle
	//
	if ((n = SelectOrganicUnitsInTable(table, r))) {
		ChangeSelectedUnits(table, n);
		return n;
	}

	//
	// 2) If no unit found, try a player's unit not selectable by rectangle
	//
	for (i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player)) {
			continue;
		}
		// FIXME: Can we get this?
		if (!unit->Removed && unit->Orders[0].Action != UnitActionDie) {
			SelectSingleUnit(unit);
			return 1;
		}
	}

	//
	// 3) If no unit found, try a resource or a neutral critter
	//
	for (i = 0; i < r; ++i) {
		unit = table[i];
		// Unit visible FIXME: write function UnitSelectable
		if (!UnitVisibleInViewport(unit, TheUI.SelectedViewport)) {
			continue;
		}
		type = unit->Type;
		// Buildings are visible but not selectable
		if (type->Building && !UnitVisibleOnMap(unit, ThisPlayer)) {
			continue;
		}
		if ((type->GivesResource && !unit->Removed)) { // no built resources.
			SelectSingleUnit(unit);
			return 1;
		}
	}

	//
	// 4) If no unit found, select an enemy unit (first found)
	//
	for (i = 0; i < r; ++i) {
		unit = table[i];
		// Unit visible FIXME: write function UnitSelectable
		if (!UnitVisibleInViewport(unit, TheUI.SelectedViewport)) {
			continue;
		}
		// Buildings are visible but not selectable
		if (unit->Type->Building && !UnitVisibleOnMap(unit, ThisPlayer)) {
			continue;
		}
		if (!unit->Removed && unit->Orders[0].Action != UnitActionDie) {
			SelectSingleUnit(unit);
			return 1;
		}
	}

	return 0;
}

/**
** Select own ground units in a rectangle.
**
** @param sx0    X start of selection rectangle in tile coordinates
** @param sy0    Y start of selection rectangle in tile coordinates
** @param sx1    X start of selection rectangle in tile coordinates
** @param sy1    Y start of selection rectangle in tile coordinates
**
** @return       the number of units found.
*/
int SelectGroundUnitsInRectangle(int sx0, int sy0, int sx1, int sy1)
{
	Unit* unit;
	Unit* table[UnitMax];
	int r;
	int n;
	int i;
	int tx0;
	int ty0;
	int tx1;
	int ty1;

	tx0 = sx0 / TileSizeX;
	ty0 = sy0 / TileSizeY;
	tx1 = sx1 / TileSizeX;
	ty1 = sy1 / TileSizeY;

	r = UnitCacheSelect(tx0 - 2, ty0 - 2, tx1 + 2 + 1, ty1 + 2 + 1, table);
	r = SelectSpritesInsideRectangle(sx0, sy0, sx1, sy1, table, r);

	for (n = i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) || !unit->Type->SelectableByRectangle) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit->Type->UnitType == UnitTypeFly) {
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		table[n++] = unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n) {
		ChangeSelectedUnits(table, n);
	}
	return n;
}

/**
** Select own air units in a rectangle.
**
** @param sx0    X start of selection rectangle in tile coordinates
** @param sy0    Y start of selection rectangle in tile coordinates
** @param sx1    X start of selection rectangle in tile coordinates
** @param sy1    Y start of selection rectangle in tile coordinates
**
** @return       the number of units found.
*/
int SelectAirUnitsInRectangle(int sx0, int sy0, int sx1, int sy1)
{
	Unit* unit;
	Unit* table[UnitMax];
	int r;
	int n;
	int i;
	int tx0;
	int ty0;
	int tx1;
	int ty1;

	tx0 = sx0 / TileSizeX;
	ty0 = sy0 / TileSizeY;
	tx1 = sx1 / TileSizeX;
	ty1 = sy1 / TileSizeY;

	r = UnitCacheSelect(tx0 - 2, ty0 - 2, tx1 + 2 + 1, ty1 + 2 + 1, table);
	r = SelectSpritesInsideRectangle(sx0, sy0, sx1, sy1, table, r);

	for (n = i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) || !unit->Type->SelectableByRectangle) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit->Type->UnitType != UnitTypeFly) {
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		table[n++] = unit;
		if (n == MaxSelectable) {
			break;
		}
	}
	if (n) {
		ChangeSelectedUnits(table, n);
	}
	return n;
}

/**
** Add the ground units in the rectangle to the current selection
**
** @param sx0    X start of selection rectangle in tile coordinates
** @param sy0    Y start of selection rectangle in tile coordinates
** @param sx1    X start of selection rectangle in tile coordinates
** @param sy1    Y start of selection rectangle in tile coordinates
**
** @return       the number of units found.
*/
int AddSelectedGroundUnitsInRectangle(int sx0, int sy0, int sx1, int sy1)
{
	Unit* unit;
	Unit* table[UnitMax];
	int r;
	int n;
	int i;
	int tx0;
	int ty0;
	int tx1;
	int ty1;

	// If there is no selected unit yet, do a simple selection.
	if (!NumSelected) {
		return SelectGroundUnitsInRectangle(sx0, sy0, sx1, sy1);
	}

	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (NumSelected == 1 &&
			(!CanSelectMultipleUnits(Selected[0]->Player) ||
				!Selected[0]->Type->SelectableByRectangle)) {
		return NumSelected;
	}

	tx0 = sx0 / TileSizeX;
	ty0 = sy0 / TileSizeY;
	tx1 = sx1 / TileSizeX;
	ty1 = sy1 / TileSizeY;

	r = UnitCacheSelect(tx0 - 2, ty0 - 2, tx1 + 2 + 1, ty1 + 2 + 1, table);
	r = SelectSpritesInsideRectangle(sx0, sy0, sx1, sy1, table, r);

	for (n = i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) ||
			!unit->Type->SelectableByRectangle) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit->Type->UnitType == UnitTypeFly) {
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		table[n++] = unit;
		if (n == MaxSelectable) {
			break;
		}
	}

	//
	// Add the units to selected.
	//
	for (i = 0; i < n && NumSelected < MaxSelectable; ++i) {
		SelectUnit(table[i]);
	}
	return NumSelected;
}

/**
** Add the air units in the rectangle to the current selection
**
** @param sx0    X start of selection rectangle in tile coordinates
** @param sy0    Y start of selection rectangle in tile coordinates
** @param sx1    X start of selection rectangle in tile coordinates
** @param sy1    Y start of selection rectangle in tile coordinates
**
** @return       the number of units found.
*/
int AddSelectedAirUnitsInRectangle(int sx0, int sy0, int sx1, int sy1)
{
	Unit* unit;
	Unit* table[UnitMax];
	int r;
	int n;
	int i;
	int tx0;
	int ty0;
	int tx1;
	int ty1;

	// If there is no selected unit yet, do a simple selection.
	if (!NumSelected) {
		return SelectAirUnitsInRectangle(sx0, sy0, sx1, sy1);
	}

	// Check if the original selected unit (if it's alone) is ours,
	// and can be selectable by rectangle.
	// In this case, do nothing.
	if (NumSelected == 1 &&
			(!CanSelectMultipleUnits(Selected[0]->Player) ||
				!Selected[0]->Type->SelectableByRectangle)) {
		return NumSelected;
	}

	tx0 = sx0 / TileSizeX;
	ty0 = sy0 / TileSizeY;
	tx1 = sx1 / TileSizeX;
	ty1 = sy1 / TileSizeY;

	r = UnitCacheSelect(tx0 - 2, ty0 - 2, tx1 + 2 + 1, ty1 + 2 + 1, table);
	r = SelectSpritesInsideRectangle(sx0, sy0, sx1, sy1, table, r);

	for (n = i = 0; i < r; ++i) {
		unit = table[i];
		if (!CanSelectMultipleUnits(unit->Player) ||
			!unit->Type->SelectableByRectangle) {
			continue;
		}
		if (UnitUnusable(unit)) {  // guess SelectUnits doesn't check this
			continue;
		}
		if (unit->Type->UnitType != UnitTypeFly) {
			continue;
		}
		if (unit->TeamSelected) { // Somebody else onteam has this unit
			continue;
		}
		table[n++] = unit;
		if (n == MaxSelectable) {
			break;
		}
	}

	//
	// Add the units to selected.
	//
	for (i = 0; i < n && NumSelected < MaxSelectable; ++i) {
		SelectUnit(table[i]);
	}
	return NumSelected;
}

/**
**  Initialize the selection module.
*/
void InitSelections(void)
{
	int i;

	if (!Selected) {
		Selected = malloc(MaxSelectable * sizeof(Unit*));
	}

	for (i = 0; i < PlayerMax; ++i) {
		if (!TeamSelected[i]) {
			TeamSelected[i] = malloc(MaxSelectable * sizeof(Unit*));
		}
	}
}

/**
**  Save current selection state.
**
**  @param file  Output file.
*/
void SaveSelections(CLFile* file)
{
	int i;
	char* ref;

	CLprintf(file, "\n--- -----------------------------------------\n");
	CLprintf(file, "--- MODULE: selection $Id: selection.c,v 1.79 2004/06/25 17:39:05 jarod42 Exp $\n\n");

	CLprintf(file, "SetGroupId(%d)\n", GroupId);
	CLprintf(file, "Selection(%d, {", NumSelected);
	for (i = 0; i < NumSelected; ++i) {
		ref = UnitReference(Selected[i]);
		CLprintf(file, "\"%s\", ", ref);
		free(ref);
	}
	CLprintf(file, "})\n");
}

/**
**  Clean up the selection module.
*/
void CleanSelections(void)
{
	int i;

	GroupId = 0;
	NumSelected = 0;
	Assert(!NoUnitP); // Code fails if none zero
	free(Selected);
	Selected = NULL;

	for (i = 0; i < PlayerMax; ++i) {
		free(TeamSelected[i]);
		TeamSelected[i] = NULL;
		TeamNumSelected[i] = 0;
	}
}

// ----------------------------------------------------------------------------

/**
**  Set the current group id. (Needed for load/save)
**
**  @param l  Lua state.
*/
static int CclSetGroupId(lua_State* l)
{
	int old;

	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	old = GroupId;
	GroupId = LuaToNumber(l, 1);

	lua_pushnumber(l, old);
	return 1;
}

/**
**  Define the current selection.
**
**  @param l  Lua state.
*/
static int CclSelection(lua_State* l)
{
	int i;
	int args;
	int j;

	if (lua_gettop(l) != 2 || !lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	InitSelections();
	NumSelected = LuaToNumber(l, 1);
	i = 0;
	args = luaL_getn(l, 2);
	for (j = 0; j < args; ++j) {
		const char* str;

		lua_rawgeti(l, 2, j + 1);
		str = LuaToString(l, -1);
		lua_pop(l, 1);
		Selected[i++] = UnitSlots[strtol(str + 1, NULL, 16)];
	}

	return 0;
}

/**
**  Register CCL features for selections.
*/
void SelectionCclRegister(void)
{
	lua_register(Lua, "SetGroupId", CclSetGroupId);
	lua_register(Lua, "Selection", CclSelection);
}

//@}
