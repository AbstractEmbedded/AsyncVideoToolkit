#ifndef VIDEO_DIALOG_H
#define VIDEO_DIALOG_H

#include "DialogTemplate.h"

//define some IDs for dialog components so we can retrieve them by ID later
#define ID_EDIT	  180
#define ID_EDIT_WIDTH ID_EDIT+1
#define ID_EDIT_HEIGHT ID_EDIT+2
#define ID_TEXT   200


int CALLBACK SetChildFont(HWND child, LPARAM font) {
    SendMessage(child, WM_SETFONT, font, TRUE);
    return TRUE;
}


//set all components of the dialog to use the Windows System Font
static void setDialogFont(HWND hwndDlg)
{
	/* Set font for dialog and all child controls */
	EnumChildWindows(hwndDlg, (WNDENUMPROC)SetChildFont, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));
}

//set the dialog position using SetWindowPos
static void setDialogPosition(HWND parentDlg, HWND hwndDlg)
{

	RECT rcOwner;
	RECT rcDlg;
	RECT rc;

	GetWindowRect(parentDlg, &rcOwner); 
	GetWindowRect(hwndDlg, &rcDlg); 
	CopyRect(&rc, &rcOwner); 

	// Offset the owner and dialog box rectangles so that right and bottom 
	// values represent the width and height, and then offset the owner again 
	// to discard space taken up by the dialog box. 

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	// The new position is the sum of half the remaining space and the owner's 
	// original position. 

	SetWindowPos(hwndDlg, 
				 HWND_TOP, 
				 rcOwner.left + (rc.right / 2), 
				 rcOwner.top + (rc.bottom / 2), 
				 0, 0,          // Ignores size arguments. 
				 SWP_NOSIZE); 

	/*
	if (GetDlgCtrlID((HWND) wParam) != ID_ITEMNAME) 
	{ 
		SetFocus(GetDlgItem(hwndDlg, ID_ITEMNAME)); 
		return FALSE; 
	} 
	*/
}

static BOOL createVideoPropertiesDialog(HWND hwnd, LPCWSTR pszTitle, DLGPROC dlgCallback, unsigned int videoWidth, unsigned int videoHeight)
{
	BOOL fSuccess = FALSE;
	HDC hdc = GetDC(NULL);
	if (hdc) 
	{


		NONCLIENTMETRICSW ncm = { sizeof(ncm) };
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0)) 
		{
			wchar_t vWidthStr[1024];
			wchar_t vHeightStr[1024];
			swprintf_s(vWidthStr, L"%u", videoWidth);
			swprintf_s(vHeightStr, L"%u", videoHeight);

			//use the Dialog Template Obj
			DialogTemplate tmp;

			short dlgWidth = 200;
			short dlgHeight = 300;
			short labelXPadding = 10;
			short labelYPadding = 4;

			short labelHeight = 16;
			short textLabelHeight, textEditHeight, buttonHeight;
			short labelX, labelY;

			
			// Next comes the font description.
			// See text for discussion of fancy formula.
			if (ncm.lfMessageFont.lfHeight < 0) {
				ncm.lfMessageFont.lfHeight = -MulDiv(ncm.lfMessageFont.lfHeight,72, GetDeviceCaps(hdc, LOGPIXELSY));
			}

			//set labelheight ot be the size of the font
			textLabelHeight = (short)ncm.lfMessageFont.lfHeight;
			textEditHeight = (short)ncm.lfMessageFont.lfHeight + 2;
			buttonHeight = 16;

			//calculate total height needed for all components
			dlgHeight = labelYPadding + textLabelHeight * 2 + textEditHeight * 2 + labelYPadding * 2 + buttonHeight + labelYPadding;

			// Write out the extended dialog template header
			tmp.Write<WORD>(1); // dialog version
			tmp.Write<WORD>(0xFFFF); // extended dialog template
			tmp.Write<DWORD>(0); // help ID
			tmp.Write<DWORD>(0); // extended style
			tmp.Write<DWORD>(WS_CAPTION | WS_SYSMENU | DS_SETFONT | DS_MODALFRAME);
			tmp.Write<WORD>(5); // number of controls
			tmp.Write<WORD>(0); // X
			tmp.Write<WORD>(0); // Y
			tmp.Write<WORD>(dlgWidth); // width
			tmp.Write<WORD>(dlgHeight); // height
			tmp.WriteString(L""); // no menu
			tmp.WriteString(L""); // default dialog class
			tmp.WriteString(pszTitle); // title


			// Next comes the font description.
			// See text for discussion of fancy formula.
			if (ncm.lfMessageFont.lfHeight < 0) {
				ncm.lfMessageFont.lfHeight = -MulDiv(ncm.lfMessageFont.lfHeight,72, GetDeviceCaps(hdc, LOGPIXELSY));
			}
			tmp.Write<WORD>((WORD)ncm.lfMessageFont.lfHeight); // point
			tmp.Write<WORD>((WORD)ncm.lfMessageFont.lfWeight); // weight
			tmp.Write<BYTE>(ncm.lfMessageFont.lfItalic); // Italic
			tmp.Write<BYTE>(ncm.lfMessageFont.lfCharSet); // CharSet
			tmp.WriteString(ncm.lfMessageFont.lfFaceName);

		

			labelY = labelYPadding;
			labelHeight = textLabelHeight;
			
			// Then come the two controls.  First is the static text.
			tmp.AlignToDword();
			tmp.Write<DWORD>(0); // help id
			tmp.Write<DWORD>(0); // window extended style
			tmp.Write<DWORD>(WS_CHILD | WS_VISIBLE); // style
			tmp.Write<WORD>(labelXPadding); // x
			tmp.Write<WORD>(labelY); // y
			tmp.Write<WORD>(dlgWidth - 2*labelXPadding); // width
			tmp.Write<WORD>(labelHeight); // height
			tmp.Write<DWORD>(-1); // control ID
			tmp.Write<DWORD>(0x0082FFFF); // static
			tmp.WriteString(L"Video Width:"); // text
			tmp.Write<WORD>(0); // no extra data
   

			// HWND video_properties_edit = CreateWindowEx(WS_EX_TOOLWINDOW, "edit", "Line one", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT, CW_USEDEFAULT, CW_USEDEFAULT, 200, 24, video_properties_main, (HMENU)(101),	(HINSTANCE) GetWindowLong (video_properties_main, 0), NULL);

			labelY += labelHeight;
			labelHeight = textEditHeight;

			tmp.AlignToDword();
			tmp.Write<DWORD>(101); // help id
			tmp.Write<DWORD>(WS_EX_TOOLWINDOW); // window extended style
			tmp.Write<DWORD>(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT); // style
			tmp.Write<WORD>(labelXPadding); // x
			tmp.Write<WORD>(labelY); // y
			tmp.Write<WORD>(dlgWidth - 2*labelXPadding); // width
			tmp.Write<WORD>(labelHeight); // height
			tmp.Write<DWORD>(ID_EDIT_WIDTH); // control ID
			tmp.Write<DWORD>(0x0081FFFF); // static
			tmp.WriteString(vWidthStr); // text
			tmp.Write<WORD>(0); // no extra data

			labelY += labelHeight + labelYPadding;
			labelHeight = textLabelHeight;

			// Then come the two controls.  First is the static text.
			tmp.AlignToDword();
			tmp.Write<DWORD>(0); // help id
			tmp.Write<DWORD>(0); // window extended style
			tmp.Write<DWORD>(WS_CHILD | WS_VISIBLE); // style
			tmp.Write<WORD>(labelXPadding); // x
			tmp.Write<WORD>(labelY); // y
			tmp.Write<WORD>(dlgWidth - 2*labelXPadding); // width
			tmp.Write<WORD>(labelHeight); // height
			tmp.Write<DWORD>(-1); // control ID
			tmp.Write<DWORD>(0x0082FFFF); // static
			tmp.WriteString(L"Video Height:"); // text
			tmp.Write<WORD>(0); // no extra data
   

			labelY += labelHeight ;//+ labelYPadding;
			labelHeight = textEditHeight;
			tmp.AlignToDword();
			tmp.Write<DWORD>(0); // help id
			tmp.Write<DWORD>(WS_EX_TOOLWINDOW); // window extended style
			tmp.Write<DWORD>(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT); // style
			tmp.Write<WORD>(labelXPadding); // x
			tmp.Write<WORD>(labelY); // y
			tmp.Write<WORD>(dlgWidth - 2*labelXPadding); // width
			tmp.Write<WORD>(labelHeight); // height
			tmp.Write<DWORD>(ID_EDIT_HEIGHT); // control ID
			tmp.Write<DWORD>(0x0081FFFF); // static
			tmp.WriteString(vHeightStr); // text
			tmp.Write<WORD>(0); // no extra data


			labelY += labelHeight + labelYPadding;
			labelHeight = buttonHeight;

			// Second control is the OK button.
			tmp.AlignToDword();
			tmp.Write<DWORD>(0); // help id
			tmp.Write<DWORD>(0); // window extended style
			tmp.Write<DWORD>(WS_CHILD | WS_VISIBLE |
							WS_GROUP | WS_TABSTOP | BS_DEFPUSHBUTTON); // style
			tmp.Write<WORD>(labelXPadding); // x
			tmp.Write<WORD>(labelY); // y
			tmp.Write<WORD>(dlgWidth - 2*labelXPadding); // width
			tmp.Write<WORD>(labelHeight); // height
			tmp.Write<DWORD>(IDOK); // control ID
			tmp.Write<DWORD>(0x0080FFFF); // static
			tmp.WriteString(L"OK"); // text
			tmp.Write<WORD>(0); // no extra data

			// Template is ready - go display it.
			fSuccess = DialogBoxIndirect(glWindow.hinstance, tmp.Template(), hwnd, dlgCallback) >= 0;


		}
		ReleaseDC(NULL, hdc); // fixed 11 May
	}
	return fSuccess;
}

#endif