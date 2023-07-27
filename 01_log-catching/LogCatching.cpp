/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 27.07.2023

  Copyright (C) 2023-now Authors and www.dsp-crowd.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <time.h>
#include <stdio.h>
#include <signal.h>

#include "LogCatching.h"
#include "LibFilesys.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StTmp) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

#define LOG_LVL	0

uint32_t LogCatching::numLines = 0;
string LogCatching::nameBase = "";

LogCatching *LogCatching::pLog = NULL;

LogCatching::LogCatching()
	: Processing("LogCatching")
	, mState(StStart)
	, mStartMs(0)
	, mFragmentLine("")
{}

/* member functions */

Success LogCatching::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	//Success success;
	char buf[5];
	size_t lenReq;
	ssize_t lenRead;
	char *pBuf, *pFound;
	bool ok;
#if 0
	procWrnLog("mState = %s", ProcStateString[mState]);
#endif
	switch (mState)
	{
	case StStart:

		pLog = this;

		//fileNonBlockingSet(STDIN_FILENO);
		signal(SIGUSR1, logSaveRequest);

		mState = StMain;

		break;
	case StMain:

		lenReq = sizeof(buf) - 1;

		lenRead = read(STDIN_FILENO, buf, lenReq);
#if 0
		if (!lenRead)
			break;

		if (lenRead < 0)
#else
		if (lenRead <= 0)
#endif
		{
#if 0
			if (errno == EAGAIN or errno == EWOULDBLOCK)
				break;

			procWrnLog("could not read line from stdin: %s (%d)",
							strerror(errno), errno);
#endif
			ok = logSave();
			if (!ok)
				return procErrLog(-1, "could not save log");

			return Positive;
		}

		buf[lenRead] = 0;
		pBuf = buf;

		while (true)
		{
			pFound = strchr(pBuf, '\n');
			if (!pFound)
			{
				mFragmentLine += pBuf;
				break;
			}

			*pFound++ = 0;
			mFragmentLine += pBuf;

			//procInfLog("line: '%s'", mFragmentLine.c_str());

			mLines.push_back(mFragmentLine);
			mFragmentLine = "";

			if (mLines.size() > numLines)
				mLines.pop_front();

			pBuf = pFound;
		}

		break;
	case StTmp:

		break;
	default:
		break;
	}

	return Pending;
}

void LogCatching::logSaveRequest(int signum)
{
	(void)signum;

	if (!pLog)
	{
		wrnLog("pLog not set");
		return;
	}

	bool ok;

	ok = pLog->logSave(true);
	if (!ok)
		wrnLog("could not save log");
}

bool LogCatching::logSave(bool triggeredByUser)
{
	time_t timeRaw;
	struct tm *infoTime;
	char buf[32];
	string prefixFile;
	string nameFile;

	time(&timeRaw);
	infoTime = localtime(&timeRaw);

	strftime(buf, sizeof(buf), "%y%m%d-%H%M%S_", infoTime);
	nameFile = string(buf) + nameBase;

	if (triggeredByUser)
		nameFile += "_usr";
	else
		nameFile += "_end";

	nameFile += ".log";

	//procWrnLog("Saving to file: %s", nameFile.c_str());

	ofstream fileLog(nameFile);

	if (!fileLog.good())
	{
		procErrLog(-1, "error opening file: %s (%d)", strerror(errno), errno);
		return false;
	}

	list<string>::iterator iter;

	iter = mLines.begin();
	for (; iter != mLines.end(); ++iter)
		fileLog << *iter << endl;

	fileLog.close();

	return true;
}

void LogCatching::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */
