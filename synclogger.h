/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 */

#ifndef _INCL_SYNCLOGGER_H
#define _INCL_SYNCLOGGER_H

#include <windows.h>

class SyncLogger{
    private:
        HANDLE _hLogFile;
        HANDLE _hLoggerThread;
        DWORD _dwLoggerThreadId;

        static DWORD WINAPI SyncLoggerThread(SyncLogger *pthis);

    public:
        SyncLogger(const char *pszFilename);
        ~SyncLogger();
        void Log(const char *pszText);
};

#endif

