// https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>

int main( int, char *[] )
{
    https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-processentry32
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            // this is actually so fucking ballre brah what, goated struct and goated API call
            if (stricmp(entry.szExeFile, "ac_client.exe") == 0)
            {  
                // https://learn.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);

                printf("Found ac_client.exe with PID %d\n", entry.th32ProcessID);

                CloseHandle(hProcess);
            }
        }
    }

    CloseHandle(snapshot);

    return 0;
}
