@echo off
setlocal enabledelayedexpansion

 call :isAdmin

 if %errorlevel% == 0 (
    goto :run
 ) else (
    echo Requesting admin privileges...
    goto :UACPrompt
 )

 exit /b

 :isAdmin
    fsutil dirty query %systemdrive% >nul
 exit /b

 :run
    setlocal
    
   REM Prompt the user to provide the volume letter
   if "%~1"=="" (
      echo.
      echo Please provide the EFI partition volume letter ^(e.g., F:^)
      set /p volume="Enter the volume letter: "
      if "!volume!"=="" (
         echo Error: No volume specified.
         exit /b 1
      )
      REM Ensure the volume ends with ':'
      if not "!volume:~-1!"==":" set volume=!volume!:
   ) else (
      set volume=%~1
      REM Ensure the volume ends with ':'
      if not "!volume:~-1!"==":" set volume=!volume!:
   )
    
   set boot_directory=!volume!\EFI\Microsoft\Boot\
   if not exist !boot_directory! (
      echo The specified volume or boot directory does not exist.
      echo Please ensure the volume letter is correct and the EFI partition is mounted.
      echo Specified volume: !volume!
      echo Boot directory: !boot_directory!
      exit /b 1
   )

   echo Preparing to replace bootmgfw.efi with uefi-boot.efi...
   attrib -s !boot_directory!bootmgfw.efi

   echo Backing up original bootmgfw.efi to bootmgfw.original.efi
   move !boot_directory!bootmgfw.efi !boot_directory!bootmgfw.original.efi

   echo Copying uefi-boot.efi to bootmgfw.efi and hyperv-attachment.dll to boot directory...
   copy /Y %~dp0uefi-boot.efi !boot_directory!bootmgfw.efi
   copy /Y %~dp0hyperv-attachment.dll !boot_directory!

   bcdedit /set hypervisorlaunchtype auto
   endlocal

   echo Restart the computer and apply changes?
   choice /c YN /n /m "Press Y to restart, N to cancel: "
   if errorlevel 2 (
     echo Changes will not be applied.
   ) else if errorlevel 1 (
     shutdown /r /t 0
   )
 exit /b

 :UACPrompt
   echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
   echo UAC.ShellExecute "cmd.exe", "/c %~s0 %~1", "", "runas", 1 >> "%temp%\getadmin.vbs"

   "%temp%\getadmin.vbs"
   del "%temp%\getadmin.vbs"
  exit /B