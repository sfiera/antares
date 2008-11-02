/*
Ares, a tactical space combat game.
Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* Sprite Cursor.h */
#ifndef kSpriteHandling
#include "Sprite Handling.h"
#endif

#pragma options align=mac68k

#define	kSpriteCursorHidden		0	// if showLevel <= this, cursor is hidden
#define	kSpriteCursorVisible	1	// if showLevel <= this, cursor is hidden

typedef struct
{
	Point		where;
	Point		lastWhere;
	Rect		thisRect;
	Rect		lastRect;
	short		showLevel;
	short		lastShowLevel;
	spriteType	sprite;
	Boolean		thisShowLine;
	Boolean		lastShowLine;
	Point		thisLineStart;
	Point		thisLineEnd;
	Point		lastLineStart;
	Point		lastLineEnd;
	unsigned char	thisLineColor;
	unsigned char	thisLineColorDark;
} spriteCursorType;

short InitSpriteCursor( void);
void CleanupSpriteCursor( void);
void ResetSpriteCursor( void);
void ShowSpriteCursor( Boolean);
void HideSpriteCursor( Boolean);
void ShowHideSpriteCursor( Boolean);
Boolean SpriteCursorVisible( void);
Boolean SetSpriteCursorTable( short);
void SetSpriteCursorShape( short);
void EraseSpriteCursorSprite( void);
void DrawSpriteCursorSprite( longRect *);
void ShowSpriteCursorSprite( void);
void MoveSpriteCursor( Point);
void ShowHintLine( Point fromWhere, Point toWhere,
	unsigned char color, unsigned char brightness);
void HideHintLine( void);
void ResetHintLine( void);

#pragma options align=reset
