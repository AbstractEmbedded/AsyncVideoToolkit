#include "cr_display.h"//"../crPlatform.h"
#include <windows.h>

#include <stdio.h>
//#include <atlstr.h>
#include <SetupApi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN
#pragma comment(lib, "setupapi.lib")
 
#define NAME_SIZE 128
 
const GUID GUID_CLASS_MONITOR = {0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18};
 
// Assumes hEDIDRegKey is valid
bool GetMonitorSizeFromEDID(const HKEY hEDIDRegKey, short * WidthMm, short * HeightMm)
{
#ifdef UNICODE
	WCHAR * wEDIDdata;
#endif 

    BYTE EDIDdata[1024];
    DWORD edidsize = sizeof(EDIDdata);
	DWORD type = 0;

	//if (ERROR_SUCCESS != RegGetValue(hEDIDRegKey, ("EDID"), NULL, &type, EDIDdata, &edidsize))
	//	return false;
#ifdef UNICODE
    if (ERROR_SUCCESS != RegQueryValueEx(hEDIDRegKey, L"EDID", NULL, &type, EDIDdata, &edidsize))
        return false;

	//wEDIDdata =(WCHAR*)(&EDIDdata);
	//*WidthMm = ((wEDIDdata[68] & 0xF0) << 4) + wEDIDdata[66];
    //*HeightMm = ((wEDIDdata[68] & 0x0F) << 8) + wEDIDdata[67];
	*WidthMm = ((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66];
    *HeightMm = ((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67];

	/*
	
	*WidthMm = ((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66];
    *HeightMm = ((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67];

	switch (type)
    {
        case REG_DWORD:
        {
            //LPDWORD value = reinterpret_cast<LPDWORD>(&EDIDdata);
            // use *value as needed ...
            break;
        }

        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
        {	
			//unicode null termianted string 
            // note the T in LPTSTR!  That means 'TCHAR' is used...
            //LPTSTR text = reinterpret_cast<LPTSTR>(&EDIDdata);
            // use text as needed, up to (dataSize/sizeof(TCHAR)) number
            // of TCHARs. This is because RegQueryValueEx() does not
            // guarantee the output data has a null terminator.  If you
            // want that, use RegGetValue() instead...
            break;
        }
    }
	*/
#else
	if (ERROR_SUCCESS != RegQueryValueEx(hEDIDRegKey, "EDID", NULL, &type, EDIDdata, &edidsize))
        return false;

	*WidthMm = ((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66];
    *HeightMm = ((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67];
#endif 

 
    return true; // valid EDID found
}

bool DisplayDeviceFromHMonitor(HMONITOR hMonitor, DISPLAY_DEVICE* ddMonOut)
{
	bool bFoundDevice;
	 
    DISPLAY_DEVICE ddMon;
	MONITORINFOEX mi;
    DISPLAY_DEVICE dd;
	DWORD devIdx = 0; // device index
	DWORD MonIdx;

    mi.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &mi);
 
    dd.cb = sizeof(dd);
 
    //CString DeviceID;
    bFoundDevice = false;
    while (EnumDisplayDevices(0, devIdx, &dd, 0))
    {
        devIdx++;
#ifdef UNICODE
		if (0 != wcscmp(dd.DeviceName, mi.szDevice))
			continue;
#else
        if (0 != strcmp(dd.DeviceName, mi.szDevice))
            continue;
#endif 

        ZeroMemory(&ddMon, sizeof(ddMon));
        ddMon.cb = sizeof(ddMon);
        MonIdx = 0;
 
        while (EnumDisplayDevices(dd.DeviceName, MonIdx, &ddMon, 0))
        {
            MonIdx++;
 
            *ddMonOut = ddMon;
            return TRUE;
 
            ZeroMemory(&ddMon, sizeof(ddMon));
            ddMon.cb = sizeof(ddMon);
        }
 
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
    }
 
    return FALSE;
}


char * cr_string_find_character(char * search_string, char character/*, unsigned int repeat*/)
{
	char * charPtr;
	unsigned int repeatIndex;
	unsigned int length = strlen(search_string);
	
	//printf("String length = %u\n", length);

	charPtr = strchr(search_string, character);

	/*
	for( repeatIndex = 1; repeatIndex<repeat; repeatIndex++)
	{
		if( charPtr && (charPtr+1 - devicePath) < length )
		{
			charPtr++;
			charPtr = strchr(charPtr, character);
		}
		else
			return NULL;
	}
	*/

	//advance past the character
	//charPtr++;

	return charPtr;
}


/*
    CString sOut = sIn.Right(sIn.GetLength() - FirstSlash - 1);
    FirstSlash = sOut.Find(_T('\\'));
    sOut = sOut.Left(FirstSlash);
    return sOut;
}
*/

#ifdef UNICODE

bool GetSizeForDeviceID(const WCHAR* TargetDevID, short* WidthMm, short* HeightMm)
{
	ULONG i;
	bool bRes;
    HDEVINFO devInfo = SetupDiGetClassDevsEx(
        &GUID_CLASS_MONITOR, //class GUID
        NULL, //enumerator
        NULL, //HWND
        DIGCF_PRESENT | DIGCF_PROFILE, // Flags //DIGCF_ALLCLASSES|
        NULL, // device info, create a new one.
        NULL, // machine name, local machine
        NULL);// reserved
 
    if (NULL == devInfo)
        return false;
 
    bRes = false;
 
    for (i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i)
    {
		HKEY hEDIDRegKey;
        SP_DEVINFO_DATA devInfoData;
        memset(&devInfoData, 0, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);
 
        if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
        {
            /*TCHAR*/ WCHAR Instance[MAX_DEVICE_ID_LEN];
            SetupDiGetDeviceInstanceId(devInfo, &devInfoData, Instance, MAX_PATH, NULL);
 
			// TO DO:  check if Instance string contains targetDevID string

			if( !wcsstr(Instance, TargetDevID) )
			{
				//printf("\n Instance String (%S) != TargetDevID (%S)\n", Instance, TargetDevID);
				//printf("\n Instance String Length (%d) != TargetDevID (%d)\n", strlen(Instance), strlen(TargetDevID));
				continue;
			
			}
			//else
				//printf("\n Instance String (%S) == TargetDevID (%S)\n", Instance, TargetDevID);

            hEDIDRegKey = SetupDiOpenDevRegKey(devInfo, &devInfoData,
                DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
 
            if (!hEDIDRegKey || (hEDIDRegKey == INVALID_HANDLE_VALUE))
			{
				printf("\n INVALID_HANDLE_VALUE \n", Instance, TargetDevID);
                continue;
			}
            bRes = GetMonitorSizeFromEDID(hEDIDRegKey, WidthMm, HeightMm);
 
            RegCloseKey(hEDIDRegKey);
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return bRes;


}

#else

bool GetSizeForDeviceID(const char* TargetDevID, short* WidthMm, short* HeightMm)
{
	ULONG i;
	bool bRes;
    HDEVINFO devInfo = SetupDiGetClassDevsEx(
        &GUID_CLASS_MONITOR, //class GUID
        NULL, //enumerator
        NULL, //HWND
        DIGCF_PRESENT | DIGCF_PROFILE, // Flags //DIGCF_ALLCLASSES|
        NULL, // device info, create a new one.
        NULL, // machine name, local machine
        NULL);// reserved
 
    if (NULL == devInfo)
	{
		printf("\n devInfo is NULL\n");
        return false;
	}

    bRes = false;
 
    for (i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i)
    {
		HKEY hEDIDRegKey;
        SP_DEVINFO_DATA devInfoData;
        memset(&devInfoData, 0, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);
 
        if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
        {
            TCHAR Instance[MAX_DEVICE_ID_LEN];
            SetupDiGetDeviceInstanceId(devInfo, &devInfoData, Instance, MAX_PATH, NULL);
 
			// TO DO:  check if Instance string contains targetDevID string

			if( !strstr(Instance, TargetDevID) )
			{
				//printf("\n Instance String (%s) != TargetDevID (%s)\n", Instance, TargetDevID);
				//printf("\n Instance String Length (%d) != TargetDevID (%d)\n", strlen(Instance), strlen(TargetDevID));
				continue;

			}
            //CString sInstance(Instance);
            //if (-1 == sInstance.Find(TargetDevID))
            //    continue;
 
            hEDIDRegKey = SetupDiOpenDevRegKey(devInfo, &devInfoData,
                DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
 
            if (!hEDIDRegKey || (hEDIDRegKey == INVALID_HANDLE_VALUE))
                continue;
 
            bRes = GetMonitorSizeFromEDID(hEDIDRegKey, WidthMm, HeightMm);
 
            RegCloseKey(hEDIDRegKey);
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return bRes;
}
#endif

cr_ulong2 cr_display_get_resolution()
{
		long lRet;
		cr_ulong2 screenResolution;
	/*
	cr_long2 screenResolution;
	   RECT desktop;
	   // Get a handle to the desktop window
	   const HWND hDesktop = GetDesktopWindow();
	   // Get the size of screen to the variable desktop
	   GetWindowRect(hDesktop, &desktop);
	   // The top left corner will have coordinates (0,0)
	   // and the bottom right corner will have coordinates
	   // (horizontal, vertical)
	   screenResolution.x = desktop.right;
	   screenResolution.y = desktop.bottom;
	   */

	DEVMODE dm;
   // initialize the DEVMODE structure
   ZeroMemory(&dm, sizeof(dm));
   dm.dmSize = sizeof(dm);

   if (0 != EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
   {
      // inspect the DEVMODE structure to obtain details
      // about the display settings such as
      //  - Orientation
      //  - Width and Height
      //  - Frequency
      //  - etc.

	   screenResolution.x = dm.dmPelsWidth;
	   screenResolution.y = dm.dmPelsHeight;

	   /*
	   dm.dmPelsWidth = 3840;
	   dm.dmPelsHeight = 2160;
		lRet = ChangeDisplaySettings(&dm, 0);
		if (DISP_CHANGE_SUCCESSFUL != lRet)
		{
			// add exception handling here
		}
		*/
   }

	   return screenResolution;

}

cr_float2 cr_display_get_dimensions()
{
#ifdef UNICODE
	WCHAR * deviceID;
	WCHAR * deviceIDend;
#else
	char * deviceID;
	char * deviceIDend;
#endif 
	short widthInMM, heightInMM;
	bool bFoundDevice;
	MONITORINFO monitorInfo;
	cr_float2 screenDimensions;
	HMONITOR hMonitor;
	DISPLAY_DEVICE ddMon;
	//CString DeviceID;
    POINT    pt;
    UINT     dpix = 0, dpiy = 0;
    BOOL  hr = FALSE;

    // Get the DPI for the main monitor, and set the scaling factor
    pt.x = 1;
    pt.y = 1;
    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    //hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);

	//WmiMonitorBasicDisplayParams params;
	monitorInfo.cbSize = sizeof(MONITORINFO);

	hr = GetMonitorInfo( hMonitor, &monitorInfo);

    if (!hr)
    {
#ifdef UNICODE
        MessageBox(NULL, L"GetMonitorInfo failed", L"Notification", MB_OK);
#else
		MessageBox(NULL, "GetMonitorInfo failed", "Notification", MB_OK);
#endif
		//return FALSE;
    }
	

	//screenResolution = cr_get_display_resolution();
	//screenResolution.x = (float)(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) ;/// 72.0f;
	//screenResolution.y = (float)(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) ;/// 72.0f;


	//printf("\nscreen resolution x (%d), screen resolution y (%ld)\n", screenResolution.x, screenResolution.y);

	// Get a DISPLAY_DEVICE from the HMONITOR
    if (FALSE == DisplayDeviceFromHMonitor(hMonitor, &ddMon))
	{
		printf("\nDisplayDeviceFromHMonitorFailed \n");
	}


	widthInMM = heightInMM = 0;
#ifdef UNICODE

	//we need to extract from the DISPLAY_DEVICE.DeviceID which is a registry path containing slashes
	//search for first occurence of slash character in DISPLAY_DEVICE.DeviceID string
	deviceID = wcschr(ddMon.DeviceID, L'\\');
	//advance past the slash character
	deviceID++;
	//get the second occurrence of slash character to find the end of the string we want to extract
	deviceIDend = wcschr(deviceID, L'\\');

	//lop off the end of the DeviceID path so we can pass it to the next function
	*deviceIDend = L'\0';

	//advance past the '\\' character
	//deviceID++;

	//printf("\nDevice ID =  %S\n", deviceID);
	
	bFoundDevice = GetSizeForDeviceID(deviceID, &widthInMM, &heightInMM);
#else

	//we need to extract from the DISPLAY_DEVICE.DeviceID which is a registry path containing slashes
	//search for first occurence of slash character in DISPLAY_DEVICE.DeviceID string
	deviceID = strchr(ddMon.DeviceID, '\\');
	//advance past the slash character
	deviceID++;
	//get the second occurrence of slash character to find the end of the string we want to extract
	deviceIDend = strchr(deviceID, '\\');

	//lop off the end of the DeviceID path so we can pass it to the next function
	*deviceIDend = '\0';

	//advance past the '\\' character
	//deviceID++;

	//printf("\nDevice ID =  %s\n", deviceID);
	
	bFoundDevice = GetSizeForDeviceID(deviceID, &widthInMM, &heightInMM);
#endif
 
	screenDimensions.x = widthInMM;
	screenDimensions.y = heightInMM;


	return screenDimensions;

    //g_Dpi.SetScale(dpix);
}

 
