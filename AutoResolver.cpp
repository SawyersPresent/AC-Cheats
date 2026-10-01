// ═══════════════════════════════════════════════════════
// win32 api reading order for external memory access
// ═══════════════════════════════════════════════════════

// 1. Process Security and Access Rights
//    understand what flags exist before you call anything
//    PROCESS_VM_READ, PROCESS_VM_WRITE, PROCESS_ALL_ACCESS etc
//    https://learn.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights

// 2. CreateToolhelp32Snapshot + Process32First + Process32Next + PROCESSENTRY32
//    how you find a PID from a process name like "ac_client.exe"
//    PROCESSENTRY32.th32ProcessID = the PID
//    PROCESSENTRY32.szExeFile     = the process name
//    https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot
//    https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-process32first
//    https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-process32next
//    https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-processentry32

// 3. OpenProcess
//    takes the PID, returns a HANDLE
//    returns NULL on failure
//    https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess
//    https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-processes

// 4. ReadProcessMemory
//    takes the HANDLE + address, reads bytes into your buffer
//    needs PROCESS_VM_READ
//    https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory

// 5. WriteProcessMemory
//    same but writes — freeze health, set position etc
//    needs PROCESS_VM_WRITE and PROCESS_VM_OPERATION
//    https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-writeprocessmemory

// 6. CloseHandle
//    always call this when done, clean up your handle
//    https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle

// 7. GetLastError
//    call whenever any of the above return NULL or FALSE
//    returns an error code that tells you what went wrong
//    https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror


#include <windows.h>  // gives you all win32 types and functions. HANDLE, DWORD, OpenProcess etc all live here
#include <stdio.h>    // printf family
#include <tchar.h>    // gives you TCHAR, TEXT(), _tprintf — handles unicode vs ascii automatically
#include <psapi.h>    // gives you EnumProcesses, EnumProcessModules, GetModuleBaseName
// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1


void PrintProcessNameAndID( DWORD processID )  // DWORD = unsigned 32-bit int, windows uses it everywhere for IDs
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    // TCHAR    = char that switches between ascii/unicode depending on build setting
    // MAX_PATH = 260, windows max path length constant
    // TEXT()   = wraps string literal so it works in both ascii and unicode mode
    // buffer to hold the process name, defaults to <unknown> if it cant be found


    HANDLE hProcess = OpenProcess(
    // HANDLE = opaque token windows gives you to refer to a kernel object
    // you dont know whats inside it, you just pass it to other functions
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        // PROCESS_QUERY_INFORMATION = lets you ask for info about the process
        // PROCESS_VM_READ           = lets you read memory from the process
        // | combines both flags — asking for both at once
        FALSE,      // dont let child processes inherit this handle
        processID   // which process to open, the PID we passed in
    );
    // returns NULL if it fails — process doesnt exist, no permission etc


    if (NULL != hProcess)  // always check. passing NULL to next functions will crash
    {
        HMODULE hMod;    // HMODULE = handle to a loaded module (.exe or .dll)
        DWORD cbNeeded;  // cbNeeded = "count bytes needed", common win32 pattern
                         // the function writes how many bytes it actually used into this

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
        // EnumProcessModules lists all modules loaded into the process
        // &hMod        = pass address of hMod so the function can write into it
        // sizeof(hMod) = 4 bytes = one pointer, we only want the first module
        //                which is always the main executable
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
            // reads the name of the module and writes it into szProcessName
            // sizeof(szProcessName)/sizeof(TCHAR) = how many characters fit in the buffer
            // divide by sizeof(TCHAR) because in unicode each char is 2 bytes not 1
        }
    }


    _tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );
    // _tprintf = unicode aware printf
    // %s = string, %u = unsigned integer


    CloseHandle( hProcess );
    // every OpenProcess must be paired with CloseHandle
    // if you dont, you leak a kernel handle
    // the OS has a limit on how many handles one process can have open
}


int main( void )
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    // making 3 DWORDs (unsigned 32-bit ints):
    // aProcesses[1024] = array of 1024 slots to hold PIDs, one DWORD per PID
    // cbNeeded         = EnumProcesses writes how many bytes it used into this
    // cProcesses       = we calculate the actual count of processes from cbNeeded

    unsigned int i;  // loop counter


    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    // EnumProcesses fills aProcesses with all current PIDs
    // sizeof(aProcesses) = 1024 * 4 = 4096 bytes, tells it how big our buffer is
    // &cbNeeded          = it writes how many bytes it actually used here
    // ! = if it FAILED bail out, win32 returns zero on failure non-zero on success
    {
        return 1;
    }


    cProcesses = cbNeeded / sizeof(DWORD);
    // cbNeeded is in bytes, each PID is a DWORD (4 bytes)
    // dividing gives you the actual count of processes that were returned


    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        // PID 0 is the system idle process, not a real process, skip it
        {
            PrintProcessNameAndID( aProcesses[i] );
        }
    }

    return 0;
}