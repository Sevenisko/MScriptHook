#pragma once

#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <string>
#include <vector>

#define SH_EXPORT __declspec(dllexport)

extern HWND gGameWindow;

inline std::vector<std::string> SplitString(std::string s, std::string delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

inline bool IsNumber(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

inline std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

inline void DebugPrintf(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);

	vsprintf(buf, fmt, args);

	OutputDebugStringA(buf);
	printf(buf);

	va_end(args);
}

inline void FatalError(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);

	vsprintf(buf, fmt, args);

	MessageBoxA(NULL, buf, "MScriptHook", MB_OK | MB_ICONERROR);

	ExitProcess(1);

	va_end(args);
}

inline bool AttachToConsole()
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (GetLastError() != ERROR_ACCESS_DENIED) //already has a console
		{
			if (!AttachConsole(GetCurrentProcessId()))
			{
				DWORD dwLastError = GetLastError();
				if (dwLastError != ERROR_ACCESS_DENIED) //already has a console
				{
					return false;
				}
			}
		}
	}

	return true;
}

typedef int(__thiscall* C_program__DecodeInstruction_func)(void*, char*, int);
typedef int(__thiscall* C_program__DecodeParams_func)(void*, char*, DWORD);
typedef void(__thiscall* C_program__Process_func)(void*, unsigned int);
typedef int(__thiscall* C_entity__DecodeInstruction_func)(void*, char*, int);
typedef char(__thiscall* C_entity__DecodeParams_func)(void*, char*, DWORD);

extern C_program__DecodeInstruction_func C_program__DecodeInstruction_original;
extern C_program__DecodeParams_func C_program__DecodeParams_original;
extern C_program__Process_func C_program__Process_original;
extern C_entity__DecodeInstruction_func C_entity__DecodeInstruction_original;
extern C_entity__DecodeParams_func C_entity__DecodeParams_original;

class CProgramHooks
{
public:
	static void Hook();
};