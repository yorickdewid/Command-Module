/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 */

#include "synclogger.h"

SyncLogger::SyncLogger(const char *pszFilename)
{
	_hLogFile = CreateFile(pszFilename, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if(_hLogFile){
		SetFilePointer(_hLogFile, 0, 0, FILE_END);
		_hLoggerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SyncLoggerThread, this, 0, &_dwLoggerThreadId);
	}
}

SyncLogger::~SyncLogger()
{
	if(_hLoggerThread){
		PostThreadMessage(_dwLoggerThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(_hLoggerThread, INFINITE);
		CloseHandle(_hLoggerThread);
	}
	if(_hLogFile){
		CloseHandle(_hLogFile);
	}
}

void SyncLogger::Log(const char *pszText)
{
	char *psz;
	DWORD dwDateLen, dwTimeLen;

	if(_hLogFile && _dwLoggerThreadId && pszText){
		dwDateLen = GetDateFormat(LOCALE_SYSTEM_DEFAULT, NULL, 0, "dd-MM-yyyy", 0, 0);
		dwTimeLen = GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, 0, "HH':'mm':'ss", 0, 0);
		size_t buflen = dwDateLen+dwTimeLen+strlen(pszText)+5;
		psz = new char[buflen];
		psz[0] = '[';
		GetDateFormat(LOCALE_SYSTEM_DEFAULT, NULL, 0, "dd-MM-yyyy", psz+1, dwDateLen);
		psz[dwDateLen] = ' ';
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, 0, "HH':'mm':'ss" , psz+dwDateLen+1, dwTimeLen);
		strcat(psz, "] ");
		strcat(psz, pszText);
		strcat(psz, "\r\n");
		while(!PostThreadMessage(_dwLoggerThreadId, WM_USER, 0, (LPARAM)psz)){
			Sleep(0);
		}
	}
}

DWORD WINAPI SyncLogger::SyncLoggerThread(SyncLogger *pthis)
{
	MSG msg;
	DWORD dw;

	PeekMessage(&msg,0,0,0,PM_NOREMOVE);
	while(GetMessage(&msg,0,0,0)){
		switch(msg.message){
		case WM_USER:
			WriteFile(pthis->_hLogFile, (char *)msg.lParam, (DWORD)strlen((char *)msg.lParam), &dw, 0);
			delete(char *)msg.lParam;
		}
	}

	return 0;
}
