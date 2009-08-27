;;
;; english.nsh
;;
;; Default language strings for the Windows Purple Plugin Pack NSIS installer.
;; Windows Code Page: 1252
;; Language Code: 1033
;;

;; Startup Checks
LangString PIDGIN_NEEDED ${LANG_ENGLISH} "The Purple Plugin Pack requires that Pidgin be installed.  You must install Pidgin before install the Purple Plugin Pack."

; Overrides for default text in windows:
LangString WELCOME_TITLE ${LANG_ENGLISH} "Purple Plugin Pack v${PP_VERSION} Installer"

LangString WELCOME_TEXT ${LANG_ENGLISH} "Note: This version of the plugin is designed for Pidgin ${PIDGIN_VERSION}, and will not install or function with versions of Pidgin having a different major version number.\r\n\r\nWhen you upgrade your version of Pidgin, you must uninstall or upgrade the Purple Plugin Pack as well.\r\n\r\n"

;; vi: syntax=nsis
