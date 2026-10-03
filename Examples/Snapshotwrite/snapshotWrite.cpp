#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

DWORD GetProcessID(const char *processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

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

DWORD GetModuleBase(DWORD pid, const char *moduleName)
{
    DWORD base = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

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
    // targeting notepad.exe — swap for any process you want
    DWORD pid = GetProcessID("notepad.exe");
    if (pid == 0) { printf("process not found\n"); return 1; }
    printf("pid: %lu\n", pid);

    HANDLE handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (handle == NULL) { printf("failed to open: %d\n", GetLastError()); return 1; }

    DWORD moduleBase = GetModuleBase(pid, "notepad.exe");
    if (moduleBase == 0) { printf("module not found\n"); CloseHandle(handle); return 1; }
    printf("module base: 0x%08X\n", moduleBase);

    // swap these two values for whatever process and offsets you are targeting
    DWORD staticPointerOffset = 0x00012345;   // your ac_client.exe+XXXXXX offset goes here
    DWORD fieldOffset         = 0x000000EC;   // your field offset goes here

    // follow the pointer chain
    DWORD staticPointerAddr = moduleBase + staticPointerOffset;

    DWORD objectBase = 0;
    ReadProcessMemory(handle, (LPCVOID)staticPointerAddr, &objectBase, sizeof(objectBase), NULL);
    printf("object base: 0x%08X\n", objectBase);

    DWORD fieldAddr = objectBase + fieldOffset;
    printf("field address: 0x%08X\n", fieldAddr);

    // write a new value
    int newValue = 9999;
    SIZE_T bytesWritten = 0;
    BOOL result = WriteProcessMemory(handle, (LPVOID)fieldAddr, &newValue, sizeof(newValue), &bytesWritten);

    if (result)
        printf("wrote %zu bytes — value set to %d\n", bytesWritten, newValue);
    else
        printf("write failed: %d\n", GetLastError());

    CloseHandle(handle);
    return 0;
}