; NSIS Script for the Purple Plugin Pack
; Author Gary Kramlich
; Based on the guifications2.x installer
; Uses NSIS v2.0

;Include Modern UI
!include "MUI.nsh"

!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

;---------
; General
;---------
Name "Plugin Pack ${PP_VERSION}"
CRCCheck On
OutFile "purple-plugin-pack-${PP_VERSION}.exe"

InstallDir "$PROGRAMFILES\pidgin"
InstallDirRegKey HKLM SOFTWARE\pidgin ""

ShowInstDetails show
ShowUnInstDetails show
SetCompressor /SOLID lzma

!insertmacro MUI_RESERVEFILE_LANGDLL
!define PP_UNINST_EXE "purple-plugin-pack-uninst.exe"
   

; Pidgin helper stuff
!addincludedir "${PIDGIN_TREE_TOP}\pidgin\win32\nsis"
!include "pidgin-plugin.nsh"

;---------------------------
; UI Config
;---------------------------
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "nsis\header.bmp"
!define MUI_ABORTWARNING

;---------------------------
; Translations
;---------------------------
!insertmacro MUI_LANGUAGE "English"
!include "nsis\translations\english.nsh"

;---------------------------
; Pages
;---------------------------

; Welcome
!define MUI_WELCOMEPAGE_TITLE $(WELCOME_TITLE)
!define MUI_WELCOMEPAGE_TEXT $(WELCOME_TEXT)
!insertmacro MUI_PAGE_WELCOME

;---------------------------
; Sections
;---------------------------
Section -SecUninstallOld
  ; Check install rights...
  Call CheckUserInstallRights
  Pop $R0

  StrCmp $R0 "HKLM" rights_hklm
  StrCmp $R0 "HKCU" rights_hkcu done
