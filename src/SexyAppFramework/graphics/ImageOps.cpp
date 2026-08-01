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

#include "ImageOps.h"
#include "MemoryImage.h"
#include "GLImage.h"
#include "Graphics.h"
#include "Color.h"
#include "misc/Debug.h"
#include "Common.h" // uchar
#include <algorithm>
#include <cstring>

namespace Sexy
{

static void ColorizeBits(const uint32_t* theSrcBits, uint32_t* theDestBits, int theNumColors, const Color& theColor)
{
	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) &&
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < theNumColors; i++)
		{
			uint32_t aColor = theSrcBits[i];

			theDestBits[i] =
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < theNumColors; i++)
		{
			uint32_t aColor = theSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			aAlpha = std::min(aAlpha, 255);
			aRed = std::min(aRed, 255);
			aGreen = std::min(aGreen, 255);
			aBlue = std::min(aBlue, 255);

			theDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}
}

void ColorizeImage(Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == nullptr)
		return;

	uint32_t* aBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == nullptr)
	{
		aBits = aSrcMemoryImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;
		aNumColors = 256;
	}

	ColorizeBits(aBits, aBits, aNumColors, theColor);

	aSrcMemoryImage->BitsChanged();
}

void MirrorImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		uint32_t* aLeftBits = aSrcBits + (y * aPhysSrcWidth);
		uint32_t* aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			uint32_t aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void FlipImage(Image* theImage)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	uint32_t* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		uint32_t* aTopBits    = aSrcBits + x;
		uint32_t* aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			uint32_t aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void RotateImageHue(MemoryImage *theImage, int theDelta)
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	uint32_t *aPtr = theImage->GetBits();
	for (int i=0; i<aSize; i++)
	{
		uint32_t aPixel = *aPtr;
		int alpha = aPixel&0xff000000;
		int r = (aPixel>>16)&0xff;
		int g = (aPixel>>8) &0xff;
		int b = aPixel&0xff;

		int maxval = std::max(r, std::max(g, b));
		int minval = std::min(r, std::min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval+maxval)/2;
		int delta = maxval - minval;

		if (delta != 0)
		{
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));

			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v= (l < 128) ? (l * (255+s))/255 :
				(l+s-l*s/255);

		int y = static_cast<int>(2 * l - v);

		int aColorDiv = (6 * h) / 256;
		int x = static_cast<int>(y + (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
		x = std::min(x, 255);

		int z = static_cast<int>(v - (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
		z = std::max(z, 0);

		switch (aColorDiv)
		{
			case 0: r = static_cast<int>(v); g = x; b = y; break;
			case 1: r = z; g = static_cast<int>(v); b = y; break;
			case 2: r = y; g = static_cast<int>(v); b = x; break;
			case 3: r = y; g = z; b = static_cast<int>(v); break;
			case 4: r = x; g = y; b = static_cast<int>(v); break;
			case 5: r = static_cast<int>(v); g = y; b = z; break;
			default: r = static_cast<int>(v); g = x; b = y; break;
		}

		*aPtr++ = alpha | (r<<16) | (g << 8) | (b);

	}

	theImage->BitsChanged();
}

uint32_t HSLToRGB(int h, int s, int l)
{
	int r;
	int g;
	int b;

	double v= (l < 128) ? (l * (255+s))/255 :
			(l+s-l*s/255);

	int y = static_cast<int>(2 * l - v);

	int aColorDiv = (6 * h) / 256;
	int x = static_cast<int>(y + (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
	x = std::min(x, 255);

	int z = static_cast<int>(v - (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
	z = std::max(z, 0);

	switch (aColorDiv)
	{
		case 0: r = static_cast<int>(v); g = x; b = y; break;
		case 1: r = z; g = static_cast<int>(v); b = y; break;
		case 2: r = y; g = static_cast<int>(v); b = x; break;
		case 3: r = y; g = z; b = static_cast<int>(v); break;
		case 4: r = x; g = y; b = static_cast<int>(v); break;
		case 5: r = static_cast<int>(v); g = y; b = z; break;
		default: r = static_cast<int>(v); g = x; b = y; break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

uint32_t RGBToHSL(int r, int g, int b)
{
	int maxval = std::max(r, std::max(g, b));
	int minval = std::min(r, std::min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval+maxval)/2;
	int delta = maxval - minval;

	if (delta != 0)
	{
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));

		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);
}

void HSLToRGB(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void RGBToHSL(const uint32_t* theSource, uint32_t* theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		uint32_t src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}


GLImage* CreateCrossfadeImage(GLInterface* theGLInterface, Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor)
{
	MemoryImage* aMemoryImage1 = dynamic_cast<MemoryImage*>(theImage1);
	MemoryImage* aMemoryImage2 = dynamic_cast<MemoryImage*>(theImage2);

	if ((aMemoryImage1 == nullptr) || (aMemoryImage2 == nullptr))
		return nullptr;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) || 
		(theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect1 out of bounds");
		return nullptr;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) || 
		(theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect2 out of bounds");
		return nullptr;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	GLImage* anImage = new GLImage(theGLInterface);
	anImage->Create(aWidth, aHeight);

	uint32_t* aDestBits = anImage->GetBits();
	uint32_t* aSrcBits1 = aMemoryImage1->GetBits();
	uint32_t* aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	uint32_t aMult = static_cast<uint32_t>(theFadeFactor * 256);
	uint32_t aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		uint32_t* s1 = &aSrcBits1[(y+theRect1.mY)*aSrc1Width+theRect1.mX];
		uint32_t* s2 = &aSrcBits2[(y+theRect2.mY)*aSrc2Width+theRect2.mX];
		uint32_t* d = &aDestBits[y*aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			uint32_t p1 = *s1++;
			uint32_t p2 = *s2++;

			*d++ = 
				((((p1 & 0x000000FF)*aOMM + (p2 & 0x000000FF)*aMult)>>8) & 0x000000FF) |
				((((p1 & 0x0000FF00)*aOMM + (p2 & 0x0000FF00)*aMult)>>8) & 0x0000FF00) |
				((((p1 & 0x00FF0000)*aOMM + (p2 & 0x00FF0000)*aMult)>>8) & 0x00FF0000) |
				((((p1 >> 24)*aOMM + (p2 >> 24)*aMult)<<16) & 0xFF000000);
		}
	}

	anImage->BitsChanged();
	
	return anImage;
}

GLImage* CreateColorizedImage(GLInterface* theGLInterface, Image* theImage, const Color& theColor)
{
	MemoryImage* aSrcMemoryImage = dynamic_cast<MemoryImage*>(theImage);

	if (aSrcMemoryImage == nullptr)
		return nullptr;

	GLImage* anImage = new GLImage(theGLInterface);
	
	anImage->Create(theImage->GetWidth(), theImage->GetHeight());
	
	uint32_t* aSrcBits;
	uint32_t* aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == nullptr)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();				
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new uint32_t[256];
		aNumColors = 256;
		
		anImage->mColorIndices = new uchar[anImage->mWidth*theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth*theImage->mHeight);
	}
						
	ColorizeBits(aSrcBits, aDestBits, aNumColors, theColor);

	anImage->BitsChanged();

	return anImage;
}

GLImage* CopyImage(GLInterface* theGLInterface, Image* theImage, const Rect& theRect)
{
	GLImage* anImage = new GLImage(theGLInterface);

	anImage->Create(theRect.mWidth, theRect.mHeight);
	
	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

GLImage* CopyImage(GLInterface* theGLInterface, Image* theImage)
{
	return CopyImage(theGLInterface, theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

}
