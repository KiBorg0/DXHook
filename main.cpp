//#define D3D_DEBUG_INFO

#include <qglobal.h>
#include <windows.h>
//#include <winbase.h>
//#include <Processthreadsapi.h>
#include <WinBase.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr9.h>
#include <QString>
#include <tchar.h>
//#include <stdio.h>
//#include <strsafe.h>
#include <QSettings>
#include <QProcess>
#include <QDebug>
#include <fstream>
#include "cRender.h"
#include "logger.h"
#include "cMemory.h"
#include "hooks.h"
#include <tlhelp32.h>
//#include "SetWindowsHookEx-Keylogger.h"
#include "apmkeyhook.h"
//#include "..\SSStats\systemwin32.h"

HHOOK keyboardHook;
HHOOK mouseHook;

Logger logger;
cRender Render;
CDraw Draw;

using namespace std;
#define D3DparamX		, UINT paramx
#define D3DparamvalX	, paramx
#define BUFSIZE 4096
void paint(LPDIRECT3DDEVICE9 pDevice);


// Ensure 'original function' symbols are defined. (see hooks.h)
decltype(hooks::original_present) hooks::original_present = nullptr;
decltype(hooks::original_reset) hooks::original_reset = nullptr;
decltype(hooks::original_release) hooks::original_release = nullptr;
//typedef HRESULT	(WINAPI* oPresent)(LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);


LPD3DXFONT pFont = nullptr;
bool fontInited = false;

HHOOK ExistingKeyboardProc;
LRESULT CALLBACK KeyboardProcLowLevel(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT * hookstruct = (KBDLLHOOKSTRUCT *) (lParam);
    switch( wParam ){
        case WM_KEYDOWN :
            break;
        case WM_SYSKEYDOWN :
            // Take no Action, Signal app to take action in main loop
            if( (((hookstruct->flags)>>5)&1) ){
                // ALT +
                switch( hookstruct->vkCode ){
                    case VK_TAB : // ALT+TAB
//                        cWinBase::SignalKeysAltTab();
                        break;
                    case VK_RETURN : // ALT+ENTER
//                        cWinBase::SignalKeysAltEnter();
//                        cWinBase::SignalKeysAltEnter();
                        break;
                    case VK_ESCAPE : // ALT+ESC
//                        cWinBase::SignalKeysAltEsc();
                        break;
                    case VK_DELETE : // ALT+DEL
//                        cWinBase::SignalKeysCtrAltDel();
                        break;
                };//switch
            }// if alt+
            break;
        case WM_KEYUP :
            break;
        case WM_SYSKEYUP :
            break;
    }
    return CallNextHookEx( ExistingKeyboardProc, nCode, wParam, lParam);
}
int HookKeyboardProc(HINSTANCE hinst)
{
    ExistingKeyboardProc = SetWindowsHookEx( WH_KEYBOARD_LL, // int idHook,
    KeyboardProcLowLevel,//HOOKPROC lpfn,
    hinst,// HINSTANCE hMod,
    0);// DWORD dwThreadId
    if( !ExistingKeyboardProc ){
        //  Failed
        return false;
    }
    else{
        //Succeeded.
        return true;
    }
}

void unhookKeyboard()
{
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
//    exit(0);
}

int UnHookKeyboardProc()
{
    if( ExistingKeyboardProc ){
        BOOL retcode = UnhookWindowsHookEx((HHOOK) KeyboardProcLowLevel);
        if( retcode ){
            // Successfully Un Hooked keyboard routine.
        }
        else{
            // Error Keyboard Not successfully Un hooked!
            // UnhookWindowsHookEx() returned failure!
        }
        return retcode;
    }
    else{
        //Error Keyboard Not successfully hooked!
        // Could not unhook procedure!
        return true;
    }
}

typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT (WINAPI* oReset)( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );
typedef HRESULT	(WINAPI* oPresent)(LPDIRECT3DDEVICE9 pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
typedef HRESULT (WINAPI* oRelease) (LPDIRECT3DDEVICE9);
DWORD WINAPI VirtualMethodTableRepatchingLoopToCounterExtensionRepatching(LPVOID);
oReset pReset = NULL;
oPresent pPresent = NULL;
oRelease pRelease = NULL;
oEndScene pEndScene = NULL;
BYTE CodeFragmentPR[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
BYTE jmpbPR[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectPR = 0;
BYTE CodeFragmentRES[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
BYTE jmpbRES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectRES = 0;
BYTE CodeFragmentES[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
BYTE jmpbES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectES = 0;
BYTE CodeFragmentREL[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
BYTE jmpbREL[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
BYTE fiveBytesOfReset[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
DWORD dwOldProtectREL = 0;

PBYTE ptrToExternalHookFunction = nullptr;
//std::shared_ptr<DWORD> pTestCode = nullptr;

LPDIRECT3DDEVICE9 oldDevice = nullptr;
HANDLE hSharedMemory = nullptr;
PGameInfo lpSharedMemory = nullptr;
int fontSize = 14;
DWORD height, width;
bool closeWithGame = false;
QString version_str = "0.0.0";

bool deviceReseted = false;
bool needAddHook = false;

void initFonts(LPDIRECT3DDEVICE9 pDevice){
    qDebug() << "Device changed from" << oldDevice << "to" << pDevice;
    oldDevice = pDevice;
    // получаем высоту и ширину экрана
    D3DDEVICE_CREATION_PARAMETERS cparams;
    RECT rect;
    pDevice->GetCreationParameters(&cparams);
    GetWindowRect(cparams.hFocusWindow, &rect);
    height = abs(rect.bottom - rect.top);
    width = abs(rect.right - rect.left);
    if(height<640)
        fontSize =  12;
    else if(height<768)
        fontSize =  14;
    else if(height<1024)
        fontSize =  16;
    else if(height<1280)
        fontSize =  18;
    else if(height<1600)
        fontSize =  20;
    else if(height<2200)
        fontSize =  22;

    qDebug() << "Screen size:" << QString("%1x%2").arg(width).arg(height)
             << "Font size:" << fontSize;
    Render.setMenuParams(fontSize, width, height);

    qDebug() << "Release fonts";
    Draw.SetDevice(pDevice);
    Draw.ReleaseFonts();
    Render.setDevice(pDevice);
    Render.ReleaseFonts();

    qDebug() << "Init fonts";
    Draw.AddFont("Gulim", fontSize, false, false);

    bool success = false;
    if(AddFontResourceA("Engine/Locale/English/data/font/engo.ttf"))
        success = Render.AddFont("GothicRus", fontSize, false, false);
    if(!success)
        Render.AddFont("Arial", fontSize, false, false);
    Render.AddFont("Arial", fontSize, false, false);
}

HRESULT STDMETHODCALLTYPE hooks::user_release(LPDIRECT3DDEVICE9 pDevice) {
    qDebug() << "HookedRelease";
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Release TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    if(hr==D3DERR_DEVICENOTRESET&&pFont!=nullptr){
        pFont->Release();
        pFont = nullptr;
    }
    return hooks::original_release(pDevice);
}



HRESULT STDMETHODCALLTYPE hooks::user_present(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) {
//    qDebug() << "PRESENT";
//    HRESULT hr = D3D_OK;
//    HRESULT hr = pDevice->TestCooperativeLevel();
//    if (FAILED(hr))
//        qDebug() << "Present TestCooperativeLevel" << DXGetErrorString9A(hr);

//    if(oldDevice!=pDevice){
//        qDebug() << "Device changed from" << oldDevice << "to" << pDevice;
//        oldDevice = pDevice;
//        if(pFont){
//            pFont->Release();
//            pFont = nullptr;
//        }
//    }
//    if(!pFont){
//        hr = D3DXCreateFont(pDevice, 30, 0, FW_NORMAL, 1, false,
//        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
//        DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &pFont);
//        if (FAILED(hr))
//            qDebug() << "Could not create font." << DXGetErrorString9A(hr);
//    }
//    if(SUCCEEDED(hr)){
//        RECT rect = {0,0,0,0};
//        SetRect(&rect, 0, 0, 300, 100);
//        int height = pFont->DrawText(nullptr, L"Hello, World!", -1, &rect,
//            DT_LEFT | DT_NOCLIP, -1);
//        if(!height)
//            qDebug() << "Could not draw text.";
//    }
    paint(pDevice);
    return hooks::original_present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

#ifndef TH32CS_SNAPMODULE32
#define TH32CS_SNAPMODULE32 0x8
#endif

bool externalHookIsValid()
{
    HANDLE hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 0);
    if (hthSnapshot!=INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 me = { sizeof(me) };
        Module32First(hthSnapshot, &me);
        do{
//            qDebug() << QString::fromWCharArray(me.szModule);
            if(ptrToExternalHookFunction>me.modBaseAddr&&ptrToExternalHookFunction<(me.modBaseAddr+me.modBaseSize)){
                qDebug() << QString::fromWCharArray(me.szModule) << ptrToExternalHookFunction
                         << me.modBaseAddr << (PDWORD)me.modBaseSize
                         << (PDWORD)(me.modBaseAddr+me.modBaseSize);
                CloseHandle(hthSnapshot);
                return true;
            }
        }while(Module32Next(hthSnapshot, &me));
        CloseHandle(hthSnapshot);
    }else{
        qDebug() << "hthSnapshot is INVALID_HANDLE_VALUE";
        return true;
    }
    return false;
}

HRESULT STDMETHODCALLTYPE hooks::user_reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    qDebug() << "RESET";
    if(needAddHook){
        // если Reset на момент хука уже был хукнут
        if(CodeFragmentRES[0]==0xE9){
            // то проверим не был ли отключен хук
            if(externalHookIsValid()){
                qDebug() << "External hook is active!";
                // если нет, то хукаем резет этим хуком, для того чтобы запустить его в дальнейшем
                memcpy((void *)pReset, CodeFragmentRES, 5);
            }else{
                qDebug() << "External hook is not active!";
                // если да, то CodeFragmentRES больше не содержит код не хукнутой Reset
                memcpy(CodeFragmentRES, (void *)pReset, 5);
                // Reset содержит стим хук, копируем его в jmpbRES, чтобы он запускался в дальнейшем
                memcpy(jmpbRES, fiveBytesOfReset, 5);
                // и пишем в Reset ее изначальный код, чтобы вызывать в дальнейшем
                memcpy((void *)pReset, fiveBytesOfReset, 5);
            }
        }else
            memcpy((void *)pReset, CodeFragmentRES, 5);
    }

    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Reset TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
    Draw.OnLostDevice();
    if(pFont)pFont->OnLostDevice();
    hr = hooks::original_reset(pDevice, pPresentationParameters);

    if(SUCCEEDED(hr)){
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
        Draw.OnResetDevice();
        if(pFont)pFont->OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    if(needAddHook)memcpy((void *)pReset, jmpbRES, 5);
    return hr;
}


// проблема заключается в стимовском оверлее,
// дело в том что выполняя сброс устройства здесь,
// мы не ждем пока освободятся ресурсы стимовского хука
// из за этого резет не выполняется
HRESULT APIENTRY HookedReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters )
{
    qDebug() << "HookedReset";
    memcpy((void *)pReset, CodeFragmentRES, 5);
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Reset TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
    Draw.OnLostDevice();
    if(pFont!=nullptr)pFont->OnLostDevice();
    hr = pDevice->Reset(pPresentationParameters);

    if(SUCCEEDED(hr)) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
        Draw.OnResetDevice();
        if(pFont!=nullptr)pFont->OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    memcpy((void *)pReset, jmpbRES, 5);
    return hr;
}
HRESULT APIENTRY HookedRelease(LPDIRECT3DDEVICE9 pDevice){

    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Release TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    memcpy((void *)pRelease, CodeFragmentREL, 5);
    hr = pDevice->Release();
    memcpy((void *)pRelease, jmpbREL, 5);
    qDebug() << "HookedRelease" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    return hr;
}

HRESULT APIENTRY HookedPresent(LPDIRECT3DDEVICE9 pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
        qDebug() << "Present TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    paint(pDevice);

    memcpy((void *)pPresent, CodeFragmentPR, 5);
    hr = pDevice->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    memcpy((void *)pPresent, jmpbPR, 5);

    if(FAILED(hr))
        qDebug() << "HookedPresent return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    return hr;
}

HRESULT APIENTRY HookedEndScene(LPDIRECT3DDEVICE9 pDevice)
{
//    qDebug() << "HookedEndScene";
    HRESULT hr = D3D_OK;
//    hr = pDevice->TestCooperativeLevel();
//    if (FAILED(hr))
//        qDebug() << "HookedEndScene TestCooperativeLevel" << DXGetErrorString9A(hr);

//    if(oldDevice!=pDevice){
//        qDebug() << "Device changed from" << oldDevice << "to" << pDevice;
//        oldDevice = pDevice;
//        if(pFont){
//            pFont->Release();
//            pFont = nullptr;
//        }
//    }
//    if(!pFont){
//        hr = D3DXCreateFont(pDevice, 30, 0, FW_NORMAL, 1, false,
//        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
//        DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &pFont);
//        if (FAILED(hr))
//            qDebug() << "Could not create font." << DXGetErrorString9A(hr);
//    }
//    if(SUCCEEDED(hr)){
//        RECT rect = {0,0,0,0};
//        SetRect(&rect, 0, 0, 300, 100);
//        int height = pFont->DrawText(nullptr, L"Hello, World!", -1, &rect,
//                DT_LEFT | DT_NOCLIP, -1);
//        if(!height)
//            qDebug() << "Could not draw text.";
//    }
    paint(pDevice);
    memcpy((void *)pEndScene, CodeFragmentES, 5);
    hr = pDevice->EndScene();
    memcpy((void *)pEndScene, jmpbES, 5);

    if(FAILED(hr))
        qDebug() << "HookedEndScene return is" << DXGetErrorString9A(hr);

    return hr;
}

std::uintptr_t present_addr;
std::uintptr_t reset_addr;
PVOID orig_ptr=nullptr;
bool hooked=false;
void HookDevice9Methods(){
    DWORD *VTable;
    DWORD hD3D9 = 0;
    while (!hD3D9) hD3D9 = (DWORD)GetModuleHandle(L"d3d9.dll");
    DWORD PPPDevice = FindPattern(hD3D9, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    qDebug() << "PPPDevice" << (PVOID)PPPDevice;
    memcpy( &VTable, (void *)(PPPDevice + 2), 4);
    qDebug() << "pEndScene" << (PDWORD)(VTable[42]);
    qDebug() << "pPresent " << (PDWORD)(VTable[17]);
    qDebug() << "pReset   " << (PDWORD)(VTable[16]);
    qDebug() << "pRelease " << (PDWORD)(VTable[2]);
    pReset = (oReset)(VTable[16]);
    pEndScene = (oEndScene)(VTable[42]);
//    for(int i=0; i<100; ++i)
//        qDebug() << i << (PDWORD)(VTable[i]);

    hooked = true;

//    pRelease = (oRelease)(VTable[2]);
//    DWORD addrREL = (DWORD)HookedRelease - (DWORD)pRelease - 5;
//    memcpy(jmpbREL + 1, &addrREL, sizeof(DWORD));
//    memcpy(CodeFragmentREL, (PBYTE)pRelease, 5);
//    VirtualProtect((PBYTE)pRelease, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectREL);
//    qDebug() << "pRelease Hook" << (PBYTE)HookedRelease;
//    memcpy((PBYTE)pRelease, jmpbREL, 5);

    // Perform signature scans inside the 'gameoverlayrenderer.dll' library.
//    present_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 DB 74 1F") + 2;
    reset_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 FF 78 18") + 2;
//    std::uintptr_t release_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F0 85 F6 75 15") + 2;
//                                                                       "FF 15 ? ? ? ? 56 B9 24 0C 0F 10"
//    qDebug() << (void*) present_addr << (void*)reset_addr;

    qDebug() << "functions addr.:"
//             << (void*)((DWORD*)present_addr)[0]
             << (void*)((DWORD*)reset_addr)[0];
//             << (void*)((DWORD*)release_addr)[0];
    // Store the original contents of the pointers for later usage.
//    hooks::original_present = **reinterpret_cast<decltype(&hooks::original_present)*>(present_addr);
    hooks::original_reset = **reinterpret_cast<decltype(&hooks::original_reset)*>(reset_addr);
//    hooks::original_release = **reinterpret_cast<decltype(&hooks::original_release)*>(release_addr);
//    hooks::original_present = (oPresent)(**reinterpret_cast<void***>(present_addr));
//    hooks::original_reset =   (oReset)(**reinterpret_cast<void***>(reset_addr));
//    hooks::original_release = reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "user.func:      "
//             << reinterpret_cast<void*>(&hooks::user_present)
             << reinterpret_cast<void*>(&hooks::user_reset);
//             << reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "user.orig.func.:"
//             << reinterpret_cast<void*>(&hooks::original_present)
             << reinterpret_cast<void*>(&hooks::original_reset);
//             << reinterpret_cast<void*>(&hooks::original_release);
    orig_ptr = (void*)(**reinterpret_cast<void***>(reset_addr));
    qDebug() << "orig.func.:     "
//             << (void*)(**reinterpret_cast<void***>(present_addr))
             << orig_ptr;
//             << (void*)(**reinterpret_cast<void***>(release_addr));

    DWORD ptrToRDM, offsetToRDMFromReset;
    if(reset_addr){
        PBYTE ptr = (PBYTE)orig_ptr;
        while(ptr[0]!=0xE9)++ptr; // ищем в коде стим возврат к функции Reset
        ++ptr;
//        qDebug() << (PDWORD)*((PDWORD)(orig_ptr+6))
//                 << (PDWORD)(orig_ptr+5+(*((PDWORD)(orig_ptr+6))));
        // разыменовываем указатель на оффсет к оригинальной фукнции Reset
        ptrToRDM = (DWORD)orig_ptr+(*((PDWORD)ptr));
        offsetToRDMFromReset = ptrToRDM-(DWORD)pReset;
        qDebug() << (PVOID)ptrToRDM << (PVOID)offsetToRDMFromReset;
    }

    // Switch the contents to point to our replacement functions.
//    **reinterpret_cast<void***>(present_addr) = reinterpret_cast<void*>(&hooks::user_present);
    **reinterpret_cast<void***>(reset_addr) = reinterpret_cast<void*>(&hooks::user_reset);
//    **reinterpret_cast<void***>(release_addr) = reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "orig.hook.func.:"
//             << (void*)(**reinterpret_cast<void***>(present_addr))
             << (void*)(**reinterpret_cast<void***>(reset_addr));
//             << (void*)(**reinterpret_cast<void***>(release_addr));
/* RTSS хукает Reset после стима, из-за этого стимовский Reset не вызывается и как следствие, не вызывается
 * мой Reset. Решение: заменить хук RTSS на стимовский хук, а вместо вызова оригинального Reset в моем Reset,
 * вызываеть Reset RTSS.
 * В этом решении при закрытии RTSS во время игры, в моем Reset останется вызов Reseta RTSS, однако в памяти его уже не будет,
 * поэтому, нужно сделать проверку, есть ли в памяти RTSS, если нет, то вызывать оригинальный Reset.
*/
    qDebug() << (PVOID)(ptrToRDM+5) << (((PBYTE)pReset)[0]!=0xE9);
    if(ptrToRDM&&(DWORD)pReset!=(ptrToRDM+5)){
            memcpy(jmpbRES + 1, &offsetToRDMFromReset, sizeof(DWORD));
            QByteArray array5((const char*)jmpbRES, 5);
            qDebug() << "jmpbRES" << QString(array5.toHex());
            memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
            QByteArray array51((const char*)CodeFragmentRES, 5);
            qDebug() << "CodeFragmentRES" << QString(array51.toHex()) << (PDWORD)*((PDWORD)(CodeFragmentRES+1));
            if(CodeFragmentRES[0]==0xE9){
                // получить идентификатор библиотеки и которой вызывается хук
                // затем проверить есть ли библиотека в списке подключенных модулей
                    ptrToExternalHookFunction = PBYTE((DWORD)pReset+(DWORD)*((PDWORD)(CodeFragmentRES+1))+5);

//                pTestCode = ptr;
//                memcpy(&testCode, pTestCode, sizeof(DWORD));
//                qDebug() << "pTestCode" << pTestCode.get();
            }
            VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
            qDebug() << "pReset Hook" << (PBYTE)HookedReset;
            memcpy((PBYTE)pReset, jmpbRES, 5);
            needAddHook = true;
//        }else qDebug() << "Reset is already hooked";
    }else qDebug() << "Reset is the same as RDM";

    // если steam оверлей не подключен, то нужно самостоятельно хукнуть функцию Reset
    if(!reset_addr){
        DWORD addrRES = (DWORD)HookedReset - (DWORD)pReset - 5;
        memcpy(jmpbRES + 1, &addrRES, sizeof(DWORD));
        QByteArray array5((const char*)jmpbRES, 5);
        qDebug() << "jmpbRES" << QString(array5.toHex());
        memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
        QByteArray array51((const char*)CodeFragmentRES, 5);
        qDebug() << "CodeFragmentRES" << QString(array51.toHex());
        VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
        qDebug() << "pReset Hook" << (PBYTE)HookedReset;
        memcpy((PBYTE)pReset, jmpbRES, 5);
    }

    DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addrES, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    memcpy((PBYTE)pEndScene, jmpbES, 5);
    qDebug() << "pEndScene Hook" << (PVOID)HookedEndScene << (PVOID)addrES;
}

void APIENTRY InjectJump(DWORD _offset, DWORD target)
{
    unsigned long Protection;
    VirtualProtect((void*)_offset, 5, PAGE_EXECUTE_READWRITE, &Protection);
    target -= (_offset + 5);
    *((char*)_offset) = 0xE9;
    memcpy((LPVOID)(_offset+1), &target, sizeof(DWORD));
    VirtualProtect((void*)_offset, 5, Protection, 0);
}

int curIndex = 0;
void APIENTRY GetSIDSAddr(PVOID addr) //не забываем _stdcall, иначе компилятор может подкинуть нам свинью
{
    if(!lpSharedMemory->sidsAddrLock){
        PVOID addr_0xC=addr+0xC;
//        qDebug() << "finded address:" << addr_0xC;
        bool finded = false;
        for(int i=0; i<10; ++i)
            if(lpSharedMemory->sidsAddr[i]==addr_0xC)finded=true;
        if(!finded){
            if(curIndex>=10) curIndex = 0;
//            qDebug() << curIndex << addr_0xC;
            lpSharedMemory->sidsAddr[curIndex] = addr_0xC;
            ++curIndex;
        }
    }
}
//Нужно указать при создании инжекта, чтобы функция знала, куда ей возвращаться
LPVOID _injectedGetSIDSAddrRetAddr,
_injectedShutdownModAddr,
_injectedInitializeModAddr;
void APIENTRY _injectedGetSIDSAddr()
{
    //Сохраняем регистры
    //Количество пушей - это количество аргументов функции
    //Смещение в стэке расчитывается так: (кол-во аргументов)*4+0x20
    //У нас получается 2*4=8
    //    asm volatile("jmp $0x0, $0x00A35E0A\n\t");
    //    asm volatile("jmp *(_injectedGetSIDSAddrRetAddr)");
    //    asm volatile("jmp %0" : : "r"(_injectedGetSIDSAddrRetAddr));
    asm(
//        "popl %ebp\n\t"
        "pushal\n\t"
        "pushl %eax\n\t");
    asm("call %P0\n\t" : : "i"(GetSIDSAddr));
    asm("popal\n\t"
        "movl 0x1C(%eax),%ebp\n\t"
        "decl %edx\n\t"
        "pushl %esi\n\t"
        "pushl $0x00A35DF7\n\t"
//        "ret\n"
    );
}
void APIENTRY InitializeMod()
{
    qDebug() << "InitializeMod";
    if(!hooked) HookDevice9Methods();
}

void APIENTRY _injectedInitialize()
{
    asm("pushal\n\t");
    asm("call %P0\n\t" : : "i"(InitializeMod));
    asm("popal\n\t"
        "pushl $0x00AFFB74\n\t"
        "pushl $0x0096F004\n\t");

//    asm("pushal\n\t");
//    asm("call %P0\n\t" : : "i"(InitializeMod));
//    asm("popal\n\t"
//        "popl %ebp\n\t"
//        "popl %ebx\n\t"
//        "movb $0x01,%al\n\t"
//        "popl %esi\n\t"
//        "pushl $0x0096F062\n\t");
}

void APIENTRY ShutdownMod()
{
    if(hooked){
        qDebug() << "ShutdownMod: unhooking";
        // Убираем хук EndScene
        memcpy((void *)pEndScene, CodeFragmentES, 5);
        // Убираем хук Release
//        memcpy((void *)pRelease, CodeFragmentREL, 5);
        // Убираем хук Reset
//        memcpy((void *)pReset, CodeFragmentRES, 5);
        // Убираем хук steam Reset
        **reinterpret_cast<void***>(reset_addr) = orig_ptr;
        // Убираем хук который перенаправлял вызов оригинальной Reset
        // на steam Reset
        if(needAddHook)memcpy((void *)pReset, CodeFragmentRES, 5);
        // Освобождаем память
        Render.ReleaseFonts();
        Draw.ReleaseFonts();
        if(pFont!=nullptr){
            qDebug() << "Release Font";
            pFont->Release();
            pFont = nullptr;
        }
        hooked = false;
        qDebug() << "ShutdownMod: success";
    } else
        qDebug() << "ShutdownMod: not hooked";

}

void APIENTRY _injectedShutdown()
{
//    asm("pushal\n\t");
//    asm("call %P0\n\t" : : "i"(ShutdownMod));
//    asm("popal\n\t"
//        "pushl %esi\n\t"
//        "movl %ecx,%esi\n\t"
//        "movl (%esi),%ecx\n\t"
//        "pushl $0x0096F075\n\t");
    asm("pushal\n\t");
    asm("call %P0\n\t" : : "i"(ShutdownMod));
    asm("popal\n\t"
        "movl %ecx,%esi\n\t"
        "movl 0x04(%esi),%ecx\n\t"
        "pushl $0x0066D0B6\n\t");
}
DWORD WINAPI KeyHook(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    idThread = GetCurrentProcessId();
    qDebug() << "[*] Starting KeyCapture";
    /*
    HHOOK WINAPI SetWindowsHookEx(
      _In_ int       idHook,
      _In_ HOOKPROC  lpfn,
      _In_ HINSTANCE hMod,
      _In_ DWORD     dwThreadId
    );
    */
    // Start the hook of the keyboard
    keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL, // low-level keyboard input events
        KeyboardProc, // pointer to the hook procedure
        GetModuleHandle(NULL), // A handle to the DLL containing the hook procedure
        NULL //desktop apps, if this parameter is zero
        );
    mouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, GetModuleHandle(NULL), NULL);
    if (!keyboardHook){
        // Hook returned NULL and failed
        qDebug() << "[!] Failed to get handle from SetWindowsHookEx()" << GetLastError();
    }
    else {
        qDebug() << "[*] KeyCapture handle ready";
        // http://www.winprog.org/tutorial/message_loop.html
        MSG Msg;
        while (GetMessage(&Msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
}

DWORD WINAPI Hook(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    bool enableDXHook = false, runWithGame = true, showHP = false;
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TGameInfo), L"DXHook-Shared-Memory");
    if(hSharedMemory != nullptr)
    {
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        PTotalActions = &lpSharedMemory->total_actions;
        if(lpSharedMemory!=nullptr){
            lpSharedMemory->playersNumber = 0;
            Render.setGameInfo(lpSharedMemory);
            QSettings settings("stats.ini", QSettings::IniFormat);
            runWithGame = settings.value("settings/runWithGame", true).toBool();
            enableDXHook = settings.value("settings/enableDXHook", true).toBool();
            showHP = settings.value("settings/showHP", false).toBool();
            qDebug() << "enableDXHook:" << enableDXHook;
            version_str = settings.value("info/version", "0.0.0").toString();
            closeWithGame = settings.value("settings/closeWithGame", false).toBool();
            qDebug() << "Stats version:" << version_str;
        }else
            qDebug() << "MapViewOfFile: Error " << GetLastError();
    }else
        qDebug() << "CreateFileMapping: Error " << GetLastError();

    DWORD dwOldProtect = 0;
    memcpy(temp4, HPAddr, 4);
    VirtualProtect(HPAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    if(showHP&&memcmp(temp4, CodeHP, 4)==0)
        memcpy(HPAddr, nop_array4, 4);
    else if(!showHP)
        memcpy(HPAddr, CodeHP, 4);
    VirtualProtect(HPAddr, 4, dwOldProtect, 0);

//    Sleep(10000);
//    if(enableDXHook)
//        HookDevice9Methods();

    _injectedGetSIDSAddrRetAddr = (LPVOID)(0x00A35DF7); //0x00A35E05 + 5 конец опкода, которые мы перезаписываем
    InjectJump(0x00A35DF2, (DWORD)&_injectedGetSIDSAddr);

    if(enableDXHook){
        _injectedInitializeModAddr = (LPVOID)(0x0096F004);
        InjectJump(0x0096EFFF, (DWORD)&_injectedInitialize);
//        InjectJump(0x0096F05D, (DWORD)&_injectedInitialize);
//        _injectedShutdownModAddr = (LPVOID)(0x0096F075);
//        InjectJump(0x0096F070, (DWORD)&_injectedShutdown);
        _injectedShutdownModAddr = (LPVOID)(0x0066D0B6);
        InjectJump(0x0066D0B1, (DWORD)&_injectedShutdown);
    }

    if(!runWithGame) return 0;
    QProcess schtasks;
    QTextCodec *codec = QTextCodec::codecForName("CP866");
    schtasks.start("schtasks", {"/Run", "/I", "/tn", "Soulstorm Ladder v2.0"});
    schtasks.waitForFinished();
    qDebug() << codec->toUnicode(schtasks.readAllStandardOutput()).remove("\r\n")
             << QString("%1").arg((ulong)schtasks.exitCode(),8,16,QLatin1Char('0')).toUpper();
    if (schtasks.exitCode()!=0){
        TCHAR NPath[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, NPath);
        SHELLEXECUTEINFO ShExecInfo;
        ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        ShExecInfo.fMask = /*SEE_MASK_NOCLOSEPROCESS|*/SEE_MASK_FLAG_NO_UI/*|SEE_MASK_CLASSNAME*/;
//        ShExecInfo.lpClass = L"exefile";
        ShExecInfo.hwnd = NULL;
        ShExecInfo.lpVerb = L"runas";
        ShExecInfo.lpFile = L"SSStatsUpdater.exe";
        ShExecInfo.lpParameters = NULL;
        ShExecInfo.lpDirectory = NPath;
        ShExecInfo.nShow = SW_HIDE;
        ShExecInfo.hInstApp = NULL;
        qDebug() << QString::fromWCharArray(NPath);

        if(ShellExecuteEx(&ShExecInfo))
            qDebug() << "startDetached - Success!";
        else
            qDebug() << "startDetached - Failed!" << GetLastError();

        CloseHandle(ShExecInfo.hProcess);

    } else
        qDebug() << "Soulstorm Ladder task successfully runned";

    return 0;
}

BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    if (dwReason==DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);
        logger.installLog();
        qDebug() << "----------Attached----------";
        CreateThread(NULL,0,Hook,NULL,0,NULL);
        CreateThread(NULL,0,KeyHook,NULL,0,NULL);
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
        if(closeWithGame)
            PostThreadMessage(lpSharedMemory->statsThrId, WM_QUIT, 0, 0);

        qDebug() << "----------Detached----------";
        logger.finishLog();
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hSharedMemory);
        unhookKeyboard();
        // Exit if failure
    }

    return TRUE;
}

void paint(LPDIRECT3DDEVICE9 pDevice)
{
    if(oldDevice!=pDevice) initFonts(pDevice);
    if(lpSharedMemory->showMenu)
        Render.Init_PosMenu(QString("Soulstorm Ladder "+version_str).toStdString().data());

    int center_text_pos = width*0.5;
    if(lpSharedMemory->showRaces){
        int text_w = 0;
        for(int i=0; i<8; i++){
            string player_info(&lpSharedMemory->players[i][0]);
            if(!player_info.empty()){
                int w = Draw.GetTextLen(player_info.data(), 0);
                if(w>text_w)
                    text_w = w;
            } else break;
        }
        int box_width = text_w+8;
        int box_height = fontSize+8;
        int offset_x = center_text_pos-box_width/2;
        int offset_y = fontSize+14;
        for(int i=0; i<8; i++){
            string player_info(&lpSharedMemory->players[i][0]);
            if(!player_info.empty()){
                Draw.Draw_Box(offset_x, i*offset_y+offset_y, box_width, box_height, DARKGRAY(150));
                Draw.Draw_Border(offset_x, i*offset_y+offset_y, box_width, box_height, 1, SKYBLUE(255));
                Draw.String(offset_x+4, i*offset_y+4+offset_y, ORANGE(255), player_info.data(), 0, DT_LEFT|DT_NOCLIP);
            } else break;
        }
    }

    if(lpSharedMemory->showAPM){
        string str="";
        if(lpSharedMemory->CurrentAPM>0)
            str += "Current: "+to_string(lpSharedMemory->CurrentAPM);
        if(lpSharedMemory->AverageAPM>0)
            str += " Average: "+to_string(lpSharedMemory->AverageAPM);
        if(lpSharedMemory->MaxAPM>0)
            str += " Max: "+to_string(lpSharedMemory->MaxAPM);
        if(!str.empty()){
            int text_w = Draw.GetTextLen(str.data(), 0);
            int box_width = text_w+12;
            int box_height = fontSize+8;
            int offset_x = center_text_pos-(box_width)/2;
            Draw.Draw_Box(offset_x, 0, box_width, box_height, DARKGRAY(150));
            Draw.Draw_Border(offset_x, 0, box_width, box_height, 1, SKYBLUE(255));
            Draw.Text(str.data(), offset_x+4, 4, 0, false, ORANGE(255), ORANGE(255));
        }
    }
}

Q_DECL_EXPORT void Inject(void)
{
}
