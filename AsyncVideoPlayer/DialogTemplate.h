#ifndef DIALOG_TEMPLATE_H
#define DIALOG_TEMPLATE_H

//windows header (for MS WINDOWS API Window Creation)
#include <Windows.h>
#include <windowsx.h>           // useful Windows programming extensions
#include <ShellAPI.h>

#include <Commdlg.h>			//Open File Dialog

#include <vector>

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
 switch (wm) {
 case WM_INITDIALOG: return TRUE;
 case WM_COMMAND:
  if (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL) EndDialog(hwnd, 0);
  break;
 }
 return FALSE;
}

using namespace std;

class DialogTemplate {
public:

 LPCDLGTEMPLATE Template() { return (LPCDLGTEMPLATE)&v[0]; }
 
 void AlignToDword()
  { if (v.size() % 4) Write(NULL, 4 - (v.size() % 4)); }
 
 void Write(LPCVOID pvWrite, DWORD cbWrite) {
  v.insert(v.end(), cbWrite, 0);
  if (pvWrite) CopyMemory(&v[v.size() - cbWrite], pvWrite, cbWrite);
 }
 
 template<typename T> void Write(T t) { Write(&t, sizeof(T)); }
 
 void WriteString(LPCWSTR psz)
  { Write(psz, (lstrlenW(psz) + 1) * sizeof(WCHAR)); }

private:
 vector<BYTE> v;
};


/*


LPWORD lpwAlign(LPWORD lpIn)
{
    ULONG ul;

    ul = (ULONG)lpIn;
    ul ++;
    ul >>=1;
    ul <<=1;
    return (LPWORD)ul;
}

int createCustomDialog()
{
		 HGLOBAL hgbl;
       LPDLGTEMPLATE lpdt;
       LPDLGITEMTEMPLATE lpdit;
	   LPDLGITEMTEMPLATE lpdit2;

       LPWORD lpw;
       LPWSTR lpwsz;
       LRESULT ret;
	   int nchar;

       hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
       if (!hgbl)
           return -1;

       lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);

	   short dlgWidth, dlgHeight;
	   dlgWidth = 250;
	   dlgHeight = 350;

	   short labelXPadding = 10;
	   short labelYPadding = 4;

	   short labelHeight = 16;

	      unsigned int labelX, labelY;

		//Note:  Dialog items need to be rendered by type in order.  Thanks Microsoft!!! Lots of fun figuring that out and a really tedious way to make a UI.


	   //Must set the DS_CONTROL style instead of WS_CHILD on dialog windows to tell them not to be treated as popups
       lpdt->style = DS_CONTROL | WS_CAPTION | WS_BORDER | WS_SYSMENU | DS_MODALFRAME;
	   //lpdt->dwExtendedStyle = WS_EX_TOOLWINDOW;
       lpdt->cdit = 4;  // number of controls
	   lpdt->x  = 0;  lpdt->y  = 0;
       lpdt->cx =  dlgWidth; lpdt->cy = dlgHeight;

       lpw = (LPWORD) (lpdt + 1);
       *lpw++ = 0;   // no menu
       *lpw++ = 0;   // predefined dialog box class (by default)

       lpwsz = (LPWSTR) lpw;
       nchar = 1+ MultiByteToWideChar (CP_ACP, 0, "Load Raw Format", -1,
                                       lpwsz, 50);
       lpw   += nchar;

	   

	   

	   //define a label text header
	   LPWSTR lpszMessage = L"Video Width";
       //-----------------------
       // Define a static text control.
       //-----------------------
       lpw = lpwAlign (lpw);

       lpdit = (LPDLGITEMTEMPLATE) lpw;
       lpdit->x  = labelXPadding; lpdit->y  = labelYPadding;
       lpdit->cx = dlgWidth- 2*labelXPadding; lpdit->cy = labelHeight;
       lpdit->id = ID_TEXT; //ID_TEXT;  // text identifier
       lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;

       lpw = (LPWORD) (lpdit + 1);
       *lpw++ = 0xFFFF;
       *lpw++ = 0x0082;                         // static class

       for (lpwsz = (LPWSTR)lpw;
           *lpwsz++ = (WCHAR) *lpszMessage++;
          );

       lpw = (LPWORD)lpwsz;
       *lpw++ = 0;                              // no creation data

	   	//-----------------------
       // Define an OK button.
       //-----------------------
       lpw = lpwAlign (lpw);


       lpdit = (LPDLGITEMTEMPLATE) lpw;
       lpdit->x  = labelXPadding; lpdit->y  = 150;
       lpdit->cx = dlgWidth - 2*labelXPadding; lpdit->cy = labelHeight;
       lpdit->id = IDOK;  // OK button identifier
       lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;

       lpw = (LPWORD) (lpdit + 1);
       *lpw++ = 0xFFFF;
       *lpw++ = 0x0080;    // button class

       lpwsz = (LPWSTR) lpw;
       nchar = 1+MultiByteToWideChar (CP_ACP, 0, "Ok", -1, lpwsz, 50);
       lpw   += nchar;
       *lpw++ = 0;              // no creation data
	   
	   
	   


	   //-----------------------
       // Define a Cancel button.
       //-----------------------
       lpw = lpwAlign (lpw);

	   
	   labelY = lpdit->y + lpdit->cy;

       lpdit = (LPDLGITEMTEMPLATE) lpw;
       lpdit->x  = labelXPadding; lpdit->y  = labelY+labelYPadding*2;
       lpdit->cx = dlgWidth - 2*labelXPadding; lpdit->cy = labelHeight;
       lpdit->id = IDCANCEL;  // OK button identifier
       lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;

       lpw = (LPWORD) (lpdit + 1);
       *lpw++ = 0xFFFF;
       *lpw++ = 0x0080;    // button class

       lpwsz = (LPWSTR) lpw;
       nchar = 1+MultiByteToWideChar (CP_ACP, 0, "Cancel", -1, lpwsz, 50);
       lpw   += nchar;
       *lpw++ = 0;              // no creation data


	   //define a label text header
	   LPWSTR lpszMessage2 = L"Video Height";
       //-----------------------
       // Define a static text control.
       //-----------------------
       lpw = lpwAlign (lpw);
	    labelY = lpdit->y + lpdit->cy;
	 
       lpdit = (LPDLGITEMTEMPLATE) lpw;
       lpdit->x  = labelXPadding; lpdit->y  = labelY;
       lpdit->cx = dlgWidth- 2*labelXPadding; lpdit->cy = labelHeight;
       lpdit->id = ID_TEXT+1; //ID_TEXT;  // text identifier
       lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;

       lpw = (LPWORD) (lpdit + 1);
       *lpw++ = 0xFFFF;
       *lpw++ = 0x0082;                         // static class

       for (lpwsz = (LPWSTR)lpw; *lpwsz++ = (WCHAR) *lpszMessage2++; );

       lpw = (LPWORD)lpwsz;
       *lpw++ = 0;                              // no creation data

	   
	 

       GlobalUnlock(hgbl);

	   //use DialogBoxIndirect for Modeless window
       ret = DialogBoxIndirect(glWindow.hinstance, (LPDLGTEMPLATE) hgbl, glWindow.hwnd, (DLGPROC) processDialogCallback);
	   
	   
		//give keyboard focus to the window
		//SetFocus(hwnd);

	   //Use CreateDialog for
	   //HWND dlgHWND = CreateDialog(NULL, (LPCSTR) hgbl, glWindow.hwnd, (DLGPROC)DlgProc);

	   //ShowWindow(dlgHWND, SW_SHOW);
       //SetFocus(dlgHWND);
       
	   
	   GlobalFree(hgbl);

       return 1;

}
*/



#endif