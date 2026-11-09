@echo off
rem Builds CrowdTune.asi (SLAM mod) with Visual Studio. Run from the "x86 Native Tools Command Prompt for VS".
cd /d "%~dp0.."
if not exist bin mkdir bin
cl /nologo /O2 /GS- /Gs999999 /Zl /W3 /Islam  /DMOD_NAME=\"CrowdTune\" /DMOD_VERSION=\"1.0\" /LD src\crowdtune.c slam\slam_glue.c /Fobin\ /Fe:bin\CrowdTune.asi /link /nodefaultlib /entry:DllMain /noimplib kernel32.lib user32.lib || exit /b 1
del /q bin\*.obj bin\*.exp 2>nul
echo Built bin\CrowdTune.asi
