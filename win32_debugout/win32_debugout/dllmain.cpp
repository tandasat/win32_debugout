
#include "stdafx.h"


static void __stdcall DebugPrint(
    const char* Str)
{
    ::OutputDebugStringA(Str);
}


// The hook routine called from installed code on the padding space
__declspec(naked)
static void HookTrampoline()
{__asm{
    pushad
    pushfd

    // At this moment, esi+14h is a retrieved debug string.
    lea  eax, [esi+14h]
    push eax
    call DebugPrint

    popfd
    popad

    // Continue original instructions
    push 0x41F7A1   // jmp     0x41F7A1
    ret
}}


// Installs OUTPUT_DEBUG_STRING_EVENT hook.
static bool InstallHook()
{
    // A patch address to hook OUTPUT_DEBUG_STRING_EVENT handler
    // on win32_remote.exe.
    static const DWORD TARGET_ADDR  = 0x41FC44;

    // Padding space at the end of the function having TARGET_ADDR.
    static const DWORD FUNCEND_ADDR = 0x41FC98;

    static_assert(FUNCEND_ADDR > TARGET_ADDR,
        "FUNCEND_ADDR has to point a bigger address than TARGET_ADDR.");
    static_assert((FUNCEND_ADDR & 0xfffff000) == (TARGET_ADDR & 0xfffff000),
        "FUNCEND_ADDR and TARGET_ADDR have to be in the same page.");

    // Check the content of a hook target address.
    // 0041FC44 E9 58 FB FF FF    jmp     loc_41F7A1
    auto targetAddr = reinterpret_cast<WORD*>(TARGET_ADDR);
    if (*targetAddr != 0x58E9)
    {
        return false;
    }

    // Check the contents of Padding space.
    auto funcEndAddr = reinterpret_cast<BYTE*>(FUNCEND_ADDR);
    for (int i = 0; i < 5; ++i)
    {
        if (funcEndAddr[i] != 0xcc) // has to be empty.
        {
            return false;
        }
    }

    DWORD oldProtect = 0;
    auto result = ::VirtualProtect(targetAddr, 0x1000, PAGE_EXECUTE_READWRITE,
        &oldProtect);
    if (!result)
    {
        return false;
    }

    // Patch to padding space
    // 0041FC98 68 HookTrampoline push    HookTrampoline
    // 0041FC9d C3                retn
    funcEndAddr[0] = 0x68;
    *reinterpret_cast<DWORD*>(funcEndAddr + 1) = 
        reinterpret_cast<DWORD>(&HookTrampoline);
    funcEndAddr[5] = 0xC3;

    // Patch to the handler address
    // 0041FC44 EB 52             jmp     short near ptr hook_trampoline
    targetAddr[0] = 0x52EB;

    ::FlushInstructionCache(::GetCurrentProcess(), targetAddr, 0x1000);
    ::VirtualProtect(targetAddr, 0x1000, oldProtect, &oldProtect);
    return true;
}


BOOL APIENTRY DllMain(
    HMODULE hinstDLL,
    DWORD  fdwReason,
    LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            if (::IsDebuggerPresent())
            {
                __debugbreak();
            }

            if (!InstallHook())
            {
                DebugPrint("DLL installation failed.\n");
                return FALSE;
            }
            DebugPrint("DLL installation succeeded.\n");
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

