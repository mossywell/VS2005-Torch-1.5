//
// Includes
//
#include <windows.h>
#include <windowsx.h>
#include <aygshell.h>
#include "resource.h"
#include <strsafe.h>
#include <pm.h>


//
// Defines
//
#define ARRAYSIZE(a)   (sizeof(a)/sizeof(*a))


//
// Globals (consts at top)
//
const TCHAR* g_szAppWndClass = TEXT("Torch");
const TCHAR* g_szAppTitle = TEXT("Torch");
const UINT g_TASKBARSIZE = 40;
const UINT g_ColourDelta = 5;
const INT COLOR_MIN = 0;
const INT COLOR_MAX = 255;
const UINT g_ColorTableSize = 16;
const LONG g_FontHeight = -10;
const LONG g_TextTopOffset = 0;
const LONG g_TextLeftOffset = 3;
const INT g_DiscoSpeedMin = 25;
const INT g_DiscoSpeedMax = 3000;
const INT g_DiscoSpeedDelta = 25;
INT g_DiscoSpeed = 300;
HINSTANCE g_hInst = NULL;
HANDLE g_hPower = NULL;
CEDEVICE_POWER_STATE g_cps = D0;
INT g_Red = 255;
INT g_Green = 255;
INT g_Blue = 255;
COLORREF g_Color[g_ColorTableSize];
HFONT g_hFont;
BOOL g_DiscoMode = FALSE;
BOOL g_ShowRgb = FALSE;
UINT g_ColorIndex = g_ColorTableSize - 1;
UINT g_OldColorIndex = g_ColorIndex;


//
// Forward function declarations
//
void LoadColors();
void LoadFonts();
void SetGlobalRgbBasedOnIndex();
void SetNewColorIndex();
void OnPaint(HWND hwnd);
void OnKeydown(HWND hwnd, WPARAM wp);
void OnDestroy(HWND hwnd);
void OnActivate(HWND hwnd, WPARAM wp);
void OnTimer(HWND hwnd);


//
// Functions
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  LRESULT lResult = TRUE;

  switch(msg)
  {
    case WM_TIMER:
      OnTimer(hwnd);
      break;

    case WM_PAINT:
      OnPaint(hwnd);
      break;

    case WM_ACTIVATE:
      OnActivate(hwnd, wp);
      break;

    case WM_KEYDOWN:
      OnKeydown(hwnd, wp);
      break;
 
    case WM_DESTROY:
      OnDestroy(hwnd);
      break;

    default:
      lResult = DefWindowProc(hwnd, msg, wp, lp);
      break;
  }
  return(lResult);
}


HRESULT ActivatePreviousInstance(const TCHAR* pszClass, const TCHAR* pszTitle, BOOL* pfActivated)
{
  HRESULT hr = S_OK;
  int cTries;
  HANDLE hMutex = NULL;

  *pfActivated = FALSE;
  cTries = 5;
  while(cTries > 0)
  {
    hMutex = CreateMutex(NULL, FALSE, pszClass); // NOTE: We don't want to own the object.
    if(NULL == hMutex)
    {
      hr = E_FAIL;
      goto Exit;
    }

    if(GetLastError() == ERROR_ALREADY_EXISTS)
    {
      HWND hwnd;

      CloseHandle(hMutex);
      hMutex = NULL;

      // There is already an instance of this app
      // running.  Try to bring it to the foreground.

      hwnd = FindWindow(pszClass, pszTitle);
      if(NULL == hwnd)
      {
        // It's possible that the other window is in the process of being created...
        Sleep(500);
        hwnd = FindWindow(pszClass, pszTitle);
      }

      if(NULL != hwnd) 
      {
        // Set the previous instance as the foreground window
        // The "| 0x01" in the code below activates
        // the correct owned window of the
        // previous instance's main window.
        SetForegroundWindow((HWND) (((ULONG) hwnd) | 0x01));

        // We are done.
        *pfActivated = TRUE;
        break;
      }

      // It's possible that the instance we found isn't coming up,
      // but rather is going down.  Try again.
      cTries--;
    }
    else
    {
      // We were the first one to create the mutex
      // so that makes us the main instance.  'leak'
      // the mutex in this function so it gets cleaned
      // up by the OS when this instance exits.
      break;
    }
  }
  if(cTries <= 0)
  {
    // Someone else owns the mutex but we cannot find
    // their main window to activate.
    hr = E_FAIL;
    MessageBox(NULL, TEXT("Unable to reactivate previous instance."), TEXT("Torch Error"), MB_OK);
    goto Exit;
  }

  Exit:
    return(hr);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HWND hwnd = NULL;
    BOOL fActivated;
    WNDCLASS wc;
    HWND hwndMain;
    g_hInst = hInstance;

    if(FAILED(ActivatePreviousInstance(g_szAppWndClass, g_szAppTitle, &fActivated)) || fActivated)
    {
      // Previous instance could not be actived or was indeed activated - either way shut me down
      return(0);
    }

    // Register our main window's class.
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hInstance = g_hInst;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szAppWndClass;
    if(!RegisterClass(&wc))
    {
      MessageBox(NULL, TEXT("Unable to register the main window class."), TEXT("Torch Error"), MB_OK);
      return(0);
    }

    // Create the main window - use defaults for now to get a handle (events: 1)
    hwndMain = CreateWindow(
      g_szAppWndClass,
      g_szAppTitle,
      WS_CLIPCHILDREN,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      g_hInst,
      NULL);
    if(!hwndMain)
    {
      MessageBox(NULL, TEXT("Unable to create the main window."), TEXT("Torch Error"), MB_OK);
      return(0);
    }

    // Load the colours table and the font
    LoadColors();
    LoadFonts();

    // Get the monitor info
    HMONITOR hm = MonitorFromWindow(hwndMain, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hm, &mi);

    // Now move the window to full screen
    MoveWindow(hwndMain, 0, 0, mi.rcMonitor.right, mi.rcMonitor.bottom, FALSE);

    // Show the window  (events: 3, 5, 134, 783, 6, 641, 642, 7, 71)
    ShowWindow(hwndMain, nCmdShow);

    // Paint the window  (events: 15, 20)
    UpdateWindow(hwndMain);

    // Seed the random number generator
    srand((unsigned)GetTickCount());

    // Pump messages until a PostQuitMessage.
    while(GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage (&msg);
      DispatchMessage(&msg);
    }
    return msg.wParam;
}


void LoadColors()
{
  g_Color[0] = RGB(0, 0, 0);       // Black
  g_Color[1] = RGB(128, 0, 0);     // Maroon
  g_Color[2] = RGB(0, 128, 0);     // Green
  g_Color[3] = RGB(128, 128, 0);   // Olive
  g_Color[4] = RGB(0, 0, 128);     // Navy
  g_Color[5] = RGB(128, 0, 128);   // Purple
  g_Color[6] = RGB(0, 128, 128);   // Teal
  g_Color[7] = RGB(192, 192, 192); // Silver
  g_Color[8] = RGB(128, 128, 128); // Gray
  g_Color[9] = RGB(255, 0, 0);     // Red
  g_Color[10] = RGB(0, 255, 0);    // Lime
  g_Color[11] = RGB(255, 255, 0);  // Yellow
  g_Color[12] = RGB(0, 0, 255);    // Blue
  g_Color[13] = RGB(255, 0, 255);  // Magenta
  g_Color[14] = RGB(0, 255, 255);  // Aqua
  g_Color[15] = RGB(255, 255, 255);// White
}


void LoadFonts()
{
  LOGFONT lf;
  ZeroMemory((PVOID)&lf, (SIZE_T)sizeof(LOGFONT));
  lf.lfHeight = g_FontHeight;
  lf.lfWeight = FW_NORMAL;
  lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = DEFAULT_QUALITY;
  lf.lfPitchAndFamily = FF_SWISS;
  UINT strlength;
  StringCchLength(TEXT("Nina"), STRSAFE_MAX_CCH, &strlength);
  StringCchCopy(lf.lfFaceName, strlength + 1, TEXT("Nina"));

  g_hFont = CreateFontIndirect(&lf);
}


void SetGlobalRgbBasedOnIndex()
{
  g_Red = GetRValue(g_Color[g_ColorIndex]);
  g_Green = GetGValue(g_Color[g_ColorIndex]);
  g_Blue = GetBValue(g_Color[g_ColorIndex]);
}


void SetNewColorIndex()
{
  g_OldColorIndex = g_ColorIndex;
  while(g_ColorIndex == g_OldColorIndex)
  {
    g_ColorIndex = rand() % g_ColorTableSize;
  }
}


//
// Events
//
void OnPaint(HWND hwnd)
{
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rc;
  HBRUSH hBrush;

  // Get a handle on the DC and create a brush
  hdc = BeginPaint(hwnd, &ps);
  GetClientRect(hwnd, &rc);
  RECT rectTR;
  RECT rectBL;
  
  rectTR.top = rc.top;
  rectTR.left = rc.right / 2;
  rectTR.right = rc.right;
  rectTR.bottom = rc.bottom / 2;
  rectBL.top = rectTR.bottom;
  rectBL.left = rc.left;
  rectBL.right = rectTR.left;
  rectBL.bottom = rc.bottom;

  // Paint the whole screen with current RGB colours
  hBrush = CreateSolidBrush(RGB((BYTE)g_Red, (BYTE)g_Green, (BYTE)g_Blue));
  FillRect(hdc, &rc, hBrush);
  DeleteObject(hBrush);

  if(g_DiscoMode)
  {
    // Paint the top right and bottom left in the old colours
    hBrush = CreateSolidBrush(RGB(GetRValue(g_Color[g_OldColorIndex]) , GetGValue(g_Color[g_OldColorIndex]), GetBValue(g_Color[g_OldColorIndex])));
    FillRect(hdc, &rectTR, hBrush);
    FillRect(hdc, &rectBL, hBrush);
    DeleteObject(hBrush);
  }

  // Do we show rgb values in text?
  if(g_ShowRgb)
  {
    // Get the text and its length to put on the screen
    TCHAR buffer[64];
    StringCbPrintf(buffer, sizeof(buffer), TEXT("Version: 1.5\nRGB: (%d,%d,%d)"), g_Red, g_Green, g_Blue);
    UINT strlength;
    StringCchLength(buffer, STRSAFE_MAX_CCH, &strlength);

    // Select the font we prepared earlier
    SelectObject(hdc, (HGDIOBJ)g_hFont);

    // New rectangle to position the text in
    RECT rctext;
    rctext.left = g_TextLeftOffset;
    rctext.top = g_TextTopOffset;
    rctext.right = rc.right - g_TextLeftOffset;
    rctext.bottom = rc.bottom - g_TextTopOffset;

    // Draw in the text with transparent background
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, buffer, strlength, &rctext, DT_LEFT | DT_NOCLIP);
  }
  EndPaint (hwnd, &ps);
}


void OnKeydown(HWND hwnd, WPARAM wp)
{
  /*
  TCHAR buffer[255];
  StringCbPrintf(buffer, sizeof(buffer), TEXT("%d"), wp);
  MessageBox(hwnd, buffer, TEXT("Key Code"), MB_OK);
  */

  switch(wp)
  {
    case 38: // key up
      ReleasePowerRequirement(g_hPower);
      DestroyWindow(hwnd);
      break;

    case 119: // *
	    if(!g_DiscoMode)
      {
        g_Red -= g_ColourDelta;
        g_Green -= g_ColourDelta;
        g_Blue -= g_ColourDelta;
        if(g_Red < COLOR_MIN) g_Red = COLOR_MIN;
        if(g_Green < COLOR_MIN) g_Green = COLOR_MIN;
        if(g_Blue < COLOR_MIN) g_Blue = COLOR_MIN;
        InvalidateRect(hwnd, NULL, NULL);
      }
      else
      {
        KillTimer(hwnd, 1);
        g_DiscoMode = FALSE;
        g_DiscoSpeed += g_DiscoSpeedDelta;
        g_DiscoSpeed = g_DiscoSpeed > g_DiscoSpeedMax ? g_DiscoSpeedMax : g_DiscoSpeed;
        SetTimer(hwnd, 1, g_DiscoSpeed, (TIMERPROC)NULL);
        g_DiscoMode = TRUE;

      }
      break;

    case 120: // #
	    if(!g_DiscoMode)
      {
        g_Red += g_ColourDelta;
        g_Green += g_ColourDelta;
        g_Blue += g_ColourDelta;
        if(g_Red > COLOR_MAX) g_Red = COLOR_MAX;
        if(g_Green > COLOR_MAX) g_Green = COLOR_MAX;
        if(g_Blue > COLOR_MAX) g_Blue = COLOR_MAX;
        InvalidateRect(hwnd, NULL, NULL);
      }
      else
      {
        KillTimer(hwnd, 1);
        g_DiscoMode = FALSE;
        g_DiscoSpeed -= g_DiscoSpeedDelta;
        g_DiscoSpeed = g_DiscoSpeed < g_DiscoSpeedMin ? g_DiscoSpeedMin : g_DiscoSpeed;
        SetTimer(hwnd, 1, g_DiscoSpeed, (TIMERPROC)NULL);
        g_DiscoMode = TRUE;

      }
      break;

    case 48: // 0
      if(g_DiscoMode)
      {
        KillTimer(hwnd, 1);
        g_DiscoMode = FALSE;
      }
      else
      {
        SetTimer(hwnd, 1, g_DiscoSpeed, (TIMERPROC)NULL);
        g_DiscoMode = TRUE;
      }
      break;

    case 49: // 1
	    if(!g_DiscoMode)
      {
        g_Red -= g_ColourDelta;
        if(g_Red < COLOR_MIN) g_Red = COLOR_MIN;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 50: // 2
	    if(!g_DiscoMode)
      {
        g_Red = COLOR_MAX;
        g_Green = COLOR_MAX;
        g_Blue = COLOR_MAX;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 51: // 3
	    if(!g_DiscoMode)
      {
        g_Red += g_ColourDelta;
        if(g_Red > COLOR_MAX) g_Red = COLOR_MAX;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 52: // 4
	    if(!g_DiscoMode)
      {
        g_Green -= g_ColourDelta;
        if(g_Green < COLOR_MIN) g_Green = COLOR_MIN;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 53: // 5
      g_ShowRgb = g_ShowRgb ? FALSE : TRUE;
      InvalidateRect(hwnd, NULL, NULL);
      break;

    case 54: // 6
	    if(!g_DiscoMode)
      {
        g_Green += g_ColourDelta;
        if(g_Green > COLOR_MAX) g_Green = COLOR_MAX;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 55: // 7
	    if(!g_DiscoMode)
      {
        g_Blue -= g_ColourDelta;
        if(g_Blue < COLOR_MIN) g_Blue = COLOR_MIN;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 56: // 8
	    if(!g_DiscoMode)
      {
        if(g_ColorIndex == g_ColorTableSize - 1)
        {
          g_ColorIndex = 0;
        }
        else
        {
          g_ColorIndex++;
        }
        SetGlobalRgbBasedOnIndex();
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;

    case 57: // 9
      if(!g_DiscoMode)
      {
        g_Blue += g_ColourDelta;
        if(g_Blue > COLOR_MAX) g_Blue = COLOR_MAX;
        InvalidateRect(hwnd, NULL, NULL);
      }
      break;
  }
}


void OnDestroy(HWND hwnd)
{
  // Final chance to tidy up, let's be extra safe
  KillTimer(hwnd, 1);
  ReleasePowerRequirement(g_hPower);
  PostQuitMessage(0);
}


void OnActivate(HWND hwnd, WPARAM wp)
{
  if(wp == WA_INACTIVE) // Focus lost
  {
    KillTimer(hwnd, 1); // To be friendly to the phone
    ReleasePowerRequirement(g_hPower);
  }
  else // Focus got
  {
    if(g_DiscoMode)
    {
      // Switch it back on again now that we have focus back
      SetTimer(hwnd, 1, g_DiscoSpeed, (TIMERPROC)NULL);
    }
    g_hPower = SetPowerRequirement(TEXT("BKL1:"), g_cps, POWER_NAME, NULL, 0);
  }
}


void OnTimer(HWND hwnd)
{
  SetNewColorIndex();
  SetGlobalRgbBasedOnIndex();
  InvalidateRect(hwnd, NULL, NULL);
}

