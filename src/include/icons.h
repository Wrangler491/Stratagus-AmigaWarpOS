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
/**@name icons.h - The icons headerfile. */
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
//      $Id: icons.h,v 1.47 2004/06/26 19:10:06 jsalmon3 Exp $

#ifndef __ICONS_H__
#define __ICONS_H__

//@{

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @struct _icon_ icons.h
**
**  \#include "icons.h"
**
**  typedef struct _icon_ Icon;
**
**  This structure contains all informations about an icon.
**  Currently only rectangular static icons of 46x38 pixels are supported.
**  In the future it is planned to support animated and not rectangular
**  icons and icons of different sizes.
**
**  The icon structure members:
**
**  Icon::Ident
**
**    Unique identifier of the icon, used to reference it in config
**    files and during startup.  Don't use this in game, use instead
**    the pointer to this structure.
**
**  Icon::Tileset
**
**    Unique identifier of the tileset, used to allow different
**    graphics for the same icons depending on the tileset. Resolved
**    during startup in InitIcons().
**    @see Tileset::Ident
**
**  Icon::File
**
**    Pointer to icon file (file containing the graphics), each icon
**    could have an own icon file or some up to all icons could share
**    the same icon file.
**
**  Icon::Index
**
**    Index into the icon file. You know one up to all icons could
**    be in the same file. This index distinguishes them.
**
**  Icon::X
**
**    X pixel index into the graphic image.
**
**  Icon::Y
**
**    Y pixel index into the graphic image.
**
**  Icon::Width
**
**    Icon width in pixels.
**
**  Icon::Height
**
**    Icon height in pixels.
**
**  Icon::Graphic
**
**    Graphic image containing the loaded graphics. Loaded by
**    LoadIcons(). All icons belonging to the same icon file shares
**    this structure.
*/

/**
**  @struct _icon_config_ icons.h
**
**  \#include "icons.h"
**
**  typedef struct _icon_config_ IconConfig;
**
**  This structure contains all configuration informations about an icon.
**
**  IconConfig::Name
**
**    Unique identifier of the icon, used to reference icons in config
**    files and during startup.  The name is resolved during game
**    start and the pointer placed in the next field.
**    @see Icon::Ident
**
**  IconConfig::Icon
**
**    Pointer to an icon. This pointer is resolved during game start.
**
**    Example how this can be used in C initializers:
**
**    @code
**      { "icon-peasant" },
**    @endcode
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "video.h"
#include "iolib.h"

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define IconActive   1  ///< cursor on icon
#define IconClicked  2  ///< mouse button down on icon
#define IconSelected 4  ///< this the selected icon
#define IconAutoCast 8  ///< auto cast icon

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

/**
**  A structure describing an icon file, which could contain one or more
**  icons. @internal use only.
**
**  @todo
**    IconFile::Icons member isn't setup and used.
*/
typedef struct _icon_file_ {
	char* FileName;  ///< Icon file name/path
#if 0
	unsigned Width;   ///< Icon width
	unsigned Height;  ///< Icon height

	/** FIXME: unsed */
	unsigned Icons;  ///< Number of icons in this file
#endif

// --- FILLED UP ---
	Graphic* Sprite;  ///< Graphic data loaded
#ifdef USE_OPENGL
	Graphic* PlayerColorSprite[PlayerMax];  ///< Sprites with player colors
#endif
} IconFile;

	/// Icon: rectangle image used in menus
typedef struct _icon_ {
	char* Ident;    ///< Icon identifier
	char* Tileset;  ///< Tileset identifier

	IconFile* File;   ///< File containing the data
	unsigned  Index;  ///< Index into file

	int Width;   ///< Icon width
	int Height;  ///< Icon height

// --- FILLED UP ---
	Graphic* Sprite;  ///< Graphic data loaded
#ifdef USE_OPENGL
	Graphic** PlayerColorSprite;  ///< Sprites with player colors
#endif
} Icon;

#define NoIcon NULL  ///< used for errors == no valid icon

	/// Icon reference (used in config tables)
typedef struct _icon_config_ {
	char* Name;  ///< config icon name
	Icon* Icon;  ///< icon pointer to use to run time
} IconConfig;

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern char** IconWcNames;  ///< pud original -> internal

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void InitIcons(void);   ///< Init icons
extern void LoadIcons(void);   ///< Load icons
extern void CleanIcons(void);  ///< Cleanup icons

	/// Name -> icon
extern Icon* IconByIdent(const char* ident);
	/// Icon -> name
extern const char* IdentOfIcon(const Icon* icon);
	/// Draw icon
extern void DrawIcon(const Player*, Icon*, int, int);
	/// Draw icon of an unit
extern void DrawUnitIcon(const Player*, Icon*, unsigned, int, int);

	/// Register CCL features
extern void IconCclRegister(void);

//@}

#endif // !__ICONS_H__
