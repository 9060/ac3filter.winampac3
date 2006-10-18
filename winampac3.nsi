;
; WinampAC3 installation script
;

OutFile "${SETUP_FILE}"
Name "WinampAC3"
CRCCheck on

;Icon "ac3filter.ico"
;UninstallIcon "ac3filter.ico"

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

InstallColors {000000 C0C0C0}
InstProgressFlags "smooth"
ShowInstDetails "show"


Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles


Function .onVerifyInstDir
  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:
FunctionEnd


Section "WinampAC3"
  SetOutPath $INSTDIR\plugins

  ;; Delete old plugin version
  Delete "$INSTDIR\plugins\in_vac3.dll"

  ;; Copy Files
  File "${SOURCE_DIR}\in_winampac3.dll"

  ;; Install presets
  ExecWait 'regedit /s "${SOURCE_DIR}\winampac3_presets.reg"'

SectionEnd

Section "Uninstall"
  DeleteRegKey   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinampAC3"
  DeleteRegKey   HKCU "Software\WinampAC3"
  Delete "$INSTDIR\plugins\in_vac3.dll"
  Delete "$INSTDIR\plugins\in_winampac3.dll"
SectionEnd
