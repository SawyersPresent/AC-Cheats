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

    // open a handle — need VM_WRITE and VM_OPERATION for writing
    HANDLE handle = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, procID);
    if (handle == NULL)
    {
        printf("failed to open process: %d\n", GetLastError());
        return 1;
    }

    // write a new value to the address
    int newValue = 9999;
    SIZE_T bytesWritten;
    WriteProcessMemory(handle, (LPVOID)0x03007640, &newValue, sizeof(newValue), &bytesWritten);

    printf("wrote %zu bytes\n", bytesWritten);

    CloseHandle(handle);
    return 0;
}