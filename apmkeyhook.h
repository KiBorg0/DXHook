#include <windows.h>

#define KEY_FIRST_PRESS 3221225472

//DWORD idThread = NULL;
//LPLONG PTotalActions;

BYTE CodeHP[4] = {0x84, 0xDB, 0x74, 0x19};
BYTE temp4[4] = {0};
BYTE nop_array4[4] = {0x90, 0x90, 0x90, 0x90};
PVOID HPAddr = (PVOID)0x00956596;
bool do_ones = true;
LPLONG PTotalActions;
BOOL chatFilter_active = FALSE;
DWORD idThread = NULL;

void addAction() {
    InterlockedIncrement(PTotalActions);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if(nCode>=0) {
        if((lParam & KEY_FIRST_PRESS) == 0) {
            if(wParam == VK_RETURN)
                chatFilter_active = !chatFilter_active;
            if(!chatFilter_active && !(wParam == VK_TAB || wParam == VK_SHIFT || wParam == VK_CONTROL ||
                wParam == VK_MENU || wParam == VK_LWIN || wParam == VK_RWIN ||
                wParam == VK_CAPITAL || wParam == VK_RETURN))
                addAction();
        }
        PostThreadMessage(idThread, WH_KEYBOARD, wParam, lParam);
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        // Do the wParam and lParam parameters contain information about a keyboard message.
        if (nCode == HC_ACTION)
        {
            if((GetAsyncKeyState(VK_CONTROL) & 0x8000)){
                if(wParam == WM_KEYDOWN){
                    switch (p->vkCode) {
                    case 0x41: keybd_event(VK_LEFT , 0x4B, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                    case 0x44: keybd_event(VK_RIGHT, 0x4D, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                    case 0x53: keybd_event(VK_DOWN , 0x50, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                    case 0x57: keybd_event(VK_UP   , 0x48, KEYEVENTF_EXTENDEDKEY | 0, 0); break;
                    default: break;
                    }
                    do_ones = true;
                }else if(wParam == WM_KEYUP){
                    switch (p->vkCode) {
                    case 0x41: keybd_event(VK_LEFT , 0x4B, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                    case 0x44: keybd_event(VK_RIGHT, 0x4D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                    case 0x53: keybd_event(VK_DOWN , 0x50, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                    case 0x57: keybd_event(VK_UP   , 0x48, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); break;
                    default: break;
                    }
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
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if(code>=0) {
        if(wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP || wParam == WM_XBUTTONUP)
            addAction();
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}



//class APMKeyHook {
//public:
////    APMKeyHook(long *actions_num);
////    ~APMKeyHook();
//    static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
//    static LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);
//    static DWORD idThread;
//    static LPLONG PTotalActions;

//private:
////    static HANDLE hSharedMemory;
//    static BOOL chatFilter_active;
//    static void addAction();
//};
