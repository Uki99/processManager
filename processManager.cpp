#include "processManager.hpp"

#if INTERNAL

void process::write(DWORD destination, LPVOID data, size_t size)
{
    DWORD oldprotect;
    VirtualProtect((LPVOID)destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy((LPVOID)destination, data, size);
    VirtualProtect((LPVOID)destination, size, oldprotect, &oldprotect);
}

void process::nop(DWORD destination, size_t size)
{
    DWORD oldprotect;
    VirtualProtect((LPVOID)destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memset((LPVOID)destination, 0x90, size);
    VirtualProtect((LPVOID)destination, size, oldprotect, &oldprotect);
}

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

void process::showConsole(const char* title)
{
    AllocConsole();

    FILE* stream;

    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONIN$", "r", stdin);

    SetConsoleTitle(title);
    printf("Successfully hooked\n");
}

void process::closeConsole(void)
{
    FreeConsole();
    fclose(stdout);
    fclose(stdin);
}

#elif EXTERNAL

void process::writeEx(LPVOID destination, LPCVOID data, size_t size, HANDLE hProcess)
{
    DWORD oldprotect;
    VirtualProtect(destination, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    WriteProcessMemory(hProcess, destination, data, size, nullptr);
    VirtualProtect(destination, size, oldprotect, &oldprotect);
}

void process::readEx(DWORD target, LPVOID& container, size_t size, HANDLE hProcess)
{
    ReadProcessMemory(hProcess, (LPCVOID)target, container, size, NULL);
}

void process::nopEx(LPVOID destination, size_t size, HANDLE hProcess)
{
    BYTE* nopArray = new BYTE[size];
    memset(nopArray, 0x90, size);

    writeEx(destination, nopArray, size, hProcess);

    delete[] nopArray;
}

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