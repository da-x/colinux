;Cooeprative Linux installer
;Written by NEBOR Regis
;Modified by Dan Aloni (c) 2004

;-------------------------------------
;Good look
  !include "MUI.nsh"
  !include Sections.nsh
  !include "coLinux_def.inc"
  !define PUBLISHER "www.colinux.org"

  ;General
  Name "Cooperative Linux ${VERSION}"
  OutFile "coLinux.exe"

  ShowInstDetails show
  ShowUninstDetails show

  ;Folder selection page
  InstallDir "$PROGRAMFILES\coLinux"
  
  ;Get install folder from registry if available
  InstallDirRegKey HKCU "Software\coLinux" ""

  BrandingText "${PUBLISHER}"

  VIAddVersionKey ProductName "coLinux"
  VIAddVersionKey CompanyName "${PUBLISHER}"
  VIAddVersionKey ProductVersion "${VERSION}"
  VIAddVersionKey FileVersion "${VERSION}"
  VIAddVersionKey FileDescription "An optimized virtual Linux system for Windows"
  VIAddVersionKey LegalCopyright "Copyright @ 2004 ${PUBLISHER}"
  VIProductVersion "${LONGVERSION}"

  XPStyle on

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "header.bmp"
  !define MUI_SPECIALBITMAP "startlogo.bmp"

  !define MUI_COMPONENTSPAGE_SMALLDESC

  !define MUI_WELCOMEPAGE_TITLE "Welcome to the coLinux ${VERSION} Setup Wizard"
  !define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of Cooperative Linux ${VERSION}\r\n\r\n$_CLICK"

  !insertmacro MUI_PAGE_WELCOME

  !insertmacro MUI_PAGE_LICENSE "..\..\..\..\..\..\COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_LINK "Visit the Cooperative Linux website"
  !define MUI_FINISHPAGE_LINK_LOCATION "http://www.colinux.org/"
  
  !insertmacro MUI_PAGE_FINISH
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;------------------------------------------------------------------------
;------------------------------------------------------------------------
;Installer Sections


Section "coLinux" SeccoLinux

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "DisplayName" "coLinux"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "UninstallString" '"$INSTDIR\Uninstall.exe"'


  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "..\console\coLinux-console-fltk.exe"
  File "..\console-nt\coLinux-console-nt.exe"
  File "..\conet-daemon\coLinux-net-daemon.exe"
  File "..\daemon\coLinux-daemon.exe"
  File "..\..\build\linux.sys"
  File "premaid\vmlinux"
  File "premaid\vmlinux-modules.tar.gz"
  File "premaid\README"
  File "..\..\..\..\..\..\conf\default.coLinux.xml"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

  ;Store install folder
  WriteRegStr HKCU "Software\coLinux" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

Section "coLinux Virtual Ethernet Driver (TAP-Win32)" SeccoLinuxNet

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR\netdriver"
  File "premaid\netdriver\OemWin2k.inf"
  File "premaid\netdriver\tapdrvr.sys"
  File "premaid\netdriver\devcon.exe"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------


SectionEnd

Section "coLinux Bridged Ethernet (WinPcap)" SeccoLinuxBridgedNet

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "premaid\packet.dll"
  File "premaid\wpcap.dll"
  File "..\conet-bridged-daemon\coLinux-bridged-net-daemon.exe"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

SectionEnd



Section "Cygwin DLL" SecCygwinDll

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "premaid\cygwin1.dll"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

SectionEnd


;--------------------
;Post-install section

Section -post

;---- Directly from OpenVPN install script , some minor mods

  SectionGetFlags ${SeccoLinuxNet} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} "" notap notap
    ; TAP install/update was selected.
    ; Should we install or update?
    ; If devcon error occurred, $5 will
    ; be nonzero.
    IntOp $5 0 & 0
    nsExec::ExecToStack '"$INSTDIR\netdriver\devcon.exe" hwids TAP'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "devcon hwids returned: $R0"

    ; If devcon output string contains "TAP" we assume
    ; that TAP device has been previously installed,
    ; therefore we will update, not install.
    Push "TAP"
    Call StrStr
    Pop $R0

    IntCmp $5 0 +1 devcon_check_error devcon_check_error
    IntCmp $R0 -1 tapinstall

 ;tapupdate:
    DetailPrint "TAP-Win32 UPDATE"
    nsExec::ExecToLog '"$INSTDIR\netdriver\devcon.exe" update "$INSTDIR\netdriver\OemWin2k.inf" TAP'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "devcon update returned: $R0"
    Goto devcon_check_error

 tapinstall:
    DetailPrint "TAP-Win32 INSTALL"
    nsExec::ExecToLog '"$INSTDIR\netdriver\devcon.exe" install "$INSTDIR\netdriver\OemWin2k.inf" TAP'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "devcon install returned: $R0"

 devcon_check_error:
    IntCmp $5 +1 notap
    MessageBox MB_OK "An error occurred installing the TAP-Win32 device driver."

 notap:




SectionEnd

;--------------------------------
;Descriptions

  LangString DESC_SeccoLinux ${LANG_ENGLISH} "Install coLinux"
  LangString DESC_SeccoLinuxNet ${LANG_ENGLISH} "Install coLinux Virtual Ethernet Driver, which allows to create a network link between Linux and Windows"
  LangString DESC_SeccoLinuxBridgedNet ${LANG_ENGLISH} "Install coLinux Bridge Ethernet support, which allows to join the coLinux machine to an existing network"
  LangString DESC_SecCygwinDll ${LANG_ENGLISH} "Install the Cygwin DLL (only needed if you don't already have Cygwin installed, see www.cygwin.com)"

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinux} $(DESC_SeccoLinux)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxNet} $(DESC_SeccoLinuxNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxBridgedNet} $(DESC_SeccoLinuxBridgedNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCygwinDll} $(DESC_SecCygwinDll)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section
;--------------------------------


Section "Uninstall"

  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "DisplayName"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "UninstallString"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux"

  DetailPrint "TAP-Win32 REMOVE"
  nsExec::ExecToLog '"$INSTDIR\netdriver\devcon.exe" remove TAP'
  Pop $R0 # return value/error/timeout
  DetailPrint "devcon remove returned: $R0"


  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ;

  Delete "$INSTDIR\coLinux-console-fltk.exe"
  Delete "$INSTDIR\coLinux-console-nt.exe"
  Delete "$INSTDIR\coLinux-daemon.exe"
  Delete "$INSTDIR\coLinux-net-daemon.exe"
  Delete "$INSTDIR\coLinux-bridged-net-daemon.exe"
  Delete "$INSTDIR\cygwin1.dll"
  Delete "$INSTDIR\packet.dll"
  Delete "$INSTDIR\wpcap.dll"
  Delete "$INSTDIR\linux.sys"
  Delete "$INSTDIR\README"
  Delete "$INSTDIR\vmlinux"
  Delete "$INSTDIR\vmlinux-modules.tar.gz"
  Delete "$INSTDIR\default.coLinux.xml"

  Delete "$INSTDIR\netdriver\OemWin2k.inf"
  Delete "$INSTDIR\netdriver\tapdrvr.sys"
  Delete "$INSTDIR\netdriver\devcon.exe"
  
  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------


  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR\netdriver"
  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\coLinux"

SectionEnd


; TODO coLinux runtime detection, to kill before install or upgrade

; LEGACY

;====================================================
; StrStr - Finds a given string in another given string.
;               Returns -1 if not found and the pos if found.
;          Input: head of the stack - string to find
;                      second in the stack - string to find in
;          Output: head of the stack
;====================================================
Function StrStr
  Push $0
  Exch
  Pop $0 ; $0 now have the string to find
  Push $1
  Exch 2
  Pop $1 ; $1 now have the string to find in
  Exch
  Push $2
  Push $3
  Push $4
  Push $5

  StrCpy $2 -1
  StrLen $3 $0
  StrLen $4 $1
  IntOp $4 $4 - $3

  StrStr_loop:
    IntOp $2 $2 + 1
    IntCmp $2 $4 0 0 StrStrReturn_notFound
    StrCpy $5 $1 $3 $2
    StrCmp $5 $0 StrStr_done StrStr_loop

  StrStrReturn_notFound:
    StrCpy $2 -1

  StrStr_done:
    Pop $5
    Pop $4
    Pop $3
    Exch $2
    Exch 2
    Pop $0
    Pop $1
FunctionEnd