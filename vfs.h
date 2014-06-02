/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 */

#ifndef _INCL_VFS_H
#define _INCL_VFS_H

#include <windows.h>
#include <map>
#include <string>
#include "tree.h"

using namespace std;

class VFS{
    private:
        struct MOUNTPOINT{
            string strVirtual;
            string strLocal;
        };
        struct FINDDATA{
            string strVirtual;
            string strFilespec;
            HANDLE hFind;
            tree<MOUNTPOINT> *ptree;
        };

        tree<MOUNTPOINT> _root;

        static DWORD Map(const char *pszVirtual, string &strLocal, tree<MOUNTPOINT> *ptree);
        static tree<MOUNTPOINT> * FindMountPoint(const char *pszVirtual, tree<MOUNTPOINT> *ptree);
        static bool WildcardMatch(const char *pszFilespec, const char *pszFilename);
        static void GetMountPointFindData(tree<MOUNTPOINT> *ptree, WIN32_FIND_DATA *pw32fd);

    public:
        typedef map<string, string> listing_type;
        VFS();
        void Mount(const char *pszVirtual, const char *pszLocal);
        DWORD GetDirectoryListing(const char *pszVirtual, DWORD dwIsNLST, listing_type &listing);
        bool FileExists(const char *pszVirtual);
        bool IsFolder(const char *pszVirtual);
        LPVOID FindFirstFile(const char *pszVirtual, WIN32_FIND_DATA *pw32fd);
        bool FindNextFile(LPVOID lpFindHandle, WIN32_FIND_DATA *pw32fd);
        void FindClose(LPVOID lpFindHandle);
        HANDLE CreateFile(const char *pszVirtual, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition);
        BOOL DeleteFile(const char *pszVirtual);
        BOOL MoveFile(const char *pszOldVirtual, const char *pszNewVirtual);
        BOOL CreateDirectory(const char *pszVirtual);
        BOOL RemoveDirectory(const char *pszVirtual);
        static void CleanVirtualPath(const char *pszVirtual, string &strNewVirtual);
        static void ResolveRelative(const char *pszCurrentVirtual, const char *pszRelativeVirtual, string &strNewVirtual);
        static void strtr(char *psz, char cFrom, char cTo);
};

#endif
