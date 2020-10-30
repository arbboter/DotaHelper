#include <windows.h>
#include <winnt.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include "CloakDll.h"

#pragma warning(disable: 4996)
#pragma comment(lib, "shlwapi.lib")

#define UPPERCASE(x) if((x) >= 'a' && (x) <= 'z') (x) -= 'a' - 'A'
#define UNLINK(x) (x).Blink->Flink = (x).Flink; \
	(x).Flink->Blink = (x).Blink;
	
#pragma pack(push, 1)

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ModuleInfoNode
{
	LIST_ENTRY LoadOrder;
	LIST_ENTRY InitOrder;
	LIST_ENTRY MemoryOrder;
	HMODULE baseAddress;		//	Base address AKA module handle
	unsigned long entryPoint;
	unsigned int size;			//	Size of the modules image
	UNICODE_STRING fullPath;
	UNICODE_STRING name;
	unsigned long flags;
	unsigned short LoadCount;
	unsigned short TlsIndex;
	LIST_ENTRY HashTable;	//	A linked list of any other modules that have the same first letter
	unsigned long timestamp;
} ModuleInfoNode, *pModuleInfoNode;

typedef struct _ProcessModuleInfo
{
	unsigned int size;			//	Size of a ModuleInfo node?
	unsigned int initialized;
	HANDLE SsHandle;
	LIST_ENTRY LoadOrder;
	LIST_ENTRY InitOrder;
	LIST_ENTRY MemoryOrder;
} ProcessModuleInfo, *pProcessModuleInfo;

#pragma pack(pop)

bool HideDLL( HMODULE hDLL )
{
    ProcessModuleInfo *pmInfo;
    ModuleInfoNode *module;
 
    __asm
    {
        mov eax, fs:[18h] // TEB
        mov eax, [eax + 30h] // PEB
        mov eax, [eax + 0Ch] // PROCESS_MODULE_INFO
        mov pmInfo, eax
    }
 
    module = (ModuleInfoNode *)(pmInfo->LoadOrder.Flink);
 
    while(module->baseAddress && module->baseAddress != hDLL)
        module = (ModuleInfoNode *)(module->LoadOrder.Flink);
 
    if(!module->baseAddress)
        return false;
 
    //   Remove the module entry from the list here
    ///////////////////////////////////////////////////   
    //   Unlink from the load order list
    UNLINK(module->LoadOrder);
    //   Unlink from the init order list
    UNLINK(module->InitOrder);
    //   Unlink from the memory order list
    UNLINK(module->MemoryOrder);
    //   Unlink from the hash table
    UNLINK(module->HashTable);
 
    //   Erase all traces that it was ever there
    ///////////////////////////////////////////////////
 
    //   This code will pretty much always be optimized into a rep stosb/stosd pair
    //   so it shouldn't cause problems for relocation.
    //   Zero out the module name
    for(int i=0; i<module->fullPath.Length; i++)
        ((BYTE*)module->fullPath.Buffer)[i] = 0;
    //    Zero out the memory of this module's node
    for(int i=0; i<sizeof(ModuleInfoNode); i++)
        ((BYTE*)module)[i] = 0;  
    return true;
}