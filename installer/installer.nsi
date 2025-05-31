; WhisperApp NSIS Installer Script
; Copyright (c) 2025 WhisperApp Development Team

;--------------------------------
; Includes

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

;--------------------------------
; General Configuration

!define PRODUCT_NAME "WhisperApp"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "WhisperApp Development Team"
!define PRODUCT_WEB_SITE "https://github.com/yourname/whisperapp"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\WhisperApp.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

; Required executables
!define VCREDIST_X64 "vc_redist.x64.exe"

;--------------------------------
; MUI Settings

!define MUI_ABORTWARNING
!define MUI_ICON "..\resources\icons\app.ico"
!define MUI_UNICON "..\resources\icons\app.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "WhisperApp"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\WhisperApp.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.md"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer attributes

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\build\WhisperApp-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\WhisperApp"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin

;--------------------------------
; Version Information

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2025 ${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"

;--------------------------------
; Functions

Function .onInit
  ; Check if running on 64-bit Windows
  ${If} ${RunningX64}
    SetRegView 64
  ${Else}
    MessageBox MB_OK|MB_ICONEXCLAMATION "WhisperApp requires a 64-bit version of Windows."
    Abort
  ${EndIf}
  
  ; Check for previous installation
  ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
  
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR'
  
done:
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove ${PRODUCT_NAME} and all of its components?" IDYES +2
  Abort
FunctionEnd

;--------------------------------
; Installer Sections

Section "WhisperApp Core" SEC01
  SectionIn RO
  SetOutPath "$INSTDIR"
  
  ; Core application files
  File /r "..\build\release\WhisperApp.exe"
  File /r "..\build\release\*.dll"
  
  ; Qt dependencies
  File /r "..\build\release\platforms"
  File /r "..\build\release\styles"
  File /r "..\build\release\imageformats"
  File /r "..\build\release\iconengines"
  
  ; Whisper model files
  CreateDirectory "$INSTDIR\models"
  File /oname=models\ggml-base.en.bin "..\models\ggml-base.en.bin"
  
  ; Configuration files
  CreateDirectory "$INSTDIR\config"
  File /oname=config\default-config.json "..\config\default-config.json"
  File /oname=config\hotkey-presets.json "..\config\hotkey-presets.json"
  File /oname=config\export-templates.json "..\config\export-templates.json"
  
  ; Resources
  CreateDirectory "$INSTDIR\resources"
  File /r "..\resources\icons"
  File /r "..\resources\sounds"
  File /r "..\resources\styles"
  
  ; Documentation
  File "..\README.md"
  File "..\USER_GUIDE.md"
  File "..\CHANGELOG.md"
  File "..\LICENSE"
  
  ; Translations
  CreateDirectory "$INSTDIR\translations"
  File /r "..\build\release\translations\*.qm"
SectionEnd

Section "Visual C++ Runtime" SEC02
  SetOutPath "$TEMP"
  File "${VCREDIST_X64}"
  ExecWait '"$TEMP\${VCREDIST_X64}" /quiet /norestart'
  Delete "$TEMP\${VCREDIST_X64}"
SectionEnd

Section "File Associations" SEC03
  ; Register .wsp file extension
  WriteRegStr HKCR ".wsp" "" "WhisperApp.Session"
  WriteRegStr HKCR "WhisperApp.Session" "" "WhisperApp Session File"
  WriteRegStr HKCR "WhisperApp.Session\DefaultIcon" "" "$INSTDIR\WhisperApp.exe,0"
  WriteRegStr HKCR "WhisperApp.Session\shell\open\command" "" '"$INSTDIR\WhisperApp.exe" "%1"'
SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortcut "$SMPROGRAMS\$ICONS_GROUP\WhisperApp.lnk" "$INSTDIR\WhisperApp.exe"
  CreateShortcut "$SMPROGRAMS\$ICONS_GROUP\User Guide.lnk" "$INSTDIR\USER_GUIDE.md"
  CreateShortcut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\WhisperApp.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\WhisperApp.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  ; Write install location for auto-updater
  WriteRegStr HKLM "Software\WhisperApp" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "Software\WhisperApp" "Version" "${PRODUCT_VERSION}"
  
  ; Add to Windows Firewall exceptions
  ExecWait 'netsh advfirewall firewall add rule name="WhisperApp" dir=in action=allow program="$INSTDIR\WhisperApp.exe" enable=yes'
SectionEnd

;--------------------------------
; Section Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Core WhisperApp files and dependencies"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Microsoft Visual C++ Runtime required for WhisperApp"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Associate .wsp files with WhisperApp"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section

Section Uninstall
  ; Remove from Windows Firewall exceptions
  ExecWait 'netsh advfirewall firewall delete rule name="WhisperApp"'
  
  ; Remove file associations
  DeleteRegKey HKCR ".wsp"
  DeleteRegKey HKCR "WhisperApp.Session"
  
  ; Remove shortcuts
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\User Guide.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\WhisperApp.lnk"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  
  ; Remove files and directories
  Delete "$INSTDIR\WhisperApp.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\USER_GUIDE.md"
  Delete "$INSTDIR\CHANGELOG.md"
  Delete "$INSTDIR\LICENSE"
  
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\models"
  RMDir /r "$INSTDIR\config"
  RMDir /r "$INSTDIR\resources"
  RMDir /r "$INSTDIR\translations"
  
  ; Remove user data (optional)
  MessageBox MB_YESNO "Do you want to remove all user data and settings?" IDNO skip_userdata
  RMDir /r "$APPDATA\WhisperApp"
  
skip_userdata:
  RMDir "$INSTDIR"
  
  ; Remove registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKLM "Software\WhisperApp"
  
  SetAutoClose true
SectionEnd