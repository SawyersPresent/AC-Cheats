#include <windows.h>
#include <stdio.h>

int main()
{
    // find the window by its title
    HWND hwnd = FindWindowA(NULL, "WindowTitleHere");
    if (hwnd == NULL)
    {
        printf("window not found\n");
        return 1;
    }

    // get the PID from the window handle
    DWORD procID;
    GetWindowThreadProcessId(hwnd, &procID);

    // open a handle to the process
    HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, procID);
    if (handle == NULL)
    {
        printf("failed to open process: %d\n", GetLastError());
        return 1;
    }

    // read 4 bytes from the address into our variable
    int value = 0;
    SIZE_T bytesRead;
    ReadProcessMemory(handle, (LPCVOID)0x03007640, &value, sizeof(value), &bytesRead);

    printf("value at address: %d\n", value);
    printf("bytes read: %zu\n", bytesRead);

    CloseHandle(handle);
    return 0;
}