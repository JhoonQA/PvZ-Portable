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

#include "TextWidget.h"
#include "graphics/Graphics.h"
#include "ScrollbarWidget.h"
#include "WidgetManager.h"
#include "graphics/Font.h"

using namespace Sexy;

std::vector<std::string> TextWidget::GetLines()
{
	return mLogicalLines;
}

void TextWidget::SetLines(std::vector<std::string> theNewLines)
{
	mLogicalLines = theNewLines;
}

void TextWidget::Clear()
{
	mLogicalLines.clear();	
	mPhysicalLines.clear();
	mPosition = 0.0;
	mScrollbar->SetMaxValue(0.0);
	MarkDirty();
}

void TextWidget::DrawColorString(Graphics* g, std::string_view theString, int x, int y, bool useColors)
{		
	int aWidth = 0;
	
	if (useColors)
		g->SetColor(Color(0, 0, 0));				
	
	std::string aCurString = "";
	for (int i = 0; i < (int)theString.length(); i++)
	{
		// color marker: 0xFF followed by 3 RGB bytes
		if (theString[i] == (char)0xFF)
		{
			if (aCurString.length() > 0)
				g->DrawString(aCurString, x + aWidth, y);
			
			aWidth += g->GetFont()->StringWidth(aCurString);
			aCurString = "";
			if (useColors)
				g->SetColor(Color((uint8_t)theString[i+1], (uint8_t)theString[i+2], (uint8_t)theString[i+3]));
			i += 3;
		}
		else
			aCurString += theString[i];
	}				
	
	if (aCurString.length() > 0)
		g->DrawString(aCurString, x + aWidth, y);
}

void TextWidget::DrawColorStringHilited(Graphics* g, std::string_view theString, int x, int y, int theStartPos, int theEndPos)
{
	DrawColorString(g, theString, x, y, true);				
	
	if (theEndPos > theStartPos)
	{
		int aXOfs = GetColorStringWidth(theString.substr(0, theStartPos));
		int aWidth = GetColorStringWidth(theString.substr(0, theEndPos)) - aXOfs;
	
		Graphics aClipG(*g);
		aClipG.ClipRect(x + aXOfs, y - g->GetFont()->GetAscent(), aWidth, g->GetFont()->GetHeight());
	
		g->SetColor(Color(0, 0, 128));
		g->FillRect(x + aXOfs, y - g->GetFont()->GetAscent(), aWidth, g->GetFont()->GetHeight());
	
		aClipG.SetColor(Color(255, 255, 255));
		DrawColorString(&aClipG, theString, x, y, false);
	}
}

int TextWidget::GetStringIndex(std::string_view theString, int thePixel)
{
	// Walk whole code points so a selection never splits a UTF-8 sequence.
	int aPos = 0;
	size_t aOffset = 0;
	while (aOffset < theString.length())
	{
		size_t aNext = aOffset;
		if (theString[aOffset] == (char)0xFF)
		{
			aNext = aOffset + 4; // color marker, zero width
		}
		else
		{
			char32_t aChar;
			if (!UTF8DecodeNext(theString, aNext, aChar))
				break;
		}

		int aLoLen = GetColorStringWidth(theString.substr(0, aOffset));
		int aHiLen = GetColorStringWidth(theString.substr(0, aNext));
		if (thePixel >= (aLoLen + aHiLen) / 2)
			aPos = (int)aNext;
		aOffset = aNext;
	}

	return aPos;
}

int TextWidget::GetColorStringWidth(std::string_view theString)
{				
	int aWidth = 0;	
	std::string aTempString;
					
	for (int i = 0; i < (int)theString.length(); i++)
	{
		if (theString[i] == (char)0xFF)
		{
			aWidth += mFont->StringWidth(aTempString);
			aTempString = "";

			i += 3;
		}
		else
			aTempString += theString[i];
	}
	
	aWidth += mFont->StringWidth(aTempString);
	
	return aWidth;
}

void TextWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);				
		
	double aPageSize = 1;
	if (mHeight > mFont->GetHeight()+16)
		aPageSize = (mHeight - 8.0) / mFont->GetHeight();
	
	int aLogValue = 0;		
	if (mLineMap.size() > 0)
	{
		//IntIntMap::iterator anItr = mLineMap.find(mScrollbar->mValue);

		aLogValue = mLineMap[(int) mScrollbar->mValue];
	}

	int aNewPhysValue = 0;
	
	mLineMap.clear();
	mPhysicalLines.clear();
	for (int i = 0; i < (int)mLogicalLines.size(); i++)
	{
		if (i == aLogValue)
			aNewPhysValue = mPhysicalLines.size();
		
		AddToPhysicalLines(i, mLogicalLines[i]);
	}
	
	bool atBottom = mScrollbar->AtBottom();
	
	mPageSize = aPageSize;
	mScrollbar->SetMaxValue(mPhysicalLines.size());
	mScrollbar->SetPageSize((int) aPageSize);
	mScrollbar->SetValue(aNewPhysValue);
	
	if ((mStickToBottom) && (atBottom))			
		mScrollbar->GoToBottom();		
}

Color TextWidget::GetLastColor(std::string_view theString)
{
	size_t anIdx = theString.rfind((char)0xFF);
	if (anIdx == std::string_view::npos)
		return Color(0, 0, 0);
	
	return Color((uint8_t)theString[anIdx+1], (uint8_t)theString[anIdx+2], (uint8_t)theString[anIdx+3]);
}

// Where to wrap theText at theMaxWidth pixels: after the last fitting space, else past the last fitting code point.
static size_t FindTextWrapPos(_Font* theFont, std::string_view theText, int theMaxWidth)
{
	int aWidth = 0;
	size_t aFitEnd = 0;
	size_t aSpaceEnd = std::string_view::npos;
	size_t aOffset = 0;
	while (aOffset < theText.length())
	{
		size_t aNext = aOffset;
		if (theText[aOffset] == (char)0xFF)
		{
			aNext = aOffset + 4; // color marker
		}
		else
		{
			char32_t aChar;
			if (!UTF8DecodeNext(theText, aNext, aChar))
				break;
			aWidth += theFont->CharWidth(aChar);
			if (aWidth > theMaxWidth)
				break;
			aFitEnd = aNext;
			if (aChar == U' ')
				aSpaceEnd = aNext;
		}
		aOffset = aNext;
	}

	if (aOffset == theText.length()) // everything fit
		return theText.length();
	if (aSpaceEnd != std::string_view::npos)
		return aSpaceEnd;
	if (aFitEnd == 0)
	{
		char32_t aChar = 0;
		UTF8DecodeNext(theText, aFitEnd, aChar); // break after the first code point to make progress
	}
	return aFitEnd;
}

void TextWidget::AddToPhysicalLines(int theIdx, const std::string& theLine)
{		
	std::string aCurString;
		
	if (GetColorStringWidth(theLine) <= mWidth - 8)
	{
		aCurString = theLine;
	}
	else
	{
		size_t aCurPos = 0;
		while (aCurPos < theLine.length())
		{
			while ((aCurPos < theLine.length()) && (theLine[aCurPos] == ' '))
				aCurPos++; // drop spaces at the start of a continuation line

			size_t aBreakPos = FindTextWrapPos(mFont, std::string_view(theLine).substr(aCurPos), mWidth - 8) + aCurPos;
			std::string aPiece(theLine.substr(aCurPos, aBreakPos - aCurPos));
			if (aPiece.empty())
				break;

			aCurString += aPiece;
			if (aBreakPos < theLine.length())
			{
				Color aColor = GetLastColor(aCurString);
				mPhysicalLines.push_back(aCurString);
				mLineMap.push_back(theIdx);
				aCurString = std::string("\xFF", 1) + (char)aColor.mRed + (char)aColor.mGreen + (char)aColor.mBlue;
			}
			aCurPos = aBreakPos;
		}
	}	
	
	if (!aCurString.empty() || theLine.empty())
	{
		mPhysicalLines.push_back(aCurString);
		mLineMap.push_back(theIdx);
	}
}

void TextWidget::AddLine(const std::string& theLine)
{
	std::string aLine = theLine;

	if (aLine.empty())
		aLine = " ";
	
	bool atBottom = mScrollbar->AtBottom();
	
	mLogicalLines.push_back(aLine);
	
	if ((int)mLogicalLines.size() > mMaxLines)
	{
		// Remove an extra 10 lines, for safety
		int aNumLinesToRemove = mLogicalLines.size() - mMaxLines + 10;
				
		mLogicalLines.erase(mLogicalLines.begin(), mLogicalLines.begin() + aNumLinesToRemove);
		
		int aPhysLineRemoveCount = 0;

		// Remove all physical lines and linemap entries relating to deleted logical lines
		while (mLineMap[aPhysLineRemoveCount] < aNumLinesToRemove)
		{
			aPhysLineRemoveCount++;			
		}

		mLineMap.erase(mLineMap.begin(), mLineMap.begin() + aPhysLineRemoveCount);
		mPhysicalLines.erase(mPhysicalLines.begin(), mPhysicalLines.begin() + aPhysLineRemoveCount);
		
		// Offset the line map numbers
		for (int i = 0; i < (int)mLineMap.size(); i++)
		{
			mLineMap[i] -= aNumLinesToRemove;
		}
		
		// Move the hilited area
		for (int i = 0; i < 2; i++)
		{
			mHiliteArea[i][1] -= aNumLinesToRemove;
			if (mHiliteArea[i][1] < 0)
			{
				mHiliteArea[i][0] = 0;
				mHiliteArea[i][1] = 0;
			}
		}
		
		mScrollbar->SetValue(mScrollbar->mValue - aNumLinesToRemove);
	}
	
	AddToPhysicalLines(mLogicalLines.size()-1, aLine);
	
	mScrollbar->SetMaxValue(mPhysicalLines.size());
	
	if (atBottom)
		mScrollbar->GoToBottom();
			
	MarkDirty();
}

bool TextWidget::SelectionReversed()
{
	return ((mHiliteArea[1][1] < mHiliteArea[0][1]) ||
			((mHiliteArea[1][1] == mHiliteArea[0][1]) && 
			    (mHiliteArea[1][0] < mHiliteArea[0][0])));
}

void TextWidget::GetSelectedIndices(int theLineIdx, int* theIndices)
{
	int aXor = SelectionReversed() ? 1 : 0;
	for (int aPosIdx = 0; aPosIdx < 2; aPosIdx++)
	{	
		int aVal;
		
		if (mHiliteArea[aPosIdx][1] < theLineIdx)
			aVal = 0;
		else if (mHiliteArea[aPosIdx][1] == theLineIdx)
			aVal = mHiliteArea[aPosIdx][0];
		else 
			aVal = mPhysicalLines[theLineIdx].length();
					
		theIndices[aPosIdx ^ aXor] = aVal;			
	}			
}

void TextWidget::Draw(Graphics* g)
{
	g->SetColor(Color(255, 255, 255));
	g->FillRect(0, 0, mWidth, mHeight);
	
	Graphics aClipG(*g);
	aClipG.ClipRect(4, 4, mWidth - 8, mHeight - 8);
	
	aClipG.SetColor(Color(0, 0, 0));
	aClipG.SetFont(mFont);		
	
	int aFirstLine = (int) mPosition;
	int aLastLine = std::min((int) mPhysicalLines.size()-1, (int) mPosition + (int) mPageSize + 1);
	
	for (int i = aFirstLine; i <= aLastLine; i++)
	{
		int aYPos = 4 + (int) ((i - (int) mPosition)*mFont->GetHeight()) + mFont->GetAscent();
		std::string aString = mPhysicalLines[i];
		
		int aHilitePos[2];
		GetSelectedIndices(i, aHilitePos);
		DrawColorStringHilited(&aClipG, aString, 4, aYPos, aHilitePos[0], aHilitePos[1]);
	}				
}

void TextWidget::ScrollPosition(int theId, double thePosition)
{
	(void)theId;
	mPosition = thePosition;
	MarkDirty();
}

void TextWidget::GetTextIndexAt(int x, int y, int* thePosArray)
{
	int aLineNum = (int) (mScrollbar->mValue + (y / (double) mFont->GetHeight()));
	if (y < 0)
	{
		thePosArray[0] = 0;
		thePosArray[1] = 0;
	}
	else if (aLineNum < (int)mPhysicalLines.size())
	{
		thePosArray[0] = GetStringIndex(mPhysicalLines[aLineNum], x);
		thePosArray[1] = aLineNum;						
	}
	else
	{
		if (mPhysicalLines.size() > 0)
		{		
			thePosArray[0] = mPhysicalLines[mPhysicalLines.size()-1].length();
			thePosArray[1] = mPhysicalLines.size() - 1;				
		}
	}
}

void TextWidget::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);

	GetTextIndexAt(x-4, y-4, mHiliteArea[0]);
	mHiliteArea[1][0] = mHiliteArea[0][0];
	mHiliteArea[1][1] = mHiliteArea[0][1];
	MarkDirty();
}

void TextWidget::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);

	GetTextIndexAt(x-4, y-4, mHiliteArea[1]);
	MarkDirty();
}

std::string TextWidget::GetSelection()
{
	std::string aSelString = "";
	int aSelIndices[2];	
	bool first = true;
	
	bool reverse = SelectionReversed();
	for (int aLineNum = mHiliteArea[reverse ? 1 : 0][1]; aLineNum <= mHiliteArea[reverse ? 0 : 1][1]; aLineNum++)
	{
		std::string aString = mPhysicalLines[aLineNum];
		
		GetSelectedIndices(aLineNum, aSelIndices);
		
		if (!first)
			aSelString += "\r\n";
		
		for (int aStrIdx = aSelIndices[0]; aStrIdx < aSelIndices[1]; aStrIdx++)
		{
			char aChar = aString[aStrIdx];
			if (aChar != (char)0xFF)
				aSelString += aChar;
			else
				aStrIdx += 3;
		}
		
		first = false;
	}
	
	return aSelString;
}

void TextWidget::KeyDown(KeyCode){}
