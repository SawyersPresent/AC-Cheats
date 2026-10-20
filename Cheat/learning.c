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
#define BOT 0x4
#define Name 0x205
#define Health 0xEC
#define PROCESS_NAME "ac_client.exe"


DWORD GetPID(){
    PROCESSENTRY32 pe32; // here we define the normal process structure so were able to interact with PE's
    pe32.dwSize = sizeof(pe32); // then apparently its necessary to set the p32.dwsize even though its never used? 

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0); // we open up handles to  be able to take the snapshots necessary
    if (snapshot == INVALID_HANDLE_VALUE){ // add a failure check
        printf("it failed");
    }

        BOOL Processresult = Process32First(snapshot, &pe32); // now we use process32first to actually be able to parse the result of the snapshot, it takes the handle to
        
        while ( Processresult == TRUE ){ // while loop is actually baller because initially it was all if statments but ik the return values now so we chilling
                if (strcmp(PROCESS_NAME, pe32.szExeFile) == 0)
                {
                    printf("We have AC_Client\n");
                    printf("======================== \n");
                    printf("Name: %s \n", pe32.szExeFile);
                    printf("PID: %lu \n", pe32.th32ProcessID);
                    printf("PPID: %lu \n", pe32.th32ParentProcessID);
                    return pe32.th32ProcessID;
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

        if (ac_handle != NULL){
            printf("Now were using OpenProcess \n");
            
        }else{
            printf("process failed bozo \n");
            DWORD dw = GetLastError();
            printf("Error code: %lu\n", (unsigned long)dw);
        };

    return -1;
    
};

DWORD GetBase(){
        DWORD Func1 = GetPID();

        MODULEENTRY32 me32; // now we do the moduels so we can do deepre module enumeration
        me32.dwSize = sizeof(me32);

        HANDLE ModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, Func1); // we open up handles to  be able to take the snapshots necessary

        BOOL ModuleEnum = Module32First(ModuleSnapshot, &me32);
        if(ModuleEnum == TRUE){
            printf("here is the module base %p \n", me32.modBaseAddr); 
        }else{
            printf("it failed \n");
        };

        char mybuffer[MY_BUFFER_SIZE];

        
        HANDLE ac_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Func1);

        // now to read the module base
        DWORD OurPlayerBase = 0;

        ReadProcessMemory(ac_handle, me32.modBaseAddr+0x18AC00, &OurPlayerBase, sizeof(DWORD), NULL); // PlayerBase to be stored in DWORD
    
    
    return OurPlayerBase;
}

void PlayerNameFunction(char *buffer, int bufferSize){
        DWORD kys = GetPID("ac_client.exe");
        DWORD Base = GetBase();
        HANDLE ac_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, kys); 
        char PlayerNameFromBase = ReadProcessMemory(ac_handle, (LPCVOID)(Base+Name), // this is important, first we want to add the base and the name together and then use LPCVOID (a pointer to somewhere with no specified type)
        buffer, 
        bufferSize, 
        NULL); // finally able to actually use said playerbase and offsets. Nothing hardcoded
};


int main()
{

    // erm...
    DWORD pid = GetPID("ac_client.exe");
    printf("PID of ac_client %lu \n", pid);
    char name[24];    
    PlayerNameFunction(name, 24);
    printf("now what were going to do is the module retardation heres the name please work %s \n", name);

    return 0;
    
}