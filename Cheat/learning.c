#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>



int main()
{
    PROCESSENTRY32 pe32; // here we define the normal process structure so were able to interact with PE's
    pe32.dwSize = sizeof(pe32); // then apparently its necessary to set the p32.dwsize even though its never used? 

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 69696969); // we open up handles to  be able to take the snapshots necessary
    if (snapshot == INVALID_HANDLE_VALUE){ // add a failure check
        printf("it failed");
    }

    BOOL Processresult = Process32First(snapshot, &pe32); // now we use process32first to actually be able to parse the result of the snapshot, it takes the handle to
        while ( Processresult == TRUE ){ // while loop is actually baller because initially it was all if statments but ik the return values now so we chilling
                printf("it worked! \n");
                printf("name: %s \n",pe32.szExeFile); // so after getting the handle to what we want, then we would use the pointer to the process structure
                printf("PID: %lu \n",pe32.th32ProcessID); // it worked, it needs the right return % 

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

    return 0;
    
}