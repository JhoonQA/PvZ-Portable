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

#include "ModVal.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <vector>

struct ModStorage
{
	bool					mChanged = false;
	int						mInt = 0;
	double					mDouble = 0.0;
	std::string				mString;
};

// One registered M() call site.
struct ModCallSite
{
	ModStorage				mStorage;
	std::string				mFileName;	// source file the call site lives in
	int						mCounter = -1;	// __COUNTER__ value baked into the key
	int						mLineNum = -1;
};

// Keyed by literal address: each M() site has a unique literal, so pooling only merges the same logical site.
using CallSiteMap = std::unordered_map<const char*, ModCallSite>;

// Source file path -> the call sites registered in that file
using FileSitesMap = std::unordered_map<std::string, std::vector<ModCallSite*>>;

using StringToIntMap = std::unordered_map<std::string, int>;

static CallSiteMap& GetCallSiteMap()
{
	static CallSiteMap aMap;
	return aMap;
}

static FileSitesMap& GetFileSitesMap()
{
	static FileSitesMap aMap;
	return aMap;
}

static StringToIntMap& GetStringToIntMap()
{
	static StringToIntMap aMap;
	return aMap;
}

// Strips <counter>","<line> off a call site key, leaving the plain file name.
static bool ParseModValString(std::string &theStr, int *theCounter = nullptr, int *theLineNum = nullptr)
{
	size_t aPos = theStr.length()-1;
	bool foundComma = false;
	while (aPos>0)
	{
		if (!foundComma && theStr[aPos]==',')
		{
			aPos--;
			foundComma = true;
		}
		else if (isdigit(static_cast<unsigned char>(theStr[aPos])))
			aPos--;
		else
			break;
	}

	if (aPos==theStr.length()-1 || aPos==0) // no number,number to erase... or empty file
		return false;

	aPos++;
	int aCounterVal = -1, aLineNum = -1;
	if (sscanf(theStr.c_str()+aPos,"%d,%d",&aCounterVal,&aLineNum)!=2) // couldn't parse out the numbers
		return false;

	theStr.resize(aPos);
	if (theCounter) *theCounter = aCounterVal;
	if (theLineNum) *theLineNum = aLineNum;
	return true;
}

static ModCallSite* FindCallSite(const char* theKey)
{
	CallSiteMap &aSites = GetCallSiteMap();
	CallSiteMap::iterator anItr = aSites.find(theKey);
	if (anItr != aSites.end())
		return &anItr->second;

	// First execution of this call site: register it for ReparseModValues().
	std::string aFileName = theKey+15; // skip SEXY_SEXYMODVAL
	int aCounter = -1, aLineNum = -1;
	if (!ParseModValString(aFileName, &aCounter, &aLineNum))
		return nullptr;

	ModCallSite &aSite = aSites[theKey];
	aSite.mFileName = aFileName;
	aSite.mCounter = aCounter;
	aSite.mLineNum = aLineNum;
	GetFileSitesMap()[aFileName].push_back(&aSite); // node-based map: pointer stays valid
	return &aSite;
}

int Sexy::ModVal(const char* theFileName, int theInt)
{
	ModCallSite *aSite = FindCallSite(theFileName);
	if (aSite != nullptr && aSite->mStorage.mChanged)
		return aSite->mStorage.mInt;
	else
		return theInt;
}

double Sexy::ModVal(const char* theFileName, double theDouble)
{
	ModCallSite *aSite = FindCallSite(theFileName);
	if (aSite != nullptr && aSite->mStorage.mChanged)
		return aSite->mStorage.mDouble;
	else
		return theDouble;
}

float Sexy::ModVal(const char* theFileName, float theFloat)
{
	return (float) ModVal(theFileName, (double) theFloat);
}

const char*	Sexy::ModVal(const char* theFileName, const char *theStr)
{
	ModCallSite *aSite = FindCallSite(theFileName);
	if (aSite != nullptr && aSite->mStorage.mChanged)
		return aSite->mStorage.mString.c_str();
	else
		return theStr;
}


void Sexy::AddModValEnum(const std::string &theEnumName, int theVal)
{
	GetStringToIntMap()[theEnumName] = theVal;
}

static bool ModStringToInteger(const char* theString, int* theIntVal)
{
	*theIntVal = 0;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;

	if (isalpha((unsigned char)theString[i]) || theString[i]=='_') // enum
	{
		
		std::string aStr;
		while (isalnum((unsigned char)theString[i]) || theString[i]=='_')
		{
			aStr += theString[i];
			i++;
		}

		StringToIntMap &aStringToIntMap = GetStringToIntMap();
		StringToIntMap::iterator anItr = aStringToIntMap.find(aStr);
		if (anItr!=aStringToIntMap.end())
		{
			*theIntVal = anItr->second;
			return true;
		}

		i = 0;
	}
		
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (;;)
	{
		char aChar = theString[i];
		
		if ((theRadix == 10) && (aChar >= '0') && (aChar <= '9'))
			*theIntVal = (*theIntVal * 10) + (aChar - '0');
		else if ((theRadix == 0x10) && 
			(((aChar >= '0') && (aChar <= '9')) || 
			 ((aChar >= 'A') && (aChar <= 'F')) || 
			 ((aChar >= 'a') && (aChar <= 'f'))))
		{			
			if (aChar <= '9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - '0');
			else if (aChar <= 'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'a') + 0x0A;
		}
		else if (((aChar == 'x') || (aChar == 'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else if (aChar == ')')
		{
			if (isNeg)
				*theIntVal = -*theIntVal;
			return true;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}

		i++;
	}		
}

static bool ModStringToDouble(const char* theString, double* theDoubleVal)
{
	*theDoubleVal = 0.0;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (;;)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - '0');
		else if (aChar == '.')
		{
			i++;
			break;
		}		
		else if ((aChar == ')') || ((aChar == 'f') && (theString[i+1] == ')'))) // At end
		{
			if (isNeg)
				*theDoubleVal = -*theDoubleVal;
			return true;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}

		i++;
	}

	double aMult = 0.1;
	for (;;)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
		{
			*theDoubleVal += (aChar - '0') * aMult;	
			aMult /= 10.0;
		}
		else if ((aChar == ')') || ((aChar == 'f') && (theString[i+1] == ')'))) // At end
		{
			if (isNeg)
				*theDoubleVal = -*theDoubleVal;
			return true;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}

		i++;
	}
}

static bool ModStringToString(const char* theString, std::string &theStrVal)
{
	if (theString[0]!='"')
		return false;

	std::string &aStr = theStrVal;
	aStr.erase();

	int i=1;
	while (true)
	{
		if (theString[i]=='\\')
		{
			i++;
			switch (theString[i++])
			{
			case 'n': aStr += '\n'; break;
			case 't': aStr += '\t'; break;
			case '\\': aStr += '\\'; break;
			case '"': aStr += '\"'; break;
			default: return false;
			}
		}
		else if (theString[i]=='"')
		{
			i++;
			while (isspace((unsigned char)theString[i]))
				i++;

			if (theString[i]!='"') // continued string
				return true;
			else
				break;
		}
		else if (theString[i]=='\0')
			return false;
		else
			aStr += theString[i++];
	}

	return true;
}

// Parses an M() value (just past '(') into the site's storage; returns false
// and keeps the old value when the text is not a plain literal.
static bool ApplyModValue(const char* theText, ModStorage *theStorage)
{
	while (isspace((unsigned char)*theText))
		theText++;

	if (*theText == '"')
	{
		if (!ModStringToString(theText, theStorage->mString))
			return false;
	}
	else
	{
		bool aParsed = false;
		int anIntVal;
		if (ModStringToInteger(theText, &anIntVal))
		{
			theStorage->mInt = anIntVal;
			aParsed = true;
		}
		double aDoubleVal;
		if (ModStringToDouble(theText, &aDoubleVal))
		{
			theStorage->mDouble = aDoubleVal;
			aParsed = true;
		}
		if (!aParsed)
			return false;
	}

	theStorage->mChanged = true;
	return true;
}

struct ModToken
{
	int mLineNum;
	size_t mValuePos; // offset just past the '(' of the M( token
};

// Finds M(, M1(, ... M9( tokens at identifier boundaries, in source order.
static std::vector<ModToken> FindModTokens(const std::string &theContents)
{
	std::vector<ModToken> aTokens;
	int aLineNum = 1;
	for (size_t i = 0; i < theContents.size(); i++)
	{
		char aChar = theContents[i];
		if (aChar == '\n')
		{
			aLineNum++;
		}
		else if (aChar == 'M' && (i == 0 || (!isalnum((unsigned char)theContents[i-1]) && theContents[i-1] != '_')))
		{
			size_t aPos = i + 1;
			if (aPos < theContents.size() && isdigit((unsigned char)theContents[aPos]))
				aPos++; // M1 .. M9
			if (aPos < theContents.size() && theContents[aPos] == '(')
				aTokens.push_back({aLineNum, aPos + 1});
		}
	}
	return aTokens;
}

bool Sexy::ReparseModValues()
{
	bool aUpdated = false;

	for (auto &aFilePair : GetFileSitesMap())
	{
		const std::string &aFileName = aFilePair.first;
		std::ifstream aStream(aFileName, std::ios::binary);
		if (!aStream)
			continue;
		const std::string aContents((std::istreambuf_iterator<char>(aStream)), std::istreambuf_iterator<char>());

		const std::vector<ModToken> aTokens = FindModTokens(aContents);

		std::unordered_map<int, std::vector<size_t>> aTokensByLine;
		for (size_t i = 0; i < aTokens.size(); i++)
			aTokensByLine[aTokens[i].mLineNum].push_back(aTokens[i].mValuePos);

		std::unordered_map<int, std::vector<ModCallSite*>> aSitesByLine;
		for (ModCallSite *aSite : aFilePair.second)
			aSitesByLine[aSite->mLineNum].push_back(aSite);

		// __COUNTER__ increases in source order within a line, so sorted sites pair with tokens in scan order.
		for (auto &aLinePair : aSitesByLine)
		{
			std::unordered_map<int, std::vector<size_t>>::iterator aTokItr = aTokensByLine.find(aLinePair.first);
			if (aTokItr == aTokensByLine.end())
				continue;

			std::vector<ModCallSite*> &aLineSites = aLinePair.second;
			std::sort(aLineSites.begin(), aLineSites.end(),
				[](ModCallSite *a, ModCallSite *b) { return a->mCounter < b->mCounter; });

			const std::vector<size_t> &aPositions = aTokItr->second;
			for (size_t i = 0; i < aLineSites.size() && i < aPositions.size(); i++)
				if (ApplyModValue(aContents.c_str() + aPositions[i], &aLineSites[i]->mStorage))
					aUpdated = true;
		}
	}

	return aUpdated;
}
