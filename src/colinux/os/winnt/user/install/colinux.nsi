;Cooperative Linux installer
;Written by NEBOR Regis
;Modified by Dan Aloni (c) 2004
;Modified 8/20/2004,2/4/2004 by George P Boutwell
;Modified 4/20/2008 by Henry Nestler

;-------------------------------------

  !include "MUI.nsh"
  !include "x64.nsh"
  !include Sections.nsh
  !define ALL_USERS
  !include WriteEnvStr.nsh
  !include "coLinux_def.inc"
  !define PUBLISHER "www.colinux.org"

  ;General
  Name "Cooperative Linux ${VERSION}"
  OutFile "coLinux.exe"

  ;ShowInstDetails show
  ;ShowUninstDetails show

  ;Folder selection page
  InstallDir "$PROGRAMFILES\coLinux"

  ;Get install folder from registry if available
  InstallDirRegKey HKCU "Software\coLinux" ""

  BrandingText "${PUBLISHER}"

  VIAddVersionKey ProductName "Cooperative Linux"
  VIAddVersionKey CompanyName "${PUBLISHER}"
  VIAddVersionKey ProductVersion "${VERSION}"
  VIAddVersionKey FileVersion "${VERSION}"
  VIAddVersionKey FileDescription "An optimized virtual Linux system for Windows"
  VIAddVersionKey LegalCopyright "Copyright @ 2004-2007 ${PUBLISHER}"
  VIProductVersion "${LONGVERSION}"

  XPStyle on

; For priority Unpack
  ReserveFile "iDl.ini"
  ReserveFile "WinpcapRedir.ini"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_FINISHPAGE_NOAUTOCLOSE

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
  Page custom WinpcapRedir WinpcapRedirLeave
  Page custom StartDlImageFunc EndDlImageFunc
  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_LINK "Visit the Cooperative Linux website"
  !define MUI_FINISHPAGE_LINK_LOCATION "http://www.colinux.org/"
  !define MUI_FINISHPAGE_SHOWREADME "README.TXT"

  !insertmacro MUI_PAGE_FINISH
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"


;------------------------------------------------------------------------
;------------------------------------------------------------------------
;Custom Setup for image download

;Variables used to track user choice
  Var LOCATION
  Var RandomSeed

; StartDlImageFunc down for reference resolution

Function EndDlImageFunc
FunctionEnd

Function WinpcapRedirLeave
FunctionEnd

Function .onInit

  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "iDl.ini"
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "WinpcapRedir.ini"

FunctionEnd

;------------------------------------------------------------------------
;------------------------------------------------------------------------
;Installer Sections

SectionGroup "coLinux" SecGrpcoLinux

Section

  ;Check running 64 bit and block it (NSIS 2.21 and newer)
  ${If} ${RunningX64}
    MessageBox MB_OK "coLinux can't run on x64"
    DetailPrint "Abort on 64 bit"
    Abort
  ${EndIf}

  ;-------------------------------------------Uninstall with old driver--
  ;----------------------------------------------------------------------

  ;get the old install folder
  ReadRegStr $R2 HKCU "Software\coLinux" ""
  StrCmp $R2 "" no_old_linux_sys

  ;path without ""
  StrCpy $R1 $R2 1
  StrCmp $R1 '"' 0 +2
    StrCpy $R2 $R2 -1 1

  ;Check old daemon for removing driver
  IfFileExists "$R2\colinux-daemon.exe" 0 no_old_linux_sys

  ;Runs any monitor?
check_running_monitors:
  DetailPrint "Check running monitors"
  nsExec::ExecToStack '"$R2\colinux-daemon.exe" --status-driver'
  Pop $R0 # return value/error/timeout
  Pop $R1 # Log

  ;Remove, if no monitor is running
  Push $R1
  Push "current number of monitors: 0"
  Call StrStr
  Pop $R0
  Pop $R1
  IntCmp $R0 -1 0 remove_linux_sys remove_linux_sys

  ;remove anyway, if no can detect running status
  Push $R1
  Push "current number of monitors:"
  Call StrStr
  Pop $R0
  Pop $R1
  IntCmp $R0 -1 remove_linux_sys

  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION \
             "Any coLinux is running.$\nPlease stop it, before continue" \
             IDRETRY check_running_monitors
  DetailPrint "Abort"
  Abort

remove_linux_sys:
  DetailPrint "Uninstall old linux driver"
  nsExec::ExecToLog '"$R2\colinux-daemon.exe" --remove-driver'
  Pop $R0 # return value/error/timeout

no_old_linux_sys:

  ;------------------------------------------------------------REGISTRY--
  ;----------------------------------------------------------------------

  !define REGUNINSTAL "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux"
  !define REGEVENTS "SYSTEM\CurrentControlSet\Services\Eventlog\Application\coLinux"

  WriteRegStr HKLM ${REGUNINSTAL} "DisplayName" "coLinux ${VERSION}"
  WriteRegStr HKLM ${REGUNINSTAL} "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM ${REGUNINSTAL} "DisplayIcon" "$INSTDIR\colinux-daemon.exe,0"
  WriteRegDWORD HKLM ${REGUNINSTAL} "NoModify" "1"
  WriteRegDWORD HKLM ${REGUNINSTAL} "NoRepair" "1"

  ; plain text in event log view
  WriteRegStr HKLM ${REGEVENTS} "EventMessageFile" "$INSTDIR\colinux-daemon.exe"
  WriteRegDWORD HKLM ${REGEVENTS} "TypesSupported" "4" ;EVENTLOG_INFORMATION_TYPE

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "premaid\coLinux-daemon.exe"
  File "premaid\linux.sys"
  File "premaid\README.txt"
  File "premaid\news.txt"
  File "premaid\cofs.txt"
  File "premaid\colinux-daemon.txt"
  File "premaid\vmlinux"
  File "premaid\vmlinux-modules.tar.gz"
  ; initrd installs modules vmlinux-modules.tar.gz over cofs31 on first start
  File "premaid\initrd.gz"

  ;Backup config file if present
  IfFileExists "$INSTDIR\example.conf" 0 +2
    CopyFiles /SILENT "$INSTDIR\example.conf" "$INSTDIR\example.conf.old"
  File "premaid\example.conf"

  ; Remove kludge from older installations
  Delete "$INSTDIR\packet.dll"
  Delete "$INSTDIR\wpcap.dll"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

  ;Store install folder
  WriteRegStr HKCU "Software\coLinux" "" "$INSTDIR"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

Section "Native Windows Linux Console (NT)" SecNTConsole
  File "premaid\coLinux-console-nt.exe"
SectionEnd

Section "Cross-platform Linux Console (FLTK)" SecFLTKConsole
  File "premaid\coLinux-console-fltk.exe"
SectionEnd

Section "Virtual Ethernet Driver (coLinux TAP-Win32)" SeccoLinuxNet
  SetOutPath "$INSTDIR\netdriver"
  File "premaid\netdriver\OemWin2k.inf"
  File "premaid\netdriver\tap0801co.sys"
  File "premaid\netdriver\tapcontrol.exe"
  File "premaid\netdriver\tap.cat"

  SetOutPath "$INSTDIR"
  File "premaid\coLinux-net-daemon.exe"
SectionEnd

Section "Virtual Network Daemon (SLiRP)" SeccoLinuxNetSLiRP
  File "premaid\coLinux-slirp-net-daemon.exe"
SectionEnd

Section "Bridged Ethernet (WinPcap)" SeccoLinuxBridgedNet
  File "premaid\coLinux-bridged-net-daemon.exe"
SectionEnd

Section "Kernel Bridged Ethernet (ndis)" SeccoLinuxNdisBridge
  File "premaid\colinux-ndis-net-daemon.exe"
SectionEnd

Section "Virtual Serial Device (ttyS)" SeccoLinuxSerial
  File "premaid\coLinux-serial-daemon.exe"
SectionEnd

Section "Debugging" SeccoLinuxDebug
  File "premaid\coLinux-debug-daemon.exe"
  File "premaid\debugging.txt"
SectionEnd

SectionGroupEnd

# Defines must sync with entries in file iDl.ini
!define IDL_NOTHING 1
!define IDL_ARCHLINUX 2
!define IDL_DEBIAN 3
!define IDL_FEDORA 4
!define IDL_GENTOO 5
!define IDL_UBUNTU 6
!define IDL_LOCATION 7

Section "Root Filesystem image Download" SeccoLinuxImage

   ;----------------------------------------------------------
   ; Random sourceforge download
    ;Read a value from an InstallOptions INI file and set the filenames

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_NOTHING}" "State"
    StrCmp $R1 "1" End

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_ARCHLINUX}" "State"
    StrCpy $R0 "ArchLinux-2007.08-2-ext3-256m.7z"
    StrCmp $R1 "1" tryDownload

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_DEBIAN}" "State"
    StrCpy $R0 "Debian-4.0r0-etch.ext3.1gb.bz2"
    StrCmp $R1 "1" tryDownload

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_FEDORA}" "State"
    StrCpy $R0 "Fedora-10-20090228.exe"
    StrCmp $R1 "1" tryDownload

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_GENTOO}" "State"
    StrCpy $R0 "Gentoo-colinux-i686-2007-03-03.7z"
    StrCmp $R1 "1" tryDownload

    !insertmacro MUI_INSTALLOPTIONS_READ $R1 "iDl.ini" "Field ${IDL_UBUNTU}" "State"
    StrCpy $R0 "Ubuntu-7.10.ext3.2GB.7z"
    StrCmp $R1 "1" tryDownload
    GoTo End

    tryDownload:
    StrCpy $R1 "colinux" ; project

    !insertmacro MUI_INSTALLOPTIONS_READ $LOCATION "iDl.ini" "Field ${IDL_LOCATION}" "State"
    StrCmp $LOCATION "SourceForge defaults" SFdefault  ;This is the preferred way and counts the stats
    StrCmp $LOCATION "Asia" Asia
    StrCmp $LOCATION "Australia" Australia
    StrCmp $LOCATION "Europe" Europe
    StrCmp $LOCATION "North America" NorthAmerica
    StrCmp $LOCATION "South America" SouthAmerica

    ;Random:
    ;MessageBox MB_OK "Random"
    	Push "http://belnet.dl.sourceforge.net/sourceforge/$R1/$R0"	;Europe
	Push "http://heanet.dl.sourceforge.net/sourceforge/$R1/$R0"	;Europe
	Push "http://easynews.dl.sourceforge.net/sourceforge/$R1/$R0"	;NorthAmerica
	Push "http://twtelecom.dl.sourceforge.net/sourceforge/$R1/$R0"	;?? NorthAmerika
	Push "http://flow.dl.sourceforge.net/sourceforge/$R1/$R0"	;??
	Push "http://aleron.dl.sourceforge.net/sourceforge/$R1/$R0"	;?? NorthAmerika
	Push "http://umn.dl.sourceforge.net/sourceforge/$R1/$R0"	;NorthAmerika
        Push "http://jaist.dl.sourceforge.net/sourceforge/$R1/$R0"	;Asia
        Push "http://optusnet.dl.sourceforge.net/sourceforge/$R1/$R0"	;Australia
	Push "http://mesh.dl.sourceforge.net/sourceforge/$R1/$R0"	;Europe
	Push 10
    Goto DownloadRandom
    Asia:
    ;MessageBox MB_OK "Asia"
        Push "http://jaist.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://nchc.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://keihanna.dl.sourceforge.net/sourceforge/$R1/$R0"  ;??
        Push 3
    Goto DownloadRandom
    Australia:
        Push "http://optusnet.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push 1
    Goto DownloadRandom
    Europe:
    ;MessageBox MB_OK "Europe"
        Push "http://belnet.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://puzzle.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://switch.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://mesh.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://ohv.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://heanet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://surfnet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://kent.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://cesnet.dl.sourceforge.net/sourceforge/$R1/$R0"  ;??
	Push 9
    Goto DownloadRandom
    NorthAmerica:
    ;MessageBox MB_OK "NorthAmerica"
        Push "http://easynews.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://superb-west.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://superb-east.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://umn.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://twtelecom.dl.sourceforge.net/sourceforge/$R1/$R0"  ;??
        Push "http://aleron.dl.sourceforge.net/sourceforge/$R1/$R0"  ;??
        Push "http://unc.dl.sourceforge.net/sourceforge/$R1/$R0"  ;??
        Push 7
    Goto DownloadRandom
    SouthAmerica:
        Push "http://ufpr.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push 1
    Goto DownloadRandom
    SFdefault:
        Push "http://downloads.sourceforge.net/$R1/$R0?download"
        Push 1

    DownloadRandom:
    	Push "$INSTDIR\$R0"
	Call DownloadFromRandomMirror
	Pop $0

    End:
    
SectionEnd

Function StartDlImageFunc

  SectionGetFlags ${SeccoLinuxImage} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} "" ImageEnd ImageEnd

  !insertmacro MUI_HEADER_TEXT "Obtain a coLinux root file system image" "Choose a location"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "iDl.ini"

  ImageEnd:

FunctionEnd

Function WinpcapRedir
  SectionGetFlags ${SeccoLinuxBridgedNet} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} "" WinpcapEnd WinpcapEnd
  !insertmacro MUI_HEADER_TEXT "Get WinPCAP" "Install Bridged Ethernet WinPCAP dependency"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "WinpcapRedir.ini"
  WinpcapEnd:
FunctionEnd

;--------------------
;Post-install section

Section -post

;---- Directly from OpenVPN install script , some minor mods

  SectionGetFlags ${SeccoLinuxNet} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} "" notap notap
    ; TAP install/update was selected.
    ; Should we install or update?
    ; If tapcontrol error occurred, $5 will
    ; be nonzero.
    IntOp $5 0 & 0
    nsExec::ExecToStack '"$INSTDIR\netdriver\tapcontrol.exe" hwids TAP0801co'
    Pop $R0 # return value/error/timeout
            # leave Log in stack
    IntOp $5 $5 | $R0
    DetailPrint "tapcontrol hwids returned: $R0"

    ; If tapcontrol output string contains "TAP" we assume
    ; that TAP device has been previously installed,
    ; therefore we will update, not install.
    Push "TAP"
    Call StrStr
    Pop $R0

    IntCmp $5 0 +1 tapcontrol_check_error tapcontrol_check_error
    IntCmp $R0 -1 tapinstall

 ;tapupdate:
    DetailPrint "TAP-Win32 UPDATE (please confirm Windows-Logo-Test)"
    nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" update \
                       "$INSTDIR\netdriver\OemWin2k.inf" TAP0801co'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "tapcontrol update returned: $R0"
    Goto tapcontrol_check_error

 tapinstall:
    DetailPrint "TAP-Win32 INSTALL (please confirm Windows-Logo-Test)"
    nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" install \
		       "$INSTDIR\netdriver\OemWin2k.inf" TAP0801co'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "tapcontrol install returned: $R0"

 tapcontrol_check_error:
    IntCmp $5 +1 notap
    MessageBox MB_OK "An error occurred installing the TAP-Win32 device driver."

 notap:

SectionEnd

;--------------------
;Post-install section

Section -post
    ; Store CoLinux installation path into evironment variable
    Push COLINUX
    Push $INSTDIR
    Call WriteEnvStr

    nsExec::ExecToStack '"$INSTDIR\colinux-daemon.exe" --remove-driver'
    Pop $R0 # return value/error/timeout
    Pop $R1 # Log
    nsExec::ExecToLog '"$INSTDIR\colinux-daemon.exe" --install-driver'
    Pop $R0 # return value/error/timeout
SectionEnd


;--------------------------------
;Descriptions

  LangString DESC_SecGrpcoLinux ${LANG_ENGLISH} "Install or upgrade coLinux. Install coLinux basics, driver, Linux kernel, Linux modules and docs"
  LangString DESC_SecNTConsole ${LANG_ENGLISH} "Native Windows (NT) coLinux console views Linux console in an NT DOS Command-line console"
  LangString DESC_SecFLTKConsole ${LANG_ENGLISH} "FLTK Cross-platform coLinux console to view Linux console and manage coLinux from a GUI program"
  LangString DESC_SeccoLinuxNet ${LANG_ENGLISH} "TAP Virtual Ethernet Driver as private network link between Linux and Windows"
  LangString DESC_SeccoLinuxNetSLiRP ${LANG_ENGLISH} "SLiRP Ethernet Driver as virtual Gateway for outgoings TCP and UDP connections - Simplest to use"
  LangString DESC_SeccoLinuxBridgedNet ${LANG_ENGLISH} "Bridge Ethernet support allows to join the coLinux machine to an existing network via libPcap"
  LangString DESC_SeccoLinuxNdisBridge ${LANG_ENGLISH} "Kernel Bridge Ethernet connects coLinux to an existing network via a fast kernel ndis driver."
  LangString DESC_SeccoLinuxSerial ${LANG_ENGLISH} "Virtual Serial Driver allows to use serial Devices between Linux and Windows"
  LangString DESC_SeccoLinuxDebug ${LANG_ENGLISH} "Debugging allows to create extensive debug log for troubleshooting problems"
  LangString DESC_SecImage ${LANG_ENGLISH} "Download an image from sourceforge. Also provide useful links on how to use it"

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGrpcoLinux} $(DESC_SecGrpcoLinux)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecNTConsole} $(DESC_SecNTConsole)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecFLTKConsole} $(DESC_SecFLTKConsole)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxNet} $(DESC_SeccoLinuxNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxNetSLiRP} $(DESC_SeccoLinuxNetSLiRP)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxBridgedNet} $(DESC_SeccoLinuxBridgedNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxNdisBridge} $(DESC_SeccoLinuxNdisBridge)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxSerial} $(DESC_SeccoLinuxSerial)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxDebug} $(DESC_SeccoLinuxDebug)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxImage} $(DESC_SecImage)  
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section
;--------------------------------

Section "Uninstall"

  DetailPrint "TAP-Win32 REMOVE"
  nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" remove TAP0801co'
  Pop $R0 # return value/error/timeout
  DetailPrint "tapcontrol remove returned: $R0"

  nsExec::ExecToLog '"$INSTDIR\colinux-daemon.exe" --remove-driver'
  Pop $R0 # return value/error/timeout

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ;
  Delete "$INSTDIR\coLinux-console-fltk.exe"
  Delete "$INSTDIR\coLinux-console-nt.exe"
  Delete "$INSTDIR\coLinux-debug-daemon.exe"
  Delete "$INSTDIR\coLinux-serial-daemon.exe"
  Delete "$INSTDIR\coLinux-slirp-net-daemon.exe"
  Delete "$INSTDIR\coLinux-daemon.exe"
  Delete "$INSTDIR\coLinux-net-daemon.exe"
  Delete "$INSTDIR\coLinux-bridged-net-daemon.exe"
  Delete "$INSTDIR\colinux-ndis-net-daemon.exe"
  Delete "$INSTDIR\linux.sys"
  Delete "$INSTDIR\vmlinux"
  Delete "$INSTDIR\initrd.gz"
  Delete "$INSTDIR\vmlinux-modules.tar.gz"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\news.txt"
  Delete "$INSTDIR\cofs.txt"
  Delete "$INSTDIR\colinux-daemon.txt"
  Delete "$INSTDIR\debugging.txt"
  Delete "$INSTDIR\example.conf"

  Delete "$INSTDIR\netdriver\OemWin2k.inf"
  Delete "$INSTDIR\netdriver\tap0801co.sys"
  Delete "$INSTDIR\netdriver\tapcontrol.exe"
  Delete "$INSTDIR\netdriver\tap.cat"
  
  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR\netdriver"
  RMDir "$INSTDIR"

  # remove the variable
  Push COLINUX
  Call un.DeleteEnvStr

  ; Cleanup registry
  DeleteRegKey HKLM ${REGUNINSTAL}
  DeleteRegKey HKLM ${REGEVENTS}
  DeleteRegKey HKCU "Software\coLinux"

SectionEnd

# Functions #######################################

;====================================================
; StrStr - Finds a given string in another given string.
;          Returns -1 if not found and the pos if found.
;   Input: head of the stack - string to find
;          second in the stack - string to find in
;   Output: head of the stack
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

###################################################
#
# Downloads a file from a list of mirrors
# (the fist mirror is selected at random)
#
# Usage:
# 	Push Mirror1
# 	Push [Mirror2]
# 	...
# 	Push [Mirror10]
#	Push NumMirrors		# 10 Max
#	Push FileName
#	Call DownloadFromRandomMirror
#	Pop Return
#
#	Returns the NSISdl result
Function DownloadFromRandomMirror
	Exch $R1 #File name
	Exch
	Exch $R0 #Number of Mirros
	Push $0
	Exch 3
	Pop $0	#Mirror 1
	IntCmpU "2" $R0 0 0 +4
		Push $1
		Exch 4
		Pop $1	#Mirror 2
	IntCmpU "3" $R0 0 0 +4
		Push $2
		Exch 5
		Pop $2	#Mirror 3
	IntCmpU "4" $R0 0 0 +4
		Push $3
		Exch 6
		Pop $3	#Mirror 4
	IntCmpU "5" $R0 0 0 +4
		Push $4
		Exch 7
		Pop $4	#Mirror 5
	IntCmpU "6" $R0 0 0 +4
		Push $5
		Exch 8
		Pop $5	#Mirror 6
	IntCmpU "7" $R0 0 0 +4
		Push $6
		Exch 9
		Pop $6	#Mirror 7
	IntCmpU "8" $R0 0 0 +4
		Push $7
		Exch 10
		Pop $7	#Mirror 8
	IntCmpU "9" $R0 0 0 +4
		Push $8
		Exch 11
		Pop $8	#Mirror 9
	IntCmpU "10" $R0 0 0 +4
		Push $9
		Exch 12
		Pop $9	#Mirror 10
	Push $R4
	Push $R2
	Push $R3
	Push $R5
	Push $R6

	# If you don't want a random mirror, replace this block with:
	# StrCpy $R3 "0"
	# -----------------------------------------------------------
	StrCmp $RandomSeed "" 0 +2
		StrCpy $RandomSeed $HWNDPARENT  #init RandomSeed

	Push $RandomSeed
	Push $R0
	Call LimitedRandomNumber
	Pop $R3
	Pop $RandomSeed
	# -----------------------------------------------------------

	StrCpy $R5 "0"
MirrorsStart:
	IntOp $R5 $R5 + "1"
	StrCmp $R3 "0" 0 +3
		StrCpy $R2 $0
		Goto MirrorsEnd
	StrCmp $R3 "1" 0 +3
		StrCpy $R2 $1
		Goto MirrorsEnd
	StrCmp $R3 "2" 0 +3
		StrCpy $R2 $2
		Goto MirrorsEnd
	StrCmp $R3 "3" 0 +3
		StrCpy $R2 $3
		Goto MirrorsEnd
	StrCmp $R3 "4" 0 +3
		StrCpy $R2 $4
		Goto MirrorsEnd
	StrCmp $R3 "5" 0 +3
		StrCpy $R2 $5
		Goto MirrorsEnd
	StrCmp $R3 "6" 0 +3
		StrCpy $R2 $6
		Goto MirrorsEnd
	StrCmp $R3 "7" 0 +3
		StrCpy $R2 $7
		Goto MirrorsEnd
	StrCmp $R3 "8" 0 +3
		StrCpy $R2 $8
		Goto MirrorsEnd
	StrCmp $R3 "9" 0 +3
		StrCpy $R2 $9
		Goto MirrorsEnd
	StrCmp $R3 "10" 0 +3
		StrCpy $R2 $10
		Goto MirrorsEnd

MirrorsEnd:
	IntOp $R6 $R3 + "1"
	DetailPrint "Downloading from mirror $R6: $R2"

	NSISdl::download "$R2" "$R1"
	Pop $R4
	StrCmp $R4 "success" Success
	StrCmp $R4 "cancel" DownloadCanceled
	IntCmp $R5 $R0 NoSuccess
	DetailPrint "Download failed (error $R4), trying with other mirror"
	IntOp $R3 $R3 + "1"
	IntCmp $R3 $R0 0 MirrorsStart
	StrCpy $R3 "0"
	Goto MirrorsStart

DownloadCanceled:
	DetailPrint "Download Canceled: $R2"
	Goto End
NoSuccess:
	DetailPrint "Download Failed: $R1"
	Goto End
Success:
	DetailPrint "Download completed."
End:
	Pop $R6
	Pop $R5
	Pop $R3
	Pop $R2
	Push $R4
	Exch
	Pop $R4
	Exch 2
	Pop $R1
	Exch 2
	Pop $0
	Exch

	IntCmpU "2" $R0 0 0 +4
		Exch 2
		Pop $1
		Exch
	IntCmpU "3" $R0 0 0 +4
		Exch 2
		Pop $2
		Exch
	IntCmpU "4" $R0 0 0 +4
		Exch 2
		Pop $3
		Exch
	IntCmpU "5" $R0 0 0 +4
		Exch 2
		Pop $4
		Exch
	IntCmpU "6" $R0 0 0 +4
		Exch 2
		Pop $5
		Exch
	IntCmpU "7" $R0 0 0 +4
		Exch 2
		Pop $6
		Exch
	IntCmpU "8" $R0 0 0 +4
		Exch 2
		Pop $7
		Exch
	IntCmpU "9" $R0 0 0 +4
		Exch 2
		Pop $8
		Exch
	IntCmpU "10" $R0 0 0 +4
		Exch 2
		Pop $9
		Exch
	Pop $R0
FunctionEnd

###############################################################
#
# Returns a random number
#
# Usage:
# 	Push Seed (or previously generated number)
#	Call RandomNumber
#	Pop Generated Random Number
Function RandomNumber
	Exch $R0

	IntOp $R0 $R0 * "13"
	IntOp $R0 $R0 + "3"
	IntOp $R0 $R0 % "1048576" # Values goes from 0 to 1048576 (2^20)

	Exch $R0
FunctionEnd

####################################################
#
# Returns a random number between 0 and Max-1
#
# Usage:
# 	Push Seed (or previously generated number)
#	Push MaxValue
#	Call RandomNumber
#	Pop Generated Random Number
#	Pop NewSeed
Function LimitedRandomNumber
	Exch $R0
	Exch
	Exch $R1
	Push $R2
	Push $R3

	StrLen $R2 $R0
	Push $R1
RandLoop:
	Call RandomNumber
	Pop $R1	#Random Number
	IntCmp $R1 $R0 0 NewRnd
	StrLen $R3 $R1
	IntOp $R3 $R3 - $R2
	IntOp $R3 $R3 / "2"
	StrCpy $R3 $R1 $R2 $R3
	IntCmp $R3 $R0 0 RndEnd
NewRnd:
	Push $R1
	Goto RandLoop
RndEnd:
	StrCpy $R0 $R3
	IntOp $R0 $R0 + "0" #removes initial 0's
	Pop $R3
	Pop $R2
	Exch $R1
	Exch
	Exch $R0
FunctionEnd
