# Mafia ScriptHook

Mafia ScriptHook is an static library inspired by [MafiaScript Extensions](http://mafiascene.com/forum/viewtopic.php?t=10146), which can be used to add more instructions into the MafiaScript language.



## How to use?

1. Download an development package [here](https://github.com/Sevenisko/MScriptHook/releases).

2. Create an new **C++ DLL** *(shared library)* project, setup the include and library paths and add MScriptHook.lib into imported libraries list and *(if possible)* change the output extension to **.asi**

3. Implement the ScriptHook, here's an example code:

4. ```cpp
   #include <ScriptHook.h>
   #include <Windows.h>
   
   extern "C" __declspec(dllexport) void InitializeASI()
   {
       MafiaScriptHook::GetScriptHook()->RegisterInstruction("msgbox_test", [&](std::string prototype, std::vector<std::string> args) {
        if (args.size() > 0)
        {
            MessageBoxA(NULL, args[0].c_str(), "MScriptHook Test", MB_OK | MB_ICONINFORMATION);
        }
        });
   }
   ```

## How to build?

- **In order to build this project, Visual Studio 2022 must be installed**
1. Clone this repository

2. Open the solution file

3. Choose the build configuration and Build the solution

## Additional features

Of course it can add custom instructions into the script, but it also has few more features to it:

- Debug console

- Error reporting for non-existing instructions or inproperly parsed parameters

- Debug utilities (log, break)

- Ability to completely dump a script

- Execute any custom script (from both string and file)

## Internal instructions

- **mhook_show_version** - *Shows a message box displaying the current version and build date*

- **mhook_open_debugcon** - Opens an external console window for script debugging

- **mhook_close_debugcon** - Closes an external console window

- **mhook_debug_log *message*** - Writes an *message* into the debug console

- **mhook_debug_break** - Suspends the overall execution here and waits for *ENTER* to be pressed

- **mhook_dump_current_script** - Dumps an entire script loaded in the current scripting instance into the Debug console

## 3rd-Party

This project is mostly based on idea of MafiaScript Extensions made by ASM and is using [MinHook](https://github.com/TsudaKageyu/minhook) for hooking the required functions.


