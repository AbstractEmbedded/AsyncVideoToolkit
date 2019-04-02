#ifndef ASYNC_VIDEO_PLAYER_GUI_H
#define ASYNC_VIDEO_PLAYER_GUI_H

//#include "textentrydlg.h"

//There is no C/C++ API in Window for creating a DialogBox ?
//But we can link the system Windows Forms and Visual Basic dlls to do this
//by endabling Common Language Runtime Support in Project->Properties->General->Configuration
//Note:  When enabling /clr, must also set the compiler flag /TP in Project->Properties->C/C++->Command Line
//#using <system.windows.forms.dll>
//#using <Microsoft.VisualBasic.dll>

//typedef basic_string<wchar_t> wstring;
#include <uxtheme.h>
#include "VideoDialog.h"

//using namespace System;


//WIN32 MENU ITEM DEFINITIONS
//FILE MENU
#define IDM_FILE_IMPORT_VIDEO 1
#define IDM_FILE_EXPORT_TEMPERATURES_TO_SOURCE_FORMAT 2
#define IDM_FILE_EXPORT_TO_TDF 3
#define IDM_FILE_QUIT 4

#define IDM_FILE_LAST_MENU_ITEM IDM_FILE_QUIT

//PLAYBACK MENU
#define IDM_PLAYBACK_PLAY_PAUSE IDM_FILE_LAST_MENU_ITEM + 1
#define IDM_PLAYBACK_TOGGLE_VSYNC IDM_PLAYBACK_PLAY_PAUSE + 1
#define IDM_PLAYBACK_LAST_MENU_ITEM IDM_PLAYBACK_TOGGLE_VSYNC

//WINDOW MENU
#define IDM_WINDOW_TOGGLE_BORDER IDM_PLAYBACK_LAST_MENU_ITEM + 1
#define IDM_WINDOW_TOGGLE_FULLSCREEN IDM_WINDOW_TOGGLE_BORDER + 1
#define IDM_WINDOW_LAST_MENU_ITEM IDM_WINDOW_TOGGLE_FULLSCREEN


//DIALOG DEFINITIONS

#endif