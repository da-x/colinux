;Cooperative Linux installer
;Written by NEBOR Regis
;Modified by Dan Aloni (c) 2004
;Modified 8/20/2004,2/4/2004 by George P Boutwell

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

  VIAddVersionKey ProductName "Cooperative Linux"
  VIAddVersionKey CompanyName "${PUBLISHER}"
  VIAddVersionKey ProductVersion "${VERSION}"
  VIAddVersionKey FileVersion "${VERSION}"
  VIAddVersionKey FileDescription "An optimized virtual Linux system for Windows"
  VIAddVersionKey LegalCopyright "Copyright @ 2004 ${PUBLISHER}"
  VIProductVersion "${LONGVERSION}"

  XPStyle on

; For priority Unpack
  ReserveFile "iDl.ini"
  ReserveFile "WinpcapRedir.ini"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
 
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
  Page custom WinpcapRedir WinpcapRedirLeave
  Page custom StartDlImageFunc EndDlImageFunc
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
;Custom Setup for image download

;Variables used to track user choice
  Var GENTOO
  Var DEBIAN
  Var NOTHING
  ; ....
  ;Var OIMAGE
  ; ....
  Var LOCATION
  Var RandomSeed

; StartDlImageFunc down for reference resolution

Function EndDlImageFunc

  ;Read a value from an InstallOptions INI file
  !insertmacro MUI_INSTALLOPTIONS_READ $DEBIAN "iDl.ini" "Field 1" "State"
  !insertmacro MUI_INSTALLOPTIONS_READ $GENTOO "iDl.ini" "Field 2" "State"
  !insertmacro MUI_INSTALLOPTIONS_READ $NOTHING "iDl.ini" "Field 7" "State"
  ;!insertmacro MUI_INSTALLOPTIONS_READ $OIMAGE "iDl.ini" "Field x" "State"

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


Section "coLinux" SeccoLinux

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "DisplayName" "coLinux"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "UninstallString" '"$INSTDIR\Uninstall.exe"'


  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "${DISTDIR}\colinux-console-fltk.exe"
  File "${DISTDIR}\colinux-console-nt.exe"
  File "${DISTDIR}\colinux-net-daemon.exe"
  File "${DISTDIR}\colinux-slirp-net-daemon.exe"
  File "${DISTDIR}\colinux-debug-daemon.exe"
  File "${DISTDIR}\colinux-daemon.exe"
  File "${DISTDIR}\README.txt"
  File "${DISTDIR}\news.txt"
  File "${DISTDIR}\cofs.txt"
  File "${DISTDIR}\colinux-daemon.txt"
  
  ;Enable when working correctly
  ;File "${DISTDIR}\colinux-serial-daemon.exe"
  File "${DISTDIR}\linux.sys"
  File "${DISTDIR}\vmlinux"
  File /oname=vmlinux-modules.tar.gz "${DISTDIR}\modules-${KERNEL_VERSION}-co-${PRE_VERSION}.tar.gz"
  ; initrd replaces vmlinux-modules.tar.gz as preferred way to ship modules.
  File "premaid\initrd.gz"

  ;Backup config file if present
  IfFileExists "$INSTDIR\default.colinux.xml" 0 +1
  CopyFiles /SILENT "$INSTDIR\default.colinux.xml" "$INSTDIR\default.colinux.xml.old"
  File "${DISTDIR}\default.colinux.xml"

  ; Remove kludge from older installations	
  Delete "$INSTDIR\packet.dll"
  Delete "$INSTDIR\wpcap.dll"

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
  File "premaid\netdriver\tap0801co.sys"
  File "premaid\netdriver\tapcontrol.exe"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------


SectionEnd

Section "coLinux Bridged Ethernet (WinPcap)" SeccoLinuxBridgedNet

  ;---------------------------------------------------------------FILES--
  ;----------------------------------------------------------------------
  ; Our Files . If you adds something here, Remember to delete it in 
  ; the uninstall section

  SetOutPath "$INSTDIR"
  File "${DISTDIR}\coLinux-bridged-net-daemon.exe"

  ;--------------------------------------------------------------/FILES--
  ;----------------------------------------------------------------------

SectionEnd

Section "Root Filesystem image Download" SeccoLinuxImage

   ;----------------------------------------------------------
   ; Random sourceforge download
   
    StrCmp $NOTHING "1" "" tryGentoo
    Goto End

    tryGentoo:
    StrCmp $GENTOO "1" "" tryDebian
    ;MessageBox MB_OK "Gentoo"
    StrCpy $R0 "Gentoo-colinux-stage3-x86-2004.3.bz2"
    Goto tryDownload

    tryDebian:
    StrCmp $DEBIAN "1" "" End ; tryOImage
    ;MessageBox MB_OK "Debian"
    StrCpy $R0 "Debian-3.0r2.ext3-mit-backports.1gb.bz2"

    ; ....
    ;tryOImage:
    ;StrCmp $OIMAGE "1" "" End or tryOther
    ;MessageBox MB_OK "Gentoo"
    ;StrCpy $R0 "nameOfImageXfile"
    ;Goto tryDownload
    ; ....

    tryDownload:
    StrCpy $R1 "colinux" ; project


    !insertmacro MUI_INSTALLOPTIONS_READ $LOCATION "iDl.ini" "Field 4" "State"
    StrCmp $LOCATION "Random" Random
    StrCmp $LOCATION "Europe" Europe
    StrCmp $LOCATION "Asia" Asia
    StrCmp $LOCATION "North America" NorthAmerica

    Random:
    ;MessageBox MB_OK "Random"
    	Push "http://belnet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://cesnet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://heanet.dl.sourceforge.net/sourceforge/$R1/$R0"
	;Push "http://switch.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://keihanna.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://easynews.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://twtelecom.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://flow.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://aleron.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://umn.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://unc.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push 10
    Goto DownloadRandom
    Europe:
    ;MessageBox MB_OK "Europe"
        Push "http://belnet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://cesnet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://heanet.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push "http://switch.dl.sourceforge.net/sourceforge/$R1/$R0"
	Push 4
    Goto DownloadRandom
    Asia:
    ;MessageBox MB_OK "Asia"
        Push "http://keihanna.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push 1
    Goto DownloadRandom
    NorthAmerica:
    ;MessageBox MB_OK "NorthAmerica"
        Push "http://twtelecom.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://aleron.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://easynews.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://umn.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push "http://unc.dl.sourceforge.net/sourceforge/$R1/$R0"
        Push 5

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
    DetailPrint "TAP-Win32 UPDATE"
    nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" update "$INSTDIR\netdriver\OemWin2k.inf" TAP0801co'
    Pop $R0 # return value/error/timeout
    IntOp $5 $5 | $R0
    DetailPrint "tapcontrol update returned: $R0"
    Goto tapcontrol_check_error

 tapinstall:
    DetailPrint "TAP-Win32 INSTALL"
    nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" install "$INSTDIR\netdriver\OemWin2k.inf" TAP0801co'
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
    nsExec::ExecToStack '"$INSTDIR\colinux-daemon.exe" --remove-driver'
    Pop $R0 # return value/error/timeout
    nsExec::ExecToStack '"$INSTDIR\colinux-daemon.exe" --install-driver'
    Pop $R0 # return value/error/timeout
SectionEnd


;--------------------------------
;Descriptions

  LangString DESC_SeccoLinux ${LANG_ENGLISH} "Install coLinux"
  LangString DESC_SeccoLinuxNet ${LANG_ENGLISH} "Install coLinux Virtual Ethernet Driver, which allows to create a network link between Linux and Windows"
  LangString DESC_SeccoLinuxBridgedNet ${LANG_ENGLISH} "Install coLinux Bridge Ethernet support, which allows to join the coLinux machine to an existing network"
  LangString DESC_SecImage ${LANG_ENGLISH} "Download an image from sourceforge. Also provide useful links on how to use it"

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinux} $(DESC_SeccoLinux)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxNet} $(DESC_SeccoLinuxNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxBridgedNet} $(DESC_SeccoLinuxBridgedNet)
    !insertmacro MUI_DESCRIPTION_TEXT ${SeccoLinuxImage} $(DESC_SecImage)  
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section
;--------------------------------


Section "Uninstall"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "DisplayName"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux" "UninstallString"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\coLinux"

  DetailPrint "TAP-Win32 REMOVE"
  nsExec::ExecToLog '"$INSTDIR\netdriver\tapcontrol.exe" remove TAP0801co'
  Pop $R0 # return value/error/timeout
  DetailPrint "tapcontrol remove returned: $R0"

  nsExec::ExecToStack '"$INSTDIR\colinux-daemon.exe" --remove-driver'
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
  Delete "$INSTDIR\linux.sys"
  Delete "$INSTDIR\vmlinux"
  Delete "$INSTDIR\initrd.gz"
  Delete "$INSTDIR\vmlinux-modules.tar.gz"
  Delete "$INSTDIR\default.coLinux.xml"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\news.txt"
  Delete "$INSTDIR\cofs.txt"
  Delete "$INSTDIR\colinux-daemon.txt"

  Delete "$INSTDIR\netdriver\OemWin2k.inf"
  Delete "$INSTDIR\netdriver\tap0801co.sys"
  Delete "$INSTDIR\netdriver\tapcontrol.exe"
  
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

; Taken from the archives : Dowload from random Mirror

# Functions #######################################

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
# NOTE: If you don't want a random mirror, remove this Function
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
# NOTE: If you don't want a random mirror, remove this Function
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
