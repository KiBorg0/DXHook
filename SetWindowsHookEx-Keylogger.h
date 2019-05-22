// SetWindowsHookEx-Keylogger.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <Windows.h>
#include <QDebug>
//#include "time.h"
//#include "string"
//#include <sstream>
//#include <fstream>
//#include <iostream>

// KeyBoard hook handle in global scope
HHOOK KeyboardHook;
// Shift Key 
bool shift = FALSE;
// Windows Title Text -260 char-
char cWindow[1000];
// NULL is ok
HWND lastWindow = NULL;
// File Path
//std::string fileName = "C:\\test.txt";

// All hooks must be unhooked!
void unhookKeyboard()
{
	UnhookWindowsHookEx(KeyboardHook);
	exit(0);
}

//std::string HookCode(DWORD code, BOOL caps, BOOL shift)
//{
//	/*
//	Translate the return code from hook and
//	return the std::string rep of the the code.
//	ex. 0x88 -> "[SHIFT]"
//	caps = Caps lock on
//	shift = Shift key pressed
//	WinUser.h = define statments
//	LINK = https://msdn.microsoft.com/en-us/library/dd375731(v=VS.85).aspx
//	*/
//	std::string key;
//	switch (code) // SWITCH ON INT
//	{
//		// Char keys for ASCI
//		// No VM Def in header
//		case 0x41: key = caps ? (shift ? "a" : "A") : (shift ? "A" : "a"); break;
//		case 0x42: key = caps ? (shift ? "b" : "B") : (shift ? "B" : "b"); break;
//		case 0x43: key = caps ? (shift ? "c" : "C") : (shift ? "C" : "c"); break;
//		case 0x44: key = caps ? (shift ? "d" : "D") : (shift ? "D" : "d"); break;
//		case 0x45: key = caps ? (shift ? "e" : "E") : (shift ? "E" : "e"); break;
//		case 0x46: key = caps ? (shift ? "f" : "F") : (shift ? "F" : "f"); break;
//		case 0x47: key = caps ? (shift ? "g" : "G") : (shift ? "G" : "g"); break;
//		case 0x48: key = caps ? (shift ? "h" : "H") : (shift ? "H" : "h"); break;
//		case 0x49: key = caps ? (shift ? "i" : "I") : (shift ? "I" : "i"); break;
//		case 0x4A: key = caps ? (shift ? "j" : "J") : (shift ? "J" : "j"); break;
//		case 0x4B: key = caps ? (shift ? "k" : "K") : (shift ? "K" : "k"); break;
//		case 0x4C: key = caps ? (shift ? "l" : "L") : (shift ? "L" : "l"); break;
//		case 0x4D: key = caps ? (shift ? "m" : "M") : (shift ? "M" : "m"); break;
//		case 0x4E: key = caps ? (shift ? "n" : "N") : (shift ? "N" : "n"); break;
//		case 0x4F: key = caps ? (shift ? "o" : "O") : (shift ? "O" : "o"); break;
//		case 0x50: key = caps ? (shift ? "p" : "P") : (shift ? "P" : "p"); break;
//		case 0x51: key = caps ? (shift ? "q" : "Q") : (shift ? "Q" : "q"); break;
//		case 0x52: key = caps ? (shift ? "r" : "R") : (shift ? "R" : "r"); break;
//		case 0x53: key = caps ? (shift ? "s" : "S") : (shift ? "S" : "s"); break;
//		case 0x54: key = caps ? (shift ? "t" : "T") : (shift ? "T" : "t"); break;
//		case 0x55: key = caps ? (shift ? "u" : "U") : (shift ? "U" : "u"); break;
//		case 0x56: key = caps ? (shift ? "v" : "V") : (shift ? "V" : "v"); break;
//		case 0x57: key = caps ? (shift ? "w" : "W") : (shift ? "W" : "w"); break;
//		case 0x58: key = caps ? (shift ? "x" : "X") : (shift ? "X" : "x"); break;
//		case 0x59: key = caps ? (shift ? "y" : "Y") : (shift ? "Y" : "y"); break;
//		case 0x5A: key = caps ? (shift ? "z" : "Z") : (shift ? "Z" : "z"); break;
//		// Sleep Key
//		case VK_SLEEP: key = "[SLEEP]"; break;
//		// Num Keyboard
//		case VK_NUMPAD0:  key = "0"; break;
//		case VK_NUMPAD1:  key = "1"; break;
//		case VK_NUMPAD2 : key = "2"; break;
//		case VK_NUMPAD3:  key = "3"; break;
//		case VK_NUMPAD4:  key = "4"; break;
//		case VK_NUMPAD5:  key = "5"; break;
//		case VK_NUMPAD6:  key = "6"; break;
//		case VK_NUMPAD7:  key = "7"; break;
//		case VK_NUMPAD8:  key = "8"; break;
//		case VK_NUMPAD9:  key = "9"; break;
//		case VK_MULTIPLY: key = "*"; break;
//		case VK_ADD:      key = "+"; break;
//		case VK_SEPARATOR: key = "-"; break;
//		case VK_SUBTRACT: key = "-"; break;
//		case VK_DECIMAL:  key = "."; break;
//		case VK_DIVIDE:   key = "/"; break;
//		// Function Keys
//		case VK_F1:  key = "[F1]"; break;
//		case VK_F2:  key = "[F2]"; break;
//		case VK_F3:  key = "[F3]"; break;
//		case VK_F4:  key = "[F4]"; break;
//		case VK_F5:  key = "[F5]"; break;
//		case VK_F6:  key = "[F6]"; break;
//		case VK_F7:  key = "[F7]"; break;
//		case VK_F8:  key = "[F8]"; break;
//		case VK_F9:  key = "[F9]"; break;
//		case VK_F10:  key = "[F10]"; break;
//		case VK_F11:  key = "[F11]"; break;
//		case VK_F12:  key = "[F12]"; break;
//		case VK_F13:  key = "[F13]"; break;
//		case VK_F14:  key = "[F14]"; break;
//		case VK_F15:  key = "[F15]"; break;
//		case VK_F16:  key = "[F16]"; break;
//		case VK_F17:  key = "[F17]"; break;
//		case VK_F18:  key = "[F18]"; break;
//		case VK_F19:  key = "[F19]"; break;
//		case VK_F20:  key = "[F20]"; break;
//		case VK_F21:  key = "[F22]"; break;
//		case VK_F22:  key = "[F23]"; break;
//		case VK_F23:  key = "[F24]"; break;
//		case VK_F24:  key = "[F25]"; break;
//		// Keys
//		case VK_NUMLOCK: key = "[NUM-LOCK]"; break;
//		case VK_SCROLL:  key = "[SCROLL-LOCK]"; break;
//		case VK_BACK:    key = "[BACK]"; break;
//		case VK_TAB:     key = "[TAB]"; break;
//		case VK_CLEAR:   key = "[CLEAR]"; break;
//		case VK_RETURN:  key = "[ENTER]"; break;
//		case VK_SHIFT:   key = "[SHIFT]"; break;
//		case VK_CONTROL: key = "[CTRL]"; break;
//		case VK_MENU:    key = "[ALT]"; break;
//		case VK_PAUSE:   key = "[PAUSE]"; break;
//		case VK_CAPITAL: key = "[CAP-LOCK]"; break;
//		case VK_ESCAPE:  key = "[ESC]"; break;
//		case VK_SPACE:   key = "[SPACE]"; break;
//		case VK_PRIOR:   key = "[PAGEUP]"; break;
//		case VK_NEXT:    key = "[PAGEDOWN]"; break;
//		case VK_END:     key = "[END]"; break;
//		case VK_HOME:    key = "[HOME]"; break;
//		case VK_LEFT:    key = "[LEFT]"; break;
//		case VK_UP:      key = "[UP]"; break;
//		case VK_RIGHT:   key = "[RIGHT]"; break;
//		case VK_DOWN:    key = "[DOWN]"; break;
//		case VK_SELECT:  key = "[SELECT]"; break;
//		case VK_PRINT:   key = "[PRINT]"; break;
//		case VK_SNAPSHOT: key = "[PRTSCRN]"; break;
//		case VK_INSERT:  key = "[INS]"; break;
//		case VK_DELETE:  key = "[DEL]"; break;
//		case VK_HELP:    key = "[HELP]"; break;
//		// Number Keys with shift
//		case 0x30:  key = shift ? "!" : "1"; break;
//		case 0x31:  key = shift ? "@" : "2"; break;
//		case 0x32:  key = shift ? "#" : "3"; break;
//		case 0x33:  key = shift ? "$" : "4"; break;
//		case 0x34:  key = shift ? "%" : "5"; break;
//		case 0x35:  key = shift ? "^" : "6"; break;
//		case 0x36:  key = shift ? "&" : "7"; break;
//		case 0x37:  key = shift ? "*" : "8"; break;
//		case 0x38:  key = shift ? "(" : "9"; break;
//		case 0x39:  key = shift ? ")" : "0"; break;
//		// Windows Keys
//		case VK_LWIN:     key = "[WIN]"; break;
//		case VK_RWIN:     key = "[WIN]"; break;
//		case VK_LSHIFT:   key = "[SHIFT]"; break;
//		case VK_RSHIFT:   key = "[SHIFT]"; break;
//		case VK_LCONTROL: key = "[CTRL]"; break;
//		case VK_RCONTROL: key = "[CTRL]"; break;
//		// OEM Keys with shift
//		case VK_OEM_1:      key = shift ? ":" : ";"; break;
//		case VK_OEM_PLUS:   key = shift ? "+" : "="; break;
//		case VK_OEM_COMMA:  key = shift ? "<" : ","; break;
//		case VK_OEM_MINUS:  key = shift ? "_" : "-"; break;
//		case VK_OEM_PERIOD: key = shift ? ">" : "."; break;
//		case VK_OEM_2:      key = shift ? "?" : "/"; break;
//		case VK_OEM_3:      key = shift ? "~" : "`"; break;
//		case VK_OEM_4:      key = shift ? "{" : "["; break;
//		case VK_OEM_5:      key = shift ? "\\" : "|"; break;
//		case VK_OEM_6:      key = shift ? "}" : "]"; break;
//		case VK_OEM_7:      key = shift ? "'" : "'"; break; //TODO: Escape this char: "
//		// Action Keys
//		case VK_PLAY:       key = "[PLAY]";
//		case VK_ZOOM:       key = "[ZOOM]";
//		case VK_OEM_CLEAR:  key = "[CLEAR]";
//		case VK_CANCEL:     key = "[CTRL-C]";

//		default: key = "[UNK-KEY]";break;
//	}
//	return key;

//}

//std::string Dayofweek(int code)
//{
//	// Return Day of the year in text
//	std::string name;
//	switch (code)
//	{
//		case 0: name = "[SUNDAY]"; break;
//		case 1: name = "[MONDAY]"; break;
//		case 2: name = "[TUESDAY]"; break;
//		case 3: name = "[WENSDAY]"; break;
//		case 4: name = "[THURSDAY]"; break;
//		case 5: name = "[FRIDAY]"; break;
//		case 6: name = "[SATURDAY]"; break;
//		default:
//			name = "[UNKOWN]";
//	}
//	return name;
//}

BYTE CodeHP[4] = {0x84, 0xDB, 0x74, 0x19};
BYTE temp4[4] = {0};
BYTE nop_array4[4] = {0x90, 0x90, 0x90, 0x90};
PVOID HPAddr = (PVOID)0x00956596;
bool do_ones = true;

LRESULT CALLBACK HookProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
	/*
	SHORT WINAPI GetKeyState(
		_In_ int nVirtKeydasdsad
		);
	*/
//	std::ofstream myfile(fileName, std::ios::out | std::ios::app);
//	bool caps = FALSE;
//	SHORT capsShort = GetKeyState(VK_CAPITAL);
//	std::string outPut;
//	std::stringstream temp;
//	if (capsShort > 0)
//	{
//		// If the high-order bit is 1, the key is down; otherwise, it is up
//		caps = TRUE;
//	}
	/* 
	WH_KEYBOARD_LL uses the LowLevelKeyboardProc Call Back
	LINK = https://msdn.microsoft.com/en-us/library/windows/desktop/ms644985(v=vs.85).aspx
	*/
	// LowLevelKeyboardProc Structure 
	/*
	typedef struct tagKBDLLHOOKSTRUCT {
		DWORD     vkCode;
		DWORD     scanCode;
		DWORD     flags;
		DWORD     time;
		ULONG_PTR dwExtraInfo;
		} KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT, *LPKBDLLHOOKSTRUCT;
	*/
    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
//    INPUT inputs[1];

//    inputs[0].type = INPUT_KEYBOARD;
//    inputs[0].ki.wScan = 0;
//    inputs[0].ki.dwFlags = 0;
//    inputs[0].ki.time = 0;
//    inputs[0].ki.dwExtraInfo = 0;

//    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
//        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
//    }
//    if((p->flags & LLKHF_INJECTED) == 0){
//        switch (p->vkCode) {
//        case 0x41: inputs[0].ki.wVk=VK_LEFT ; SendInput(1, inputs, sizeof (INPUT)); return 1; break;
//        case 0x44: inputs[0].ki.wVk=VK_RIGHT; SendInput(1, inputs, sizeof (INPUT)); return 1; break;
//        case 0x53: inputs[0].ki.wVk=VK_DOWN ; SendInput(1, inputs, sizeof (INPUT)); return 1; break;
//        case 0x57: inputs[0].ki.wVk=VK_UP   ; SendInput(1, inputs, sizeof (INPUT)); return 1; break;
//        default: break;
//        }
//    }

//    if (p->vkCode == 'A' && ) {
//        inputs[0].ki.wVk = 'B';
//        ret = SendInput(1, inputs, sizeof (INPUT));
//        return 1;
//    } else if (p->vkCode == 'B' && (p->flags & LLKHF_INJECTED) == 0) {
//        inputs[0].ki.wVk = 'A';
//        ret = SendInput(1, inputs, sizeof (INPUT));
//        return 1;
//    }


	// Do the wParam and lParam parameters contain information about a keyboard message.
	if (nCode == HC_ACTION)
    {
//        &&(GetAsyncKeyState(VK_SPACE) & 0x8000)
        if((GetAsyncKeyState(VK_CONTROL) & 0x8000)){
//            qDebug() << "key pressed" << (PVOID)p->vkCode;
            if(wParam == WM_KEYDOWN){
                switch (p->vkCode) {
//                case 0x41: qDebug() << "A"; return CallNextHookEx(NULL, VK_LEFT , wParam, lParam); break;
//                case 0x44: qDebug() << "D"; return CallNextHookEx(NULL, VK_RIGHT, wParam, lParam); break;
//                case 0x53: qDebug() << "S"; return CallNextHookEx(NULL, VK_DOWN , wParam, lParam); break;
//                case 0x57: qDebug() << "W"; return CallNextHookEx(NULL, VK_UP   , wParam, lParam); break;
                case 0x41: keybd_event(VK_LEFT , 0x4B, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                case 0x44: keybd_event(VK_RIGHT, 0x4D, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                case 0x53: keybd_event(VK_DOWN , 0x50, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                case 0x57: keybd_event(VK_UP   , 0x48, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                default: break;
                }
                do_ones = true;
//                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }else if(wParam == WM_KEYUP){
                switch (p->vkCode) {
                case 0x41: keybd_event(VK_LEFT , 0x4B, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                case 0x44: keybd_event(VK_RIGHT, 0x4D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                case 0x53: keybd_event(VK_DOWN , 0x50, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                case 0x57: keybd_event(VK_UP   , 0x48, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                default: break;
                }
//                do_ones = false;
//                return 0;
            }
        }else if(do_ones){
            keybd_event(VK_LEFT , 0x4B, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            keybd_event(VK_RIGHT, 0x4D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            keybd_event(VK_DOWN , 0x50, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            keybd_event(VK_UP   , 0x48, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            do_ones = false;
        }
        memcpy(temp4, HPAddr, 4);
        DWORD dwOldProtect = 0;
        if(GetAsyncKeyState(0x59) & 0x8000){
            if(memcmp(temp4, CodeHP, 4)==0){
                VirtualProtect(HPAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
                memcpy(HPAddr, nop_array4, 4);
                VirtualProtect(HPAddr, 4, dwOldProtect, 0);
            }
        }else{
            if(memcmp(temp4, CodeHP, 4)==0){
                VirtualProtect(HPAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
                memcpy(HPAddr, CodeHP, 4);
                VirtualProtect(HPAddr, 4, dwOldProtect, 0);
            }
        }
//        if(p->vkCode == VK_CAPITAL/*VK_LSHIFT*/){
//            memcpy(temp4, HPAddr, 4);
//            DWORD dwOldProtect = 0;
//            VirtualProtect(HPAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
//            if((GetKeyState(VK_CAPITAL) & 0x0001)/*wParam == WM_KEYDOWN*/&&memcmp(temp4, CodeHP, 4)==0){
////                    qDebug() << "Show HP bars";
//                memcpy(HPAddr, nop_array4, 4);
//            }else if((GetKeyState(VK_CAPITAL) & 0x0001)==0/*wParam == WM_KEYUP*/){
////                    qDebug() << "Hide HP bars";
//                memcpy(HPAddr, CodeHP, 4);
//            }
//            VirtualProtect(HPAddr, 4, dwOldProtect, 0);
//        }

        // double fleet of foot
        // если при нажатом контроле нажимается клавиша D, то идет нажатие D TAB D TAB
//        if((GetAsyncKeyState(VK_SHIFT) & 0x8000) && wParam==WM_KEYDOWN && p->vkCode==0x44){
//            keybd_event(0x44, 0x20, KEYEVENTF_EXTENDEDKEY | 0, 0);
//            keybd_event(0x44, 0xA0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
//            keybd_event(VK_TAB , 0x0F, KEYEVENTF_EXTENDEDKEY | 0, 0);
//            keybd_event(VK_TAB , 0x8F, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
//            keybd_event(0x44, 0x20, KEYEVENTF_EXTENDEDKEY | 0, 0);
//            keybd_event(0x44, 0xA0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
//            keybd_event(VK_TAB , 0x0F, KEYEVENTF_EXTENDEDKEY | 0, 0);
//            keybd_event(VK_TAB , 0x8F, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
//        }

		// Start Loging keys now we are setup
//		if (wParam == WM_SYSKEYDOWN || wParam == WM_KEYDOWN)
//		{
//			// Retrieves a handle to the foreground window (the window with which the user is currently working).
//			HWND currentWindow = GetForegroundWindow();
//			// Check if we need to write new window output
//			if (currentWindow != lastWindow)
//			{
//				/*
//				void WINAPI GetLocalTime(
//					  _Out_ LPSYSTEMTIME lpSystemTime
//					);
//				typedef struct _SYSTEMTIME {
//					WORD wYear;
//					WORD wMonth;
//					WORD wDayOfWeek;
//					WORD wDay;
//					WORD wHour;
//					WORD wMinute;
//					WORD wSecond;
//					WORD wMilliseconds;
//					} SYSTEMTIME, *PSYSTEMTIME;
//				*/
//				//LPSYSTEMTIME t;
//				SYSTEMTIME t;
//				GetLocalTime(&t);
//				int day = t.wDay;
//				int month = t.wMonth;
//				int year = t.wYear;
//				int hour = t.wHour;
//				int min = t.wMinute;
//				int sec = t.wSecond;
//				int dayName = t.wDayOfWeek;
//				// Build our output header
//				temp << "\n\n[+] " << Dayofweek(dayName) << " - " << day << "/" << month << "/" << year << "  ";
//				temp << hour << ":" << min << ":" << sec;
//				outPut.append(temp.str());
//				temp.clear();
//				/*
//				int WINAPI GetWindowText(
//				  _In_  HWND   hWnd,
//				  _Out_ LPTSTR lpString,
//				  _In_  int    nMaxCount
//				);
//				*/
//				int c = GetWindowTextA(GetForegroundWindow(), cWindow, sizeof(cWindow));
//				std::cout << c;
//				temp << " - Current Window: " << cWindow << "\n\n";
//				//outPut.append(temp.str());
//				std::cout << temp.str() << std::endl;
//				// Setup for next CallBack
//				lastWindow = currentWindow;
//			}
//			// Now capture keys
//			if (p->vkCode)
//			{
//				//outPut.append(HookCode(p->vkCode, caps, shift));
//				temp.clear();
//				temp << HookCode(p->vkCode, caps, shift);
//				std::cout << temp.str();
//			}
//			// Final output logic
//		}
	}
	// hook procedure must pass the message *Always*
//	myfile.close();
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


//int main()
//{
//	std::cout << "[*] Starting KeyCapture" << std::endl;
//	/*
//	HHOOK WINAPI SetWindowsHookEx(
//	  _In_ int       idHook,
//	  _In_ HOOKPROC  lpfn,
//	  _In_ HINSTANCE hMod,
//	  _In_ DWORD     dwThreadId
//	);
//	*/
//	// Start the hook of the keyboard
//	KeyboardHook = SetWindowsHookEx(
//		WH_KEYBOARD_LL, // low-level keyboard input events
//		HookProcedure, // pointer to the hook procedure
//		GetModuleHandle(NULL), // A handle to the DLL containing the hook procedure
//		NULL //desktop apps, if this parameter is zero
//		);
//	if (!KeyboardHook){
//		// Hook returned NULL and failed
//		std::cout << "[!] Failed to get handle from SetWindowsHookEx()" << std::endl;
//	}
//	else {
//		std::cout << "[*] KeyCapture handle ready" << std::endl;
//		// http://www.winprog.org/tutorial/message_loop.html
//		MSG Msg;
//		while (GetMessage(&Msg, NULL, 0, 0) > 0)
//		{
//			TranslateMessage(&Msg);
//			DispatchMessage(&Msg);
//		}
//	}
//	unhookKeyboard();
//	// Exit if failure
//    return 0;
//}
