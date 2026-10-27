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

#define MY_BUFFER_SIZE 260
#define BOT 0x4
#define Name 0x205
#define Health 0xEC
#define PROCESS_NAME "ac_client.exe"
#define Playerbase 0x18AC00
#define BotBase 0x18AC04

// define the container ONCE outside everything
struct ACProcess
{
    DWORD pid;
    DWORD base;
    HANDLE handle;
    char name[260];    
    DWORD health;
    DWORD botsarray[120];
};

struct ACBots{
    DWORD pid;
    DWORD base;
    HANDLE handle;
    DWORD botsarray[120];

};

struct ACProcess player;

struct ACProcess bot; 



struct ACProcess GetPID(){

    PROCESSENTRY32 pe32; // here we define the normal process structure so were able to interact with PE's
    pe32.dwSize = sizeof(pe32); // then apparently its necessary to set the p32.dwsize even though its never used? 

    struct ACProcess result;
    result.pid = 0;
    result.handle = NULL;


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
                    printf("PPID: %lu \n", pe32.th32ParentProcessID); // QA By frog, it shouldnt be printing since its doing more than it should, its only for debugging.
                    result.pid = pe32.th32ProcessID;
                    // the openprocess API now
                    HANDLE ac_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

                    if (ac_handle != NULL){
                        printf("Now were using OpenProcess \n");
                        
                    }else{
                        printf("process failed bozo \n");
                        DWORD dw = GetLastError();
                        printf("Error code: %lu\n", (unsigned long)dw);
                    };
                    
                    result.handle = ac_handle;  
                    return result;
                    
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
    return result; // 
    
};






struct ACProcess GetBase(struct ACProcess ac, DWORD offset){
        MODULEENTRY32 me32; // now we do the moduels so we can do deepre module enumeration
        me32.dwSize = sizeof(me32);

        HANDLE ModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ac.pid); // we open up handles to  be able to take the snapshots necessary

        BOOL ModuleEnum = Module32First(ModuleSnapshot, &me32);
        if(ModuleEnum == TRUE){
            printf("here is the module base %p \n", me32.modBaseAddr); 
        }else{
            printf("it failed \n");
        };

        // now to read the module base
        DWORD OurPlayerBase = 0;

        ReadProcessMemory(ac.handle, (LPCVOID)(me32.modBaseAddr+offset), &OurPlayerBase, sizeof(DWORD), NULL); // PlayerBase to be stored in DWORD

        ac.base = OurPlayerBase;
    
    return ac;
};


struct ACProcess PlayerNameRead(struct ACProcess ac){
        BOOL PlayerNameFromBase = ReadProcessMemory(ac.handle, (LPCVOID)(ac.base+Name), // this is important, first we want to add the base and the name together and then use LPCVOID (a pointer to somewhere with no specified type)
        ac.name, 
        sizeof(ac.name), 
        NULL); // finally able to actually use said playerbase and offsets. Nothing hardcoded
        return ac;
};



 struct ACProcess PlayerNameWrite(struct ACProcess ac){
    SIZE_T bytesRead = 0;
    char mybuffer[260] = "okayitshallwork123"; //how to initialize an array
    BOOL ok = WriteProcessMemory(
        ac.handle,
        (LPCVOID)(ULONG_PTR)(ac.base+Name),
        mybuffer,
        strlen(mybuffer) + 1,   // +1 for null terminator
        &bytesRead
    );
    return ac;
};



struct ACProcess PlayerHealth(struct ACProcess ac){
        BOOL PlayerHealthBOOL = ReadProcessMemory(
            ac.handle, 
            (LPCVOID)(ULONG_PTR)(ac.base+Health), // mistake 3 was: (LPCVOID)(ac.base+Health), DWORD to LPCVOID size mismatch, needs ULONG_PTR cast
            &ac.health, 
            sizeof(ac.health), 
            NULL);
         // mistake 4: this is correct but pointless without return ac — sets the copy then copy disappears
        return ac; // mistake 4 fix: without this line ac.health is set on the copy and thrown away
};

struct ACProcess PlayerHealthWrite(struct ACProcess ac){
    SIZE_T bytesRead = 0;
    int healthWrite = 777;
    BOOL ok = WriteProcessMemory(
        ac.handle,
        (LPCVOID)(ULONG_PTR)(ac.base+Health), // where to write in AC memory, DWORD→ULONG_PTR→LPCVOID to handle 32→64 bit widening fucked it up
        &healthWrite, // another mistake forgot to add the pointer to healthwrite
        sizeof(healthWrite),
        &bytesRead
    );
    return ac;
}



struct ACProcess BotEnum(struct ACProcess ac){
        
        ac.handle = player.handle;
        ac.pid = player.pid;

        bot = GetBase(bot, BotBase);          // this line is missing
        printf("PID of ac_client %lu \n", ac.pid);
        printf("bot array base: 0x%lX\n", ac.base);


        // ReadProcessMemory(
        //     bot.handle, 
        //     (LPCVOID)(ULONG_PTR)(bot.base+BOT), // mistake 3 was: (LPCVOID)(ac.base+Health), DWORD to LPCVOID size mismatch, needs ULONG_PTR cast
        //     &bot.base, 
        //     sizeof(bot.base), 
        //     NULL);
    return ac;
}


struct ACProcess BotArray(struct ACProcess ac){
    BotEnum(bot);
    ac.botsarray;
    int i = 0;
    while (i < 1) {
        printf("%d\n", i);
        ReadProcessMemory(
            bot.handle, 
            (LPCVOID)(ULONG_PTR)(bot.base), // mistake 3 was: (LPCVOID)(ac.base+BOT_Name), DWORD to LPCVOID size mismatch, needs ULONG_PTR cast
            &ac.botsarray[i], 
            sizeof(bot.base), 
            NULL);
        
        printf("This bot object is over at %lu\n", bot.base);
        bot.base = bot.base + 0x4;
        i++;
        
    }
    return ac;
};



int main()
{
    // erm...
    
    player = GetPID();

    player = GetBase(player, Playerbase);          // this line is missing
    printf("PID of ac_client %lu \n", player.pid);

    player = PlayerNameRead(player); // char* buffer is a pointer so writes go to original memory directly, no catch needed.
    printf("now what were going to do is read the old name %s \n", player.name);

    player = PlayerHealth(player); // okay so everytime i NEED TO CALL the function I need to re-assign the object  to make sure the object members are updated 
    printf("Now this is the health %d \n", player.health);
    
    printf("now we need to write the memory \n");

    player = PlayerHealthWrite(player);
    printf("New health: %d \n", player.health);
    
    PlayerNameWrite(player);
    player = PlayerNameRead(player);
    printf("Now this is the new name %s \n", player.name); //HOLY SHIT IT WORKS PLAYERNAME WRITE ALSO WORKS

    printf("======================================================== \n"); 
    printf("Now its the bots turn I guess\n");
    bot.handle = player.handle;
    bot.pid = player.pid;
    
    bot = GetBase(bot, BotBase);          // this line is missing
    printf("PID of ac_client %lu \n", bot.pid);
    printf("bot array base: 0x%lX\n", bot.base);

    ReadProcessMemory(
            bot.handle, 
            (LPCVOID)(ULONG_PTR)(bot.base+BOT), // mistake 3 was: (LPCVOID)(ac.base+Health), DWORD to LPCVOID size mismatch, needs ULONG_PTR cast
            &bot.base, 
            sizeof(bot.base), 
            NULL);

    printf("Possible accurate base finally? %lu \n", bot.base); //HOLY SHIT IT WORKS PLAYERNAME WRITE ALSO WORKS

    
    bot = PlayerNameRead(bot); // char* buffer is a pointer so writes go to original memory directly, no catch needed.
    printf("now what were going to do is read the old name %s \n", bot.name);
    printf("GetLastError: %lu\n", GetLastError());
    printf("botterdress: 0x%lX\n", bot.base);

    bot = PlayerHealth(bot); // okay so everytime i NEED TO CALL the function I need to re-assign the object  to make sure the object members are updated 
    printf("Now this is the health %d \n", bot.health);
    
    printf("now we need to write the memory \n");

    bot = PlayerHealthWrite(bot);
    printf("New health: %d \n", bot.health);
    
    PlayerNameWrite(bot);
    bot = PlayerNameRead(bot);
    printf("Now this is the new name %s \n", bot.name); //HOLY SHIT IT WORKS PLAYERNAME WRITE ALSO WORKS
    
    printf("==================================== \n"); 

    BotArray(bot); //works!

    printf("the first address", bot.botsarray[0]);

    return 0;
    
}