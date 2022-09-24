#include "Internal.h"
#include "ScriptHook.h"
#include <cstdint>
#include <map>
#include <conio.h>
#include <ctype.h>

typedef int(*GetEngineVersion_func)();

static MafiaScriptHook* gScriptHook = nullptr;

std::vector<std::string> gRegisteredInstructionNames;
std::vector<std::function<void(std::string, std::vector<std::string>)>> gRegisteredInstructions;

bool gConsoleSetup = false;

void SetupDebugConsole()
{
	if (gConsoleSetup) return;

	AllocConsole();

	SetConsoleTitle("Debug Console");

	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);

	DebugPrintf("Mafia Script Hook version %s (%s)\nDeveloped by Sev3n, inspired by MSE made by ASM\n\n", SCRIPTHOOK_VERSION, __DATE__);

	gConsoleSetup = true;
}

void FreeDebugConsole()
{
	if (!gConsoleSetup) return;

	FreeConsole();

	gConsoleSetup = false;
}

HWND gGameWindow = nullptr;

MafiaScriptHook* MafiaScriptHook::GetScriptHook()
{
	HMODULE ls3df = GetModuleHandle("ls3df.dll");

	if (!ls3df)
		FatalError("LS3DF.dll is not loaded");

	GetEngineVersion_func GetEngineVersion = (GetEngineVersion_func)GetProcAddress(ls3df, "GetEngineVersion");

	if (!GetEngineVersion)
		FatalError("GetEngineFunction method not found in LS3DF.dll");

	int version = GetEngineVersion();

	if (version != 395)
		FatalError("Unsupported game version!\nExpected 395, got %d", version);

#ifdef _DEBUG
	if (!IsDebuggerPresent())
	{
		SetupDebugConsole();
	}
#endif

	if (!gScriptHook)
	{
		gScriptHook = new MafiaScriptHook();

		gScriptHook->RegisterInstruction("mhook_show_version", [](std::string prototype, std::vector<std::string> args) {
			char buf[1024];
			sprintf(buf, "Mafia ScriptHook version %s (%s)\nDeveloped by Sev3n, inspired by MafiaScript Extensions made by ASM", SCRIPTHOOK_VERSION, __DATE__);

			MessageBoxA(NULL, buf, "ScriptHook", MB_OK | MB_ICONINFORMATION);
			});

		gScriptHook->RegisterInstruction("mhook_open_debugcon", [](std::string prototype, std::vector<std::string> args) {
			SetupDebugConsole();
			});

		gScriptHook->RegisterInstruction("mhook_close_debugcon", [](std::string prototype, std::vector<std::string> args) {
			FreeDebugConsole();
			});

		gScriptHook->RegisterInstruction("mhook_debug_log", [](std::string prototype, std::vector<std::string> args) {
			if (args.size() > 0)
				DebugPrintf("%s\n", args[0].c_str());
			});

		gScriptHook->RegisterInstruction("mhook_debug_break", [](std::string prototype, std::vector<std::string> args) {
			if (!gConsoleSetup) SetupDebugConsole();
			DebugPrintf("An debug break point on line %u has been hit.\nPress ENTER to continue execution.", GetCurrentLine());
			SetFocus(GetConsoleWindow());
			SetForegroundWindow(GetConsoleWindow());
			getchar();
			SetFocus(gGameWindow);
			SetForegroundWindow(gGameWindow);
			});

		gScriptHook->RegisterInstruction("mhook_dump_current_script", [](std::string prototype, std::vector<std::string> args) {
			auto script = GetCurrentProgram();

			auto lines = SplitString(std::string(script->m_szSourceCode), "\n");

			DebugPrintf("Complete dump of script 0x%08X\n", script);
			DebugPrintf("================================\n");
			int curLine = 1;
			for (auto line : lines)
			{
				DebugPrintf("[%d] %s", curLine, line.c_str());
				curLine++;
			}
			DebugPrintf("================================\n");
			});

		CProgramHooks::Hook();
	}

	return gScriptHook;
}

bool MafiaScriptHook::RegisterInstruction(const std::string& instructionName, std::function<void(std::string, std::vector<std::string>)> func)
{
	if (instructionName.length() > 0 && !IsInstructionRegistered(instructionName))
	{
		gRegisteredInstructionNames.push_back(instructionName);
		gRegisteredInstructions.push_back(func);

		DebugPrintf("Registered custom instruction: %s\n", instructionName.c_str());

		return true;
	}
	
	return false;
}

int MafiaScriptHook::GetIndexOfInstruction(const std::string& instructionName)
{
	auto it = std::find(gRegisteredInstructionNames.begin(), gRegisteredInstructionNames.end(), instructionName);

	if (it != gRegisteredInstructionNames.end())
	{
		return it - gRegisteredInstructionNames.begin();
	}
	else return -1;
}

bool MafiaScriptHook::IsInstructionRegistered(const std::string& instructionName)
{
	if (std::find(gRegisteredInstructionNames.begin(), gRegisteredInstructionNames.end(), instructionName) != gRegisteredInstructionNames.end())
	{
		return true;
	}

	return false;
}

bool MafiaScriptHook::ExecuteInstruction(const std::string& instructionName, const std::string& proto, std::vector<std::string> args)
{
	if (IsInstructionRegistered(instructionName))
	{
		gRegisteredInstructions[GetIndexOfInstruction(instructionName)](proto, args);

		return true;
	}

	return false;
}

bool MafiaScriptHook::UnregisterInstruction(const std::string& instructionName)
{
	if (IsInstructionRegistered(instructionName))
	{
		int index = GetIndexOfInstruction(instructionName);

		gRegisteredInstructionNames.erase(gRegisteredInstructionNames.begin() + index);
		gRegisteredInstructions.erase(gRegisteredInstructions.begin() + index);

		return true;
	}

	return false;
}