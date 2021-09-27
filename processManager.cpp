#include "processManager.hpp"

#if INTERNAL

// Writes a chunk of data to a provided memory location.
void process::write(DWORD destination, LPVOID data, size_t size)
{
    DWORD oldprotect;
    VirtualProtect((LPVOID)destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy((LPVOID)destination, data, size);
    VirtualProtect((LPVOID)destination, size, oldprotect, &oldprotect);
}

// Writes no-ops to the provided memory location with specified size. In x86/64 assembly opcode for NOP is 0x90
void process::nop(DWORD destination, size_t size)
{
    DWORD oldprotect;
    VirtualProtect((LPVOID)destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memset((LPVOID)destination, 0x90, size);
    VirtualProtect((LPVOID)destination, size, oldprotect, &oldprotect);
}

// Resolves multi-level pointers.
DWORD process::traceOffsets(DWORD base, std::vector<DWORD> offsets)
{
    DWORD result = base;

    for (DWORD offset : offsets)
    {
        result = *(DWORD*)result;
        result += offset;
    }

    return result;
}

// Allocated console and opens read and write streams for it. Avoid in final builds as anti-cheats can easily detect such operations.
void process::showConsole(const char* title)
{
    AllocConsole();
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    SetConsoleTitle(title);
    printf("Successfully hooked\n");
}

// Closes the allocated console.
void process::closeConsole(void)
{
	fclose((FILE*)stdin);
	fclose((FILE*)stdout);
	FreeConsole();
	PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
}

#elif EXTERNAL

// Writes to the specified memory region.
void process::writeEx(LPVOID destination, LPCVOID data, size_t size, HANDLE hProcess)
{
    DWORD oldprotect;
    VirtualProtect(destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    WriteProcessMemory(hProcess, destination, data, size, nullptr);
    VirtualProtect(destination, size, oldprotect, &oldprotect);
}

// Reads a chunk of data from specified memory location.
void process::readEx(DWORD target, LPVOID& container, size_t size, HANDLE hProcess)
{
    ReadProcessMemory(hProcess, (LPCVOID)target, container, size, NULL);
}

// Writes no-ops to the provided memory location with specified size. In x86/64 assembly opcode for NOP is 0x90
void process::nopEx(LPVOID destination, size_t size, HANDLE hProcess)
{
    BYTE* nopArray = new BYTE[size];
    memset(nopArray, 0x90, size);

    writeEx(destination, nopArray, size, hProcess);

    delete[] nopArray;
}

// Resolves multi-level pointers.
uintptr_t process::traceOffsetsEx(uintptr_t base, std::vector<DWORD> offsets, HANDLE hProcess)
{
    uintptr_t result = base;

    for (DWORD offset : offsets)
    {
        ReadProcessMemory(hProcess, (LPVOID)base, &result, sizeof(result), nullptr);
        result += offset;
    }

    return result;
}

// Needs to be called in order to gain handles to the process you are working on.
bool process::hookEx(const wchar_t* window_name, const wchar_t* module_name, clientInfo& info)
{
    HWND GameWindow = FindWindowW(NULL, window_name);
    DWORD processID;
    HANDLE processHandle;
    uintptr_t gameBaseAddress;

    if (GameWindow == NULL)
    {
        return false;
    }

    GetWindowThreadProcessId(GameWindow, &processID);
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

    if (!processHandle)
    {
        return false;
    }

    gameBaseAddress = process::GetModuleBaseAddress(processID, module_name); // Gets module base address. (iw5mp.exe for example)

    // By now process is successfully found and oppened!

    info.gameBaseAddress = gameBaseAddress;
    info.processHandle = processHandle;
    info.GameWindow = GameWindow;
    info.processID = processID;

    return true;
}

uintptr_t process::GetModuleBaseAddress(DWORD processID, const wchar_t* moduleName)
{
    /* Finds module base addresses. Creates snapshot of modules running in specified
       process. Iterates through it until it finds the one we need. When it does,
       it takes it's address and returns it. Closes the handle, don't want mem leaks. */

    uintptr_t moduleBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!wcscmp(reinterpret_cast<wchar_t*>(modEntry.szModule), moduleName))
                {
                    moduleBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }

    CloseHandle(hSnap);
    return moduleBaseAddr;
}

#endif
