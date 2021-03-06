// libTools.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <tlhelp32.h>
#include <sstream>
#include <iterator>
#include <include/CharacterLib/Character.h>
#include <include/ProcessLib/Common/ResHandleManager.h>
#include <VersionHelpers.h>

#pragma comment(lib,"CharacterLib_Debug.lib")
#pragma comment(lib,"ProcessLib_Debug.lib")
#pragma comment(lib,"user32.lib")
#define _SELF L"asd"



enum class em_System_Version
{
	Unknow,
	WindowsXp,
	WindowsServer2003,
	WindowsVista,
	WindowsServer2008,
	WindowServer2008R2,
	Windows7,
	WindowsServer2012,
	Windows8,
	Windows10
};

em_System_Version GetSystemVersion()
{
	HMODULE hmDLL = ::GetModuleHandleW(L"ntdll.dll");
	if (hmDLL == NULL)
		return em_System_Version::Unknow;


	// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_osversioninfoexw
	using RtlGetVersionPtr = NTSTATUS(WINAPI *)(RTL_OSVERSIONINFOEXW *);
	RtlGetVersionPtr GetVersionPtr = (RtlGetVersionPtr)::GetProcAddress(hmDLL, "RtlGetVersion");
	if (GetVersionPtr == nullptr)
		return em_System_Version::Unknow;


	RTL_OSVERSIONINFOEXW Info = { 0 };
	Info.dwOSVersionInfoSize = sizeof(Info);
	if (GetVersionPtr(&Info) != 0)
		return em_System_Version::Unknow;


	if (Info.dwMajorVersion == 10 && Info.dwMinorVersion == 0)
		return em_System_Version::Windows10;
	else if (Info.dwMajorVersion == 6 && Info.dwMinorVersion == 2)
		return Info.wProductType == VER_NT_WORKSTATION ? em_System_Version::Windows8 : em_System_Version::WindowsServer2012;
	else if (Info.dwMajorVersion == 6 && Info.dwMinorVersion == 1)
		return Info.wProductType == VER_NT_WORKSTATION ? em_System_Version::Windows7 : em_System_Version::WindowServer2008R2;
	else if (Info.dwMajorVersion == 6 && Info.dwMinorVersion == 0)
		return Info.wProductType == VER_NT_WORKSTATION ? em_System_Version::WindowsVista : em_System_Version::WindowsServer2008;
	else if (Info.dwMajorVersion == 5 && Info.dwMinorVersion == 2)
		return em_System_Version::WindowsServer2003;
	else if (Info.dwMajorVersion == 5 && Info.dwMinorVersion == 1)
		return em_System_Version::WindowsXp;


	return em_System_Version::Unknow;
}

int main()
{
	auto v = GetSystemVersion();
	
    return 0;
}

