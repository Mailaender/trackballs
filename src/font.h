/* font.h
   Manage textured-font for drawing text

   Copyright (C) 2000  Mathias Broxvall
                       Yannick Perret

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef FONT_H
#define FONT_H

#include "general.h"

/* Return value is always the width of the rendered text */
int drawSimpleText(const char *text, int x, int y, int sz, Color c, int maxwidth = 0);
int drawRightSimpleText(const char *text, int x, int y, int sz, Color c, int maxwidth = 0);
int drawCenterSimpleText(const char *text, int x, int y, int sz, Color c, int maxwidth = 0);
int getTextWidth(const char *text, int sz, int maxwidth = 0);

#endif
