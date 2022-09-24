#include "Internal.h"
#include "ScriptHook.h"
#include "MinHook/MinHook.h"
#include <map>

#define INSTRUCTION_BASE_INDEX 397

std::string callInstructionName;
std::vector<std::string> queueCallInstructions;
std::vector<std::string> queueCallSignatures;
std::vector<std::vector<std::string>> queueCallArgs;

typedef C_program*(__thiscall* C_program__C_program_ctor)(C_program*);
typedef void(__thiscall* C_program__C_program_dtor)(C_program*);
typedef int(__thiscall* C_program__DecodeInstruction_func)(C_program*, char*, int);
typedef int(__thiscall* C_program__DecodeParams_func)(C_program*, char*, DWORD);
typedef void(__thiscall* C_program__Process_func)(C_program*, unsigned int);
typedef void(__thiscall* C_program__TryChangeProgramData_func)(C_program*);
typedef int(__thiscall* C_entity__DecodeInstruction_func)(void*, char*, int);
typedef char(__thiscall* C_entity__DecodeParams_func)(void*, char*, DWORD);

C_program__C_program_ctor C_program__Constructor_original = nullptr;
C_program__C_program_dtor C_program__Destructor_original = nullptr;
C_program__DecodeInstruction_func C_program__DecodeInstruction_original = nullptr;
C_program__DecodeParams_func C_program__DecodeParams_original = nullptr;
C_program__Process_func C_program__Process_original = nullptr;
C_program__TryChangeProgramData_func C_program__TryChangeProgramData_original = nullptr;
C_entity__DecodeInstruction_func C_entity__DecodeInstruction_original = nullptr;
C_entity__DecodeParams_func C_entity__DecodeParams_original = nullptr;

std::map<C_program*, uint32_t> compileLines;
std::map<C_program*, uint32_t> execLines;

C_program* __fastcall C_program__Constructor(C_program* _this, DWORD edx)
{
	DebugPrintf("Created new script processor: 0x%08X\n", _this);

	compileLines.insert(std::make_pair(_this, 0));
	execLines.insert(std::make_pair(_this, 0));

	return C_program__Constructor_original(_this);
}

void __fastcall C_program__Destructor(C_program* _this, DWORD edx)
{
	DebugPrintf("Destroyed an script processor: 0x%08X\n", _this);

	compileLines.erase(_this);
	execLines.erase(_this);

	C_program__Destructor_original(_this);
}

int __fastcall C_program__DecodeInstruction(C_program* _this, DWORD edx, char* buf, int unk)
{
	//DebugPrintf("C_program::DecodeInstruction(0x%08X, [%s], %d)\n", _this, buf, unk);

	compileLines[_this]++;

	if (strlen(buf) > 0)
	{
		auto vec = SplitString(buf, " ");

		std::string callPrototype;
		std::vector<std::string> callArgs;

		callInstructionName = vec[0];

		//DebugPrintf("%s\n", buf);

		int ret = C_program__DecodeInstruction_original(_this, buf, unk);

		if (ret == -1)
		{
			if (MafiaScriptHook::GetScriptHook()->IsInstructionRegistered(callInstructionName))
			{
				queueCallInstructions.push_back(callInstructionName);

				return INSTRUCTION_BASE_INDEX + MafiaScriptHook::GetScriptHook()->GetIndexOfInstruction(callInstructionName);
			}

			DebugPrintf("ERROR (line %d): Instruction %s is not defined\n", compileLines[_this], callInstructionName.c_str());
			return -1;
		}

		if (callInstructionName != "{" && callInstructionName != "}") queueCallInstructions.push_back(callInstructionName);

		return ret;
	}

	return -1;
}

int __fastcall C_program__DecodeParams(C_program* _this, DWORD edx, char* buf, DWORD unk)
{
	//DebugPrintf("C_program::DecodeParams(0x%08X, [%s], 0x%08X)\n", _this, buf, unk);

	if (!callInstructionName.empty() && MafiaScriptHook::GetScriptHook()->IsInstructionRegistered(callInstructionName))
	{
		std::string abuf(buf), callPrototype;

		if (abuf.length() > 0)
		{
			auto args = SplitString(abuf, ",");
			auto fixedArgs = SplitString(abuf, ",");

			// Remove quotes from strings
			for (int i = 0; i < args.size(); i++)
			{
				auto arg = args[i];

				fixedArgs[i] = ReplaceAll(arg, "\"", "");
			}

			// Construct prototype
			for (auto arg : args)
			{
				// Floating-point number
				if (arg.find(".") != std::string::npos && arg.find("\"") == std::string::npos) callPrototype += "f";
				else if (arg.find("\"") != std::string::npos) callPrototype += "s";
				else if (IsNumber(arg)) callPrototype += "i";
			}

			// Remove spaces between arguments
			for (auto arg : fixedArgs)
			{
				if (arg[0] == ' ') arg = arg.substr(1);
			}

			queueCallSignatures.push_back(callPrototype);
			queueCallArgs.push_back(fixedArgs);
		}
		else
		{
			queueCallSignatures.push_back("");
			queueCallArgs.push_back(std::vector<std::string>());
		}
		return 1;
	}

	int ret = C_program__DecodeParams_original(_this, buf, unk);

	if (!ret)
	{
		DebugPrintf("ERROR (line %d): Failed to parse parameters for instruction %s\n", compileLines[_this], callInstructionName.c_str());
	}

	return ret;
}

void __fastcall C_program__TryChangeProgramData(C_program* _this, DWORD edx)
{
	char* source = _this->m_szSourceCode;

	//DebugPrintf("C_program::TryChangeProgramData(0x%08X): \n%s\n", _this, source);

	C_program__TryChangeProgramData_original(_this);
}

C_program* gCurrentProgram = nullptr;

uint32_t GetCurrentLine() 
{
	if (!gCurrentProgram) return 0;

	return execLines[gCurrentProgram];
}

C_program* GetCurrentProgram() { return gCurrentProgram; }

void __fastcall C_program__Process(C_program* _this, DWORD edx, unsigned int unk)
{
	//DebugPrintf("C_program::Process(0x%08X, %u)\n", _this, unk);

	gCurrentProgram = _this;

	execLines[_this]++;

	if (!queueCallInstructions.empty())
	{
		if (MafiaScriptHook::GetScriptHook()->IsInstructionRegistered(queueCallInstructions.front()))
		{
			DebugPrintf("Executing custom instruction: %s\n", queueCallInstructions.front().c_str());
			MafiaScriptHook::GetScriptHook()->ExecuteInstruction(queueCallInstructions.front(), queueCallSignatures.front(), queueCallArgs.front());
			queueCallInstructions.erase(queueCallInstructions.begin());
			if (!queueCallSignatures.empty()) queueCallSignatures.erase(queueCallSignatures.begin());
			if (!queueCallArgs.empty()) queueCallArgs.erase(queueCallArgs.begin());
			return;
		}
		else
		{
			DebugPrintf("Executing built-in instruction: %s\n", queueCallInstructions.front().c_str());
			queueCallInstructions.erase(queueCallInstructions.begin());
		}
	}

	C_program__Process_original(_this, unk);
}

int __fastcall C_entity__DecodeInstruction(void* _this, DWORD edx, char* buf, int unk)
{
	//DebugPrintf("C_entity::DecodeInstruction(0x%08X, [%s], %d)\n", _this, buf, unk);

	return C_entity__DecodeInstruction_original(_this, buf, unk);
}

int __fastcall C_entity__DecodeParams(void* _this, DWORD edx, char* buf, DWORD unk)
{
	//DebugPrintf("C_entity::DecodeParams(0x%08X, [%s], 0x%08X)\n", _this, buf, unk);

	return C_entity__DecodeParams_original(_this, buf, unk);
}

typedef HWND(__stdcall* CreateWindowExA_proc)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

CreateWindowExA_proc CreateWindowExA_original = nullptr;

HWND __stdcall CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	gGameWindow = CreateWindowExA_original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	return gGameWindow;
}

void CProgramHooks::Hook()
{
	if (MH_Initialize() != MH_OK) {
		FatalError("Failed to initialize the MinHook library!");
	}

	MH_CreateHookApi(L"user32", "CreateWindowExA", &CreateWindowExA_Hook, (LPVOID*)&CreateWindowExA_original);

	MH_CreateHook((LPVOID)0x460AD0, &C_program__Constructor, (LPVOID*)&C_program__Constructor_original);
	MH_CreateHook((LPVOID)0x460B40, &C_program__Destructor, (LPVOID*)&C_program__Destructor_original);
	MH_CreateHook((LPVOID)0x463960, &C_program__DecodeInstruction, (LPVOID*)&C_program__DecodeInstruction_original);
	MH_CreateHook((LPVOID)0x461C70, &C_program__DecodeParams, (LPVOID*)&C_program__DecodeParams_original);
	MH_CreateHook((LPVOID)0x46D3B0, &C_program__Process, (LPVOID*)&C_program__Process_original);
	MH_CreateHook((LPVOID)0x461420, &C_program__TryChangeProgramData, (LPVOID*)&C_program__TryChangeProgramData_original);
	MH_CreateHook((LPVOID)0x51CDD0, &C_entity__DecodeInstruction, (LPVOID*)&C_entity__DecodeInstruction_original);
	MH_CreateHook((LPVOID)0x51B470, &C_entity__DecodeParams, (LPVOID*)&C_entity__DecodeParams_original);

	MH_EnableHook(MH_ALL_HOOKS);
}