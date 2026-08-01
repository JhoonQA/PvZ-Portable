/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 * 
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __IMAGEOPS_H__
#define __IMAGEOPS_H__

#include <cstdint>
#include "misc/Rect.h"

namespace Sexy
{

class Image;
class GLImage;
class GLInterface;
class MemoryImage;
class Color;

void					ColorizeImage(Image* theImage, const Color& theColor);
void					MirrorImage(Image* theImage);
void					FlipImage(Image* theImage);
void					RotateImageHue(MemoryImage* theImage, int theDelta);
uint32_t				HSLToRGB(int h, int s, int l);
uint32_t				RGBToHSL(int r, int g, int b);
void					HSLToRGB(const uint32_t* theSource, uint32_t* theDest, int theSize);
void					RGBToHSL(const uint32_t* theSource, uint32_t* theDest, int theSize);
GLImage*				CreateCrossfadeImage(GLInterface* theGLInterface, Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor);
GLImage*				CreateColorizedImage(GLInterface* theGLInterface, Image* theImage, const Color& theColor);
GLImage*				CopyImage(GLInterface* theGLInterface, Image* theImage, const Rect& theRect);
GLImage*				CopyImage(GLInterface* theGLInterface, Image* theImage);

}

#endif
