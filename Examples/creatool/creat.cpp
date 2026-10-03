#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

// step 1 — find the PID from the process name
DWORD GetProcessID(const char *processName)
{
    DWORD pid = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &pe))
    {
        do
        {
            if (strcmp(pe.szExeFile, processName) == 0)
            {
                pid = pe.th32ProcessID;
                break;
            }
        }
        while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return pid;
}

// step 2 — find where ac_client.exe loaded in memory this session
DWORD GetModuleBase(DWORD pid, const char *moduleName)
{
    DWORD base = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snapshot, &me))
    {
        do
        {
            if (strcmp(me.szModule, moduleName) == 0)
            {
                base = (DWORD)me.modBaseAddr;
                break;
            }
        }
        while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return base;
}

int main()
{
    // find ac_client.exe
    DWORD pid = GetProcessID("ac_client.exe");
    if (pid == 0)
    {
        printf("ac_client.exe not found\n");
        return 1;
    }
    printf("pid: %lu\n", pid);

    // open a handle to the process
    HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (handle == NULL)
    {
        printf("failed to open process: %d\n", GetLastError());
        return 1;
    }

    // get the base address of ac_client.exe this session
    DWORD moduleBase = GetModuleBase(pid, "ac_client.exe");
    if (moduleBase == 0)
    {
        printf("failed to get module base\n");
        CloseHandle(handle);
        return 1;
    }
    printf("module base: 0x%08X\n", moduleBase);

    // calculate where the static pointer lives
    // ac_client.exe+18AC00 = moduleBase + 0x18AC00
    DWORD staticPointerAddr = moduleBase + 0x18AC00;
    printf("static pointer address: 0x%08X\n", staticPointerAddr);

    // read the static pointer to get the player object base
    DWORD objectBase = 0;
    ReadProcessMemory(handle, (LPCVOID)staticPointerAddr, &objectBase, sizeof(objectBase), NULL);
    if (objectBase == 0)
    {
        printf("failed to read object base\n");
        CloseHandle(handle);
        return 1;
    }
    printf("object base: 0x%08X\n", objectBase);

    // calculate health address
    // object base + 0xEC = health field
    DWORD healthAddr = objectBase + 0xEC;
    printf("health address: 0x%08X\n", healthAddr);

    // read health value
    int health = 0;
    ReadProcessMemory(handle, (LPCVOID)healthAddr, &health, sizeof(health), NULL);
    printf("health: %d\n", health);

    CloseHandle(handle);
    return 0;
}