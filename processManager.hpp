#ifndef __PROCESS_MANAGER_H__
#define __PROCESS_MANAGER_H__

// Written by: Uglješa Ćirić
// Date: 15.03.2021.

// Easy to use memory management library implemented using Windows API

#include <Windows.h>
#include <vector>

#define INTERNAL 0 // 1 if you are doing internal
#define EXTERNAL 1 // 1 if you are doing external

namespace process
{
#if EXTERNAL
	#include <TlHelp32.h>
	#pragma message( "Process Manager set to EXTERNAL" )

	struct clientInfo
	{
		HWND GameWindow;
		DWORD processID;
		HANDLE processHandle;
		uintptr_t gameBaseAddress;
	};

	bool hookEx(const wchar_t* window_name, const wchar_t* module_name, clientInfo& info);
	void writeEx(LPVOID destination, LPCVOID data, size_t size, HANDLE hProcess);
	void readEx(DWORD target, LPVOID& container, size_t size, HANDLE hProcess);
	void nopEx(LPVOID destination, size_t size, HANDLE hProcess);
	uintptr_t traceOffsetsEx(uintptr_t base, std::vector<DWORD> offsets, HANDLE hProcess);


	uintptr_t GetModuleBaseAddress(DWORD processID, const wchar_t* moduleName);

#elif INTERNAL
	#pragma message( "Process Manager set to INTERNAL" )
	#include <stdio.h>

	void nop(DWORD destination, size_t size);
	void write(DWORD destination, LPVOID data, size_t size);
	DWORD traceOffsets(DWORD base, std::vector<DWORD> offsets);
	void showConsole(const char* title);
	void closeConsole(void);

#else
	#error You did not set any setting in processManager.hpp!
#endif
}
#endif // !__PROCESS_MANAGER_H__