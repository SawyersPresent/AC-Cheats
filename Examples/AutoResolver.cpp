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


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

//  Forward declarations:
BOOL GetProcessList( );
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR const* msg );


void PrintProcessNameAndID( DWORD processID )
{   
    // set the process Name I guess
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    // opens a handle to the process specfic access rights, PROCESS_QUERY_INFORMATION and PROCESS_VM_READ here false is when the handle doesnt work (?)
    // https://learn.microsoft.com/en-us/windows/desktop/ProcThread/process-security-and-access-rights
    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess
    HANDLE hProcess = OpenProcess( 
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, // defining both permissions we want to use, PROCESS_QUERY_INFORMATION is for getting the process name and PROCESS_VM_READ is for reading memory from the process which isnt used yet.
            FALSE, // if the handle is inheritable or not, false means it is not inheritable
            processID ); // the process ID of the process we want to open a handle to

    // Get the process name.
    // if it is null then what happens is (i dont know yet im retarded)
    // 
    // https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodules
    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }

    // Print the process name and identifier.

    _tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );

    // Release the handle to the process.

    CloseHandle( hProcess );
}

int main( void )
{
    // Get the list of process identifiers.

    DWORD aProcesses[2048], cbNeeded, cProcesses; // holds space for 2048 bytes of process identifiers
    unsigned int i; // why is this faggot here we cant have negative fucking numbers now do we


    // this is bullshit what the fuck am i looking at.
    // ===================================================================================================================================================================
    // API Specifics:
    // what we do is that we use the first parameter recieved the PIDs, second one is the sizeof said parameter, last one is the number of arrays returned (confused)
    // ===================================================================================================================================================================
    // LOOP SPECIFICS
    // the loop here is so we can skip one line but the real code is going to look like this 
    // ===================================================================================================================================================================
    
    // CODING HERE
    // BOOL result = EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded );
    // if ( result == FALSE )
    // {
    //     return 1;
    // }

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return 1;
    }


    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.
    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        {
            PrintProcessNameAndID( aProcesses[i] );
        }
    }

    return 0;
}