// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-moduleentry32
// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-module32first
// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot
// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-process32first
// https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-process32next
// https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory


#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <tlhelp32.h>

#define MY_BUFFER_SIZE 24



int main()
{
    PROCESSENTRY32 pe32; // here we define the normal process structure so were able to interact with PE's
    pe32.dwSize = sizeof(pe32); // then apparently its necessary to set the p32.dwsize even though its never used? 

    MODULEENTRY32 me32; // now we do the moduels so we can do deepre module enumeration
    me32.dwSize = sizeof(me32);



    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 69696969); // we open up handles to  be able to take the snapshots necessary
    if (snapshot == INVALID_HANDLE_VALUE){ // add a failure check
        printf("it failed");
    }

        BOOL Processresult = Process32First(snapshot, &pe32); // now we use process32first to actually be able to parse the result of the snapshot, it takes the handle to
        
        while ( Processresult == TRUE ){ // while loop is actually baller because initially it was all if statments but ik the return values now so we chilling
                if (strcmp("ac_client.exe", pe32.szExeFile) == 0)
                {
                    printf("We have AC_Client\n");
                    printf("======================== \n");
                    printf("Name: %s \n", pe32.szExeFile);
                    printf("PID: %lu \n", pe32.th32ProcessID);
                    printf("PPID: %lu \n", pe32.th32ParentProcessID);
                    printf("Module Base %lu \n", pe32.th32ModuleID);
                    break;
                };

            BOOL NextProcess = Process32Next(snapshot, &pe32);                
            Processresult = NextProcess;

            if (Processresult == FALSE )
            {
                printf("not copied \n");
                if(GetLastError() == ERROR_NO_MORE_FILES){
                    printf("no processes exist or the snapshot does not contain process information \n");
                }
            };
        }

        // the openprocess API now
        HANDLE ac_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

        if (!ac_handle == NULL){
            printf("Now were using OpenProcess \n");
            
        }else{
            printf("process failed bozo \n");
            DWORD dw = GetLastError();
            printf("Error code: %lu\n", (unsigned long)dw);
        };

        // now for the memory

        char mybuffer[MY_BUFFER_SIZE];

        BOOL rMemory = ReadProcessMemory(ac_handle, (LPCVOID)0x007C4BF5, mybuffer, sizeof(mybuffer), NULL); // IT WORKEDD
        
        if (rMemory != 0){
            printf("le success \n");
            printf("Here is the data in that offset %s \n" , mybuffer);
        }

        // now to read the module base
        
        HANDLE ModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 31080); // we open up handles to  be able to take the snapshots necessary

        BOOL ModuleEnum = Module32First(ModuleSnapshot, &me32);
        if(ModuleEnum == TRUE){
            printf("here is the module base %lu \n", me32.modBaseAddr); 
        }else{
            printf("it failed \n");
        };




    return 0;
    
}