#include "ScriptHook.h"

#include <Windows.h>

extern "C" __declspec(dllexport) void InitializeASI()
{
	MafiaScriptHook::GetScriptHook()->RegisterInstruction("msgbox_test", [&](std::string prototype, std::vector<std::string> args) {
		if (args.size() > 0)
		{
			MessageBoxA(NULL, args[0].c_str(), "MScriptHook Test", MB_OK | MB_ICONINFORMATION);
		}
		});
	MafiaScriptHook::GetScriptHook()->RegisterInstruction("do_pice_s_procesem", [&](std::string prototype, std::vector<std::string> args) {
		ExitProcess(0);
		});
}