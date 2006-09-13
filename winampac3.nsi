;
; WinampAC3 installation script
;


OutFile "${SETUP_FILE}"
Name "WinampAC3"
;Icon "ac3filter.ico"
CRCCheck on

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

DirText "Please select your Winamp path below (you will be able to proceed when Winamp is detected):"
DirShow show

InstallColors {000000 C0C0C0}
InstProgressFlags "smooth"
AutoCloseWindow true
ShowInstDetails nevershow

Function .onVerifyInstDir
  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:
FunctionEnd


Section "Install"
  SetOutPath $INSTDIR\plugins

  ;; Copy Files
  File "${SOURCE_DIR}\in_vac3.dll"

SectionEnd
