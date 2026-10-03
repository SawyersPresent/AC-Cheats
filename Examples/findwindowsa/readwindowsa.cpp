#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

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

int main()
{
    // find the process by name
    DWORD pid = GetProcessID("ac_client.exe");
    if (pid == 0)
    {
        printf("process not found\n");
        return 1;
    }

    printf("found process PID: %lu\n", pid);

    // open a handle
    HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (handle == NULL)
    {
        printf("failed to open process: %d\n", GetLastError());
        return 1;
    }

    // read 4 bytes from the address
    int value = 0;
    SIZE_T bytesRead;
    ReadProcessMemory(handle, (LPCVOID)0x03007640, &value, sizeof(value), &bytesRead);

    printf("value at address: %d\n", value);
    printf("bytes read: %zu\n", bytesRead);

    CloseHandle(handle);
    return 0;
}