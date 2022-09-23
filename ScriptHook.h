#pragma once

#include <string>
#include <vector>
#include <functional>

#define SCRIPTHOOK_VERSION "0.1.0-dev"

class MafiaScriptHook
{
public:
	static MafiaScriptHook* GetScriptHook();

	bool RegisterInstruction(const std::string& instructionName, std::function<void(std::string, std::vector<std::string>)> func);

	int GetIndexOfInstruction(const std::string& instructionName);

	bool IsInstructionRegistered(const std::string& instructionName);

	bool ExecuteInstruction(const std::string& instructionName, const std::string& proto, std::vector<std::string> args);

	bool UnregisterInstruction(const std::string& instructionName);
};