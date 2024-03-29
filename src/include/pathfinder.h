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
/**@name pathfinder.h - The path finder headerfile. */
//
//      (c) Copyright 1998-2004 by Lutz Sammer, Russell Smith
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
//      $Id: pathfinder.h,v 1.41 2004/04/06 21:50:02 jarod42 Exp $

#ifndef __PATH_FINDER_H__
#define __PATH_FINDER_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "unit.h"
#include "map.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/**
**  Result codes of the pathfinder.
**
**  @todo
**    Another idea is SINT_MAX as reached, SINT_MIN as unreachable
**    stop others how far to goal.
*/
enum _move_return_ {
	PF_FAILED=-3,       ///< This Pathfinder failed, try another
	PF_UNREACHABLE=-2,  ///< Unreachable stop
	PF_REACHED=-1,      ///< Reached goal stop
	PF_WAIT=0,          ///< Wait, no time or blocked
	PF_MOVE=1,          ///< On the way moving
};

/**
**  To remove pathfinder internals. Called if path destination changed.
*/
#define NewResetPath(unit) \
	do { unit->Data.Move.Fast=1; unit->Data.Move.Length=0; }while( 0 )

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

	/// Path matrix
extern unsigned char pMatrix[(MaxMapWidth + 2) * (MaxMapHeight + 3) + 2];
	/// cost associated to move on a tile occupied by a fixed unit
extern int AStarFixedUnitCrossingCost;
	/// cost associated to move on a tile occupied by a moving unit
extern int AStarMovingUnitCrossingCost;
	/// Whether to have knowledge of terrain that we haven't visited yet
extern int AStarKnowUnknown;
	/// Cost of using a square we haven't seen before.
extern int AStarUnknownTerrainCost;

//
//  Convert heading into direction.
//  N NE  E SE  S SW  W NW
extern const int Heading2X[9];
extern const int Heading2Y[9];
extern const int XY2Heading[3][3];

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

	/// Create a matrix for the old pathfinder
extern unsigned char* CreateMatrix(void);
	/// Allocate a new matrix and initialize
extern unsigned char* MakeMatrix(void);
	/// Get next element of the way to goal.
extern int NewPath(Unit* unit);
	/// Return distance to unit.
extern int UnitReachable(Unit* unit, Unit* dst, int range);

extern int PlaceReachable(Unit* src, int x, int y, int w, int h, int minrange, int maxrange);

//
// in astar.c
//
	/// Returns the next element of the path
extern int NextPathElement(Unit*, int* xdp, int* ydp);

	/// Init the a* data structures
extern void InitAStar(void);

	/// free the a* data structures
extern void FreeAStar(void);

	/// Find and a* path for a unit
extern int AStarFindPath(Unit* unit, int gx, int gy, int gw, int gh, int minrange, int maxrange, char* path);
//
// in ccl_pathfinder.c
//
	/// register ccl features
extern void PathfinderCclRegister(void);

//@}

#ifdef MAP_REGIONS
#include "../pathfinder/splitter.h"
#endif /* MAP_REGIONS */

#endif // !__PATH_FINDER_H__
