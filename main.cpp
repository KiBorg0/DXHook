#define D3D_DEBUG_INFO

#include <qglobal.h>
#include <windows.h>
//#include <winbase.h>
#include <Processthreadsapi.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr9.h>
#include <QString>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <QSettings>
#include <QProcess>
#include <QDebug>
#include <fstream>
#include "cRender.h"
#include "logger.h"
#include "cMemory.h"
#include "hooks.h"
//#include "..\SSStats\systemwin32.h"

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
LPD3DXSPRITE pSprite = nullptr;
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
BYTE CodeFragmentPR[5] = {0};
BYTE jmpbPR[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectPR = 0;
BYTE CodeFragmentRES[5] = {0};
//E9 1B 07 59 FA
BYTE jmpbRES[5] = {0xE9, 0x1B, 0x07, 0x59, 0xFA};
DWORD dwOldProtectRES = 0;
BYTE CodeFragmentES[5] = {0};
BYTE jmpbES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectES = 0;
BYTE CodeFragmentREL[5] = {0};
BYTE jmpbREL[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectREL = 0;

LPDIRECT3DDEVICE9 oldDevice = nullptr;
HANDLE hSharedMemory = nullptr;
PGameInfo lpSharedMemory = nullptr;
int fontSize = 14;
DWORD height, width;
bool closeWithGame = false;
QString version_str = "0.0.0";

bool deviceReseted = false;
bool needAddHook = true;

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

    if(hr==D3DERR_DEVICENOTRESET&&pFont){
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

HRESULT STDMETHODCALLTYPE hooks::user_reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    qDebug() << "RESET";
    if(needAddHook)memcpy((void *)pReset, CodeFragmentRES, 5);
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
    memcpy((void *)pReset, CodeFragmentRES, 5);
    qDebug() << "HookedReset";
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Reset TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
    Draw.OnLostDevice();
    if(pFont)pFont->OnLostDevice();

    hr = pDevice->Reset(pPresentationParameters);

    if(SUCCEEDED(hr)) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
        Draw.OnResetDevice();
        if(pFont)pFont->OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    memcpy((void *)pReset, jmpbRES, 5);

    return hr;
}
HRESULT APIENTRY HookedRelease(LPDIRECT3DDEVICE9 pDevice){
    qDebug() << "HookedRelease";
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Release TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

//    if(hr==D3DERR_DEVICENOTRESET&&pFont){
//        pFont->Release();
//        pFont = nullptr;
//    }
    memcpy((void *)pRelease, CodeFragmentREL, 5);
    hr = pDevice->Release();
    memcpy((void *)pRelease, jmpbREL, 5);
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
//    HRESULT hr = pDevice->TestCooperativeLevel();
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
//    if(!pSprite){
//        hr = D3DXCreateSprite(pDevice, &pSprite);
//        if (FAILED(hr))
//            qDebug() << "Could not create sprite." << DXGetErrorString9A(hr);
//    }
//    if(SUCCEEDED(hr)){
//        RECT rect = {0,0,0,0};
//        SetRect(&rect, 0, 0, 300, 100);
//        int height;
//        if(pSprite){
//            pSprite->Begin(D3DXSPRITE_DONOTMODIFY_RENDERSTATE);
//            height = pFont->DrawText(pSprite, L"Hello, World!", -1, &rect,
//                DT_LEFT | DT_NOCLIP, -1);
//            pSprite->End();
//        }else
//            height = pFont->DrawText(nullptr, L"Hello, World!", -1, &rect,
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
    hooked = true;

//    pRelease = (oRelease)(VTable[2]);
//    DWORD addrREL = (DWORD)HookedRelease - (DWORD)pRelease - 5;
//    memcpy(jmpbREL + 1, &addrREL, sizeof(DWORD));
//    memcpy(CodeFragmentREL, (PBYTE)pRelease, 5);
//    VirtualProtect((PBYTE)pRelease, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectREL);
//    qDebug() << "pRelease Hook" << (PBYTE)HookedRelease;
//    memcpy((PBYTE)pRelease, jmpbREL, 5);

    // Perform signature scans inside the 'gameoverlayrenderer.dll' library.
    present_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 DB 74 1F") + 2;
    reset_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 FF 78 18") + 2;
//    std::uintptr_t release_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F0 85 F6 75 15") + 2;
//                                                                       "FF 15 ? ? ? ? 56 B9 24 0C 0F 10"
//    qDebug() << (void*) present_addr << (void*)reset_addr;

    qDebug() << "functions addr.:"
             << (void*)((DWORD*)present_addr)[0]
             << (void*)((DWORD*)reset_addr)[0];
//             << (void*)((DWORD*)release_addr)[0];
    // Store the original contents of the pointers for later usage.
    hooks::original_present = **reinterpret_cast<decltype(&hooks::original_present)*>(present_addr);
    hooks::original_reset = **reinterpret_cast<decltype(&hooks::original_reset)*>(reset_addr);
//    hooks::original_release = **reinterpret_cast<decltype(&hooks::original_release)*>(release_addr);
//    hooks::original_present = (oPresent)(**reinterpret_cast<void***>(present_addr));
//    hooks::original_reset =   (oReset)(**reinterpret_cast<void***>(reset_addr));
//    hooks::original_release = reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "user.func:      "
             << reinterpret_cast<void*>(&hooks::user_present)
             << reinterpret_cast<void*>(&hooks::user_reset);
//             << reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "user.orig.func.:"
             << reinterpret_cast<void*>(&hooks::original_present)
             << reinterpret_cast<void*>(&hooks::original_reset);
//             << reinterpret_cast<void*>(&hooks::original_release);
    qDebug() << "orig.func.:     "
             << (void*)(**reinterpret_cast<void***>(present_addr))
             << (void*)(**reinterpret_cast<void***>(reset_addr));
//             << (void*)(**reinterpret_cast<void***>(release_addr));
    orig_ptr = (void*)(**reinterpret_cast<void***>(reset_addr));
    qDebug() << (PDWORD)*((PDWORD)(orig_ptr+6))
             << (PDWORD)(orig_ptr+5+(*((PDWORD)(orig_ptr+6))))
             << orig_ptr;
    DWORD ptrToRDM = (DWORD)orig_ptr+(*((PDWORD)(orig_ptr+6)));
    DWORD offsetToRDMFromReset = ptrToRDM-VTable[16];
    qDebug() << (PVOID)ptrToRDM << (PVOID)offsetToRDMFromReset;
    // Switch the contents to point to our replacement functions.
//    **reinterpret_cast<void***>(present_addr) = reinterpret_cast<void*>(&hooks::user_present);
    **reinterpret_cast<void***>(reset_addr) = reinterpret_cast<void*>(&hooks::user_reset);
//    **reinterpret_cast<void***>(release_addr) = reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << "orig.hook.func.:"
             << (void*)(**reinterpret_cast<void***>(present_addr))
             << (void*)(**reinterpret_cast<void***>(reset_addr));
//             << (void*)(**reinterpret_cast<void***>(release_addr));
    pReset = (oReset)(VTable[16]);
    qDebug() << VTable[16] << (ptrToRDM+5);
    if(VTable[16]!=(ptrToRDM+5)){
        memcpy(jmpbRES + 1, &offsetToRDMFromReset, sizeof(DWORD));
        memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
        VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
        qDebug() << "pReset Hook" << (PBYTE)HookedReset;
        memcpy((PBYTE)pReset, jmpbRES, 5);
    }else{
        qDebug() << "Reset is the same as RDM";
        needAddHook = false;
    }

//    DWORD addrRES = (DWORD)HookedReset - (DWORD)pReset - 5;
//    memcpy(jmpbRES + 1, &addrRES, sizeof(DWORD));
//    memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
//    VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
//    qDebug() << "pReset Hook" << (PBYTE)HookedReset;
//    memcpy((PBYTE)pReset, jmpbRES, 5);

    pEndScene = (oEndScene)(VTable[42]);
    DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addrES, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    memcpy((PBYTE)pEndScene, jmpbES, 5);
    qDebug() << "pEndScene Hook" << (PVOID)HookedEndScene << (PVOID)addrES;
//    CreateThread(NULL, 0, VirtualMethodTableRepatchingLoopToCounterExtensionRepatching, NULL, 0, NULL);
}

DWORD WINAPI VirtualMethodTableRepatchingLoopToCounterExtensionRepatching(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    while(1){
        if(deviceReseted) Sleep(5000);
        else Sleep(100);
        memcpy((PBYTE)pEndScene, jmpbES, 5);
    }

    return 1;
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

int currentAddr = 0;
void APIENTRY GetSIDSAddr(PVOID addr) //не забываем _stdcall, иначе компилятор может подкинуть нам свинью
{
    if(!lpSharedMemory->sidsAddrLock){
        PVOID addr_0xC=addr+0xC;
        if(currentAddr>=10) currentAddr = 0;
        bool finded = false;
        for(int i=0; i<10; ++i)
            if(lpSharedMemory->sidsAddr[i]==addr_0xC)finded=true;
        if(!finded){
            lpSharedMemory->sidsAddr[currentAddr] = addr_0xC;
            ++currentAddr;
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
        "popl %ebp\n\t"
        "pushal\n\t"
        "pushl %eax\n\t");
    asm("call %P0\n\t" : : "i"(GetSIDSAddr));
    asm("popal\n\t"
        "movl 0x1C(%eax),%ebp\n\t"
        "decl %edx\n\t"
        "pushl %esi\n\t"
        "pushl $0x00A35DF7\n\t"
        "ret\n"
    );
}
void APIENTRY InitializeMod()
{
    qDebug() << "InitializeMod";
    if(!hooked) HookDevice9Methods();
}

void APIENTRY _injectedInitialize()
{
    asm(
        "popl %ebp\n\t"
        "pushal\n\t");
    asm("call %P0\n\t" : : "i"(InitializeMod));
    asm("popal\n\t"
        "pushl $0x00AFFB74\n\t"
        "pushl $0x0096F004\n\t"
        "ret\n"
    );
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
//        Render.ReleaseFonts();
//        Draw.ReleaseFonts();
        if(pFont){
//            LPDIRECT3DDEVICE9 temp_pDevice;
//            pFont->GetDevice(&temp_pDevice);
//            HRESULT hr =  temp_pDevice->TestCooperativeLevel();
//            if (FAILED(hr))
//                qDebug() << "HookedEndScene TestCooperativeLevel" << DXGetErrorString9A(hr);
//            pFont->OnLostDevice();
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
    asm(
        "popl %ebp\n\t"
        "pushal\n\t");
    asm("call %P0\n\t" : : "i"(ShutdownMod));
    asm("popal\n\t"
        "pushl %esi\n\t"
        "movl %ecx,%esi\n\t"
        "movl (%esi),%ecx\n\t"
        "pushl $0x0096F075\n\t"
        "ret\n"
    );
}
//void APIENTRY _injectedGetMatchResult()
//{
//    asm(
//        "popl %ebp\n\t"
//        "pushal\n\t"
//        "pushl %eax\n\t");
//    asm("call %P0\n\t" : : "i"(GetMatchResult));
//    asm("popal\n\t"
//        "movl 0x68(%ebx),%edx\n\t"
//        "movl (%eax),%eax\n\t"
//        "pushl $0x004683DA\n\t"
//        "ret\n"
//    );
//}

DWORD WINAPI Hook(LPVOID Param)
{
//    Sleep(1000);
    UNREFERENCED_PARAMETER(Param);
    bool enableDXHook = false, runWithGame = true;
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TGameInfo), L"DXHook-Shared-Memory");
    if(hSharedMemory != nullptr)
    {
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if(lpSharedMemory!=nullptr){
            Render.setGameInfo(lpSharedMemory);
            QSettings settings("stats.ini", QSettings::IniFormat);
            runWithGame = settings.value("settings/runWithGame", true).toBool();
            enableDXHook = settings.value("settings/enableDXHook", true).toBool();
            qDebug() << "enableDXHook:" << enableDXHook;
            version_str = settings.value("info/version", "0.0.0").toString();
            closeWithGame = settings.value("settings/closeWithGame", false).toBool();
            qDebug() << "Stats version:" << version_str;
        }else
            qDebug() << "MapViewOfFile: Error " << GetLastError();
    }else
        qDebug() << "CreateFileMapping: Error " << GetLastError();

//    if(enableDXHook)
//        HookDevice9Methods();

    _injectedGetSIDSAddrRetAddr = (LPVOID)(0x00A35DF7); //0x00A35E05 + 5 конец опкода, которые мы перезаписываем
    InjectJump(0x00A35DF2, (DWORD)&_injectedGetSIDSAddr);

    _injectedShutdownModAddr = (LPVOID)(0x0096F075);
    InjectJump(0x0096F070, (DWORD)&_injectedShutdown);
    _injectedInitializeModAddr = (LPVOID)(0x0096F004);
    InjectJump(0x0096EFFF, (DWORD)&_injectedInitialize);

    if(!runWithGame) return 0;
//    qDebug() << QProcess::startDetached("SSStatsUpdater.exe");
//    QProcess schtasks;
//    QTextCodec *codec = QTextCodec::codecForName("CP866");
//    schtasks.start("schtasks", {"/Run", "/I", "/tn", "Soulstorm Ladder v2.0", "/HRESULT"});
//    schtasks.waitForFinished();
//    qDebug() << codec->toUnicode(schtasks.readAllStandardOutput()).remove("\r\n")
//             << QString("%1").arg((ulong)schtasks.exitCode(),8,16,QLatin1Char('0')).toUpper();
//    if (schtasks.exitCode()!=0){
//        TCHAR NPath[MAX_PATH];
//        GetCurrentDirectory(MAX_PATH, NPath);
//        SHELLEXECUTEINFO ShExecInfo;
//        ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
//        ShExecInfo.fMask = /*SEE_MASK_NOCLOSEPROCESS|*/SEE_MASK_FLAG_NO_UI/*|SEE_MASK_CLASSNAME*/;
////        ShExecInfo.lpClass = L"exefile";
//        ShExecInfo.hwnd = NULL;
//        ShExecInfo.lpVerb = L"runas";
//        ShExecInfo.lpFile = L"SSStatsUpdater.exe";
//        ShExecInfo.lpParameters = NULL;
//        ShExecInfo.lpDirectory = NPath;
//        ShExecInfo.nShow = SW_HIDE;
//        ShExecInfo.hInstApp = NULL;
//        qDebug() << QString::fromWCharArray(NPath);

//        if(ShellExecuteEx(&ShExecInfo))
//            qDebug() << "startDetached - Success!";
//        else
//            qDebug() << "startDetached - Failed!" << GetLastError();

//        CloseHandle(ShExecInfo.hProcess);

//    } else
//        qDebug() << "Soulstorm Ladder task successfully runned";

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
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
        if(closeWithGame)
            PostThreadMessage(lpSharedMemory->statsThrId, WM_QUIT, 0, 0);

        qDebug() << "----------Detached----------";
        logger.finishLog();
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hSharedMemory);
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
        for(int i=0; i<8; i++){
            string player_info(&lpSharedMemory->players[i][0]);
            if(!player_info.empty()){
                Draw.Draw_Box(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(fontSize+14),text_w+8,fontSize+8,DARKGRAY(150));
                Draw.Draw_Border(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(fontSize+14),text_w+8,fontSize+8,1,SKYBLUE(255));
                Draw.String(center_text_pos-(text_w+8)/2+4,/*height*0.02+*/i*(fontSize+14)+4, ORANGE(255), player_info.data(), 0, DT_LEFT|DT_NOCLIP);
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
            Draw.Draw_Box(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,fontSize+12,DARKGRAY(150));
            Draw.Draw_Border(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,fontSize+12,1,SKYBLUE(255));
            Draw.Text(str.data(), center_text_pos-(text_w+12)/2+6, /*height*0.02+*/6, 0, false, ORANGE(255), ORANGE(255));
        }
    }
}

Q_DECL_EXPORT void Inject(void)
{
//    LPDIRECT3D9 pD3D;
//    if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) != NULL)
//        pD3D->Release();
}
