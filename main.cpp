#include <qglobal.h>
#include <windows.h>
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
//#include <QMap>
#include <fstream>
#include "cRender.h"
#include "logger.h"
#include "cMemory.h"
Logger logger;
cRender Render;
CDraw Draw;

using namespace std;
#define D3DparamX		, UINT paramx
#define D3DparamvalX	, paramx
#define BUFSIZE 4096
void paint(LPDIRECT3DDEVICE9 pDevice);

//=====================================================================================

typedef HRESULT (WINAPI* CreateDevice_Prototype)        (LPDIRECT3D9, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, LPDIRECT3DDEVICE9*);
typedef HRESULT (WINAPI* Reset_Prototype)               (LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT (WINAPI* EndScene_Prototype)            (LPDIRECT3DDEVICE9);
typedef HRESULT (WINAPI* DrawIndexedPrimitive_Prototype)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);

CreateDevice_Prototype         CreateDevice_Pointer         = NULL;
Reset_Prototype                Reset_Pointer                = NULL;
EndScene_Prototype             EndScene_Pointer             = NULL;
DrawIndexedPrimitive_Prototype DrawIndexedPrimitive_Pointer = NULL;

HRESULT WINAPI Direct3DCreate9_VMTable    (VOID);
HRESULT WINAPI CreateDevice_Detour        (LPDIRECT3D9, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, LPDIRECT3DDEVICE9*);
HRESULT WINAPI Reset_Detour               (LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
HRESULT WINAPI EndScene_Detour            (LPDIRECT3DDEVICE9);
HRESULT WINAPI DrawIndexedPrimitive_Detour(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);

PBYTE HookVTableFunction( PDWORD* dwVTable, PBYTE dwHook, INT Index );
DWORD WINAPI VirtualMethodTableRepatchingLoopToCounterExtensionRepatching(LPVOID);
PDWORD Direct3D_VMTable = NULL;

//=====================================================================================

typedef LPDIRECT3D9 (WINAPI* DIRECT3DCREATE9)(unsigned int);
//edit if you want to work for the game
typedef HRESULT (WINAPI* oReset)( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );
typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
oEndScene pEndScene = NULL;
oReset pReset = NULL;
DWORD height, width;
DWORD dwEndScene;
DWORD dwReset;
BYTE CodeFragmentES[5] = {0};
BYTE jmpbES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectES = 0;
BYTE CodeFragmentRES[5] = {0};
BYTE jmpbRES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectRES = 0;

HANDLE hookThrHandle = nullptr;
PBYTE curEndScene;
LPDIRECT3DDEVICE9 oldDevice = nullptr;
LPDIRECT3DDEVICE9 curDevice = nullptr;
HANDLE hSharedMemory = nullptr;
PGameInfo lpSharedMemory = nullptr;
int fontSize = 14;
int mainFontSize = 14;
int titleFontSize = 14;
//QString hotKey = "F9";
//int version = 0;
QString version_str = "0.0.0";
D3DCOLOR fontColor = ORANGE(255);
string font = "Gulim";
//QVector<short> key_comb;

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
    qDebug() << "screen width:" << width << "screen height:" << height;
    switch (height) {
    case 600:
        mainFontSize = 12;
        titleFontSize = 12;
    case 640:
        mainFontSize = 14;
        titleFontSize = 14;
    case 720:
        mainFontSize = 14;
        titleFontSize = 14;
        break;
    case 768:
        mainFontSize = 16;
        titleFontSize = 16;
        break;
    case 780:
        mainFontSize = 16;
        titleFontSize = 16;
        break;
    case 800:
        mainFontSize = 16;
        titleFontSize = 16;
        break;
    case 1024:
        mainFontSize = 18;
        titleFontSize = 18;
        break;
    case 1280:
        mainFontSize = 20;
        titleFontSize = 20;
        break;
    case 1600:
        mainFontSize = 22;
        titleFontSize = 22;
        break;
    default:
        mainFontSize = 14;
        titleFontSize = 14;
        break;
    }

    qDebug() << "Release fonts";
    Draw.ReleaseFonts();
    qDebug() << "Init fonts";
    Draw.AddFont(font.data(), mainFontSize, false, false);
    Draw.AddFont("Tahoma", 15, false, false);
    Draw.AddFont("Verdana", 15, true, false);
    Draw.AddFont("Verdana", 13, true, false);
    Draw.AddFont("Comic Sans MS", 30, true, false);
    if(AddFontResourceA("Engine/Locale/English/data/font/engo.ttf"))
    {
        Render.AddFont("GothicRus", titleFontSize, false, false);
        Draw.AddFont("GothicRus", 15, false, false);
    }
    else{
        Render.AddFont("Arial", titleFontSize, false, false);
        Draw.AddFont("Arial", 10, false, false);
    }
    if(AddFontResourceA("Engine/Locale/English/data/font/ansnb___.ttf"))
    {
        Render.AddFont("Arial Unicode MS", titleFontSize, false, false);
    }
    else{
        Render.AddFont(font.data(), mainFontSize, false, false);
    }
}

// проблема заключается в стимовском оверлее,
// дело в том что выполняя сброс устройства здесь,
// мы не ждем пока освободятся ресурсы стимовского хука
// из за этого резет не выполняется
HRESULT APIENTRY HookedReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters )
{
    if(oldDevice!=pDevice){
        qDebug() << "Reset";
        Draw.SetDevice(pDevice);
        Render.setDevice(pDevice);
        Render.ReleaseFonts();
        initFonts(pDevice);
    }
//    qDebug() << "Reset is hooked" << (PDWORD)*(((PDWORD)pReset)+1);
    BYTE* CDRES = (BYTE*)pReset;
    CDRES[0] = CodeFragmentRES[0];
    *((DWORD*)(CDRES + 1)) = *((DWORD*)(CodeFragmentRES + 1));

    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
    {
        qDebug() << DXGetErrorString9A(hr);
        // Device lost, stay idle till we can reset it
        if (hr == D3DERR_DEVICELOST)
        {
            Sleep(50);
        }
        //        else if (hr == D3DERR_DEVICENOTRESET)
        //        {
        //            // Call lost device to release affected interface
        //            OnLostDevice();
        //            // Reset device
        //            OnResetDevice();
        //        }
    }


    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
//    Draw.OnLostDevice();
//    memcpy((void *)pReset, CodeFragmentRES, 5);
//    qDebug() << (PDWORD)*(((PDWORD)pReset)+1);
//    HRESULT hRet = pReset(pDevice, pPresentationParameters);
    HRESULT hRet = pDevice->Reset(pPresentationParameters);
//    memcpy((void *)pReset, jmpbRES, 5);
    if(hRet == D3D_OK) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
//        Draw.OnResetDevice();
    }
    else{ qDebug() << "Reset return is" << DXGetErrorString9A(hRet);
//        Render.ReleaseFonts();
//        Render.AddFont("Arial", 14, false, false);
//        Render.AddFont(font.data(), 14, false, false);
//        return hRet;
    }



//    qDebug() << (PDWORD)*(((PDWORD)pReset)+1);

    CDRES[0] = jmpbRES[0];
    *((DWORD*)(CDRES + 1)) = *((DWORD*)(jmpbRES + 1));

    return hRet;
}
HRESULT APIENTRY HookedEndScene( LPDIRECT3DDEVICE9 pDevice )
{
//    qDebug() << "EndScene is hooked" << (PDWORD)*(((PDWORD)pEndScene)+1);
    paint(pDevice);

    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
    {
        qDebug() << DXGetErrorString9A(hr);
    }
    memcpy((void *)pEndScene, CodeFragmentES, 5);
//    qDebug() << (PDWORD)*(((PDWORD)pEndScene)+1);
//    BYTE* CDES = (BYTE*)pEndScene;
//    CDES[0] = CodeFragmentES[0];
//    *((DWORD*)(CDES + 1)) = *((DWORD*)(CodeFragmentES + 1));
    memcpy((void *)pEndScene, CodeFragmentES, 5);
    HRESULT hRet = pDevice->EndScene();
    memcpy((void *)pEndScene, jmpbES, 5);
//    CDES[0] = jmpbES[0];
//    *((DWORD*)(CDES + 1)) = *((DWORD*)(jmpbES + 1));

    if(hRet != D3D_OK)
        qDebug() << "EndScene return is" << DXGetErrorString9A(hRet);;

//    memcpy((void *)pEndScene, jmpbES, 5);
//    qDebug() << (PDWORD)*(((PDWORD)pEndScene)+1);

    return hRet;
}



void GetDevice9Methods()
{
    qDebug() << "GetDevice9Methods";
    // создаем свое окно
    HWND hWnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // получаем адрес модуля загруженной в память библиотеки
    HMODULE hD3D9 = LoadLibrary(L"d3d9");
    // получаем функцию создания устройства рисования
    DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(hD3D9, "Direct3DCreate9");
    // создаем объект Direct3D
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DDISPLAYMODE d3ddm;
    d3d->GetAdapterDisplayMode(0, &d3ddm);
    // Для успешного создания объекта устройства нужно заполнить
    // структурную переменную типа D3DPRESENT_PARAMETERS:
    D3DPRESENT_PARAMETERS d3dpp;
    // Здесь мы заполняем только часть полей, поэтому,
    // чтобы в остальных полях не оказалось какого-нибудь случайного значения,
    // вся память, занимаемая переменной, обнуляется (функция ZeroMemory).
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    IDirect3DDevice9* d3dDevice = 0;
    // создаем устройство рисования (видеокарта) для того чтобы получить адреса функций
    d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
    // получаем таблицу адресов функций устройства
    DWORD* vtablePtr = (DWORD*)(*((DWORD*)d3dDevice));

    // получаем адрес функции EndScene (первый способ)
    // За каждым вызовом BeginScene рано или поздно должен следовать вызов EndScene,
    // осуществляемый до того, как будет произведено обновление экрана с помощью вызова Present9.
    // Когда вызов EndScene завершается успешно, сцена ставится в очередь для визуализации драйвером.
    // Этот метод не является синхронным, поэтому завершение рендеринга сцены к моменту возврата из метода не гарантируется.
//    dwEndScene = vtablePtr[42] - (DWORD)hD3D9;
    // получаем адрес функции Present9 (второй способ)
    // Предоставляет для отображения содержимое следующего буфера в последовательности задних буферов, принадлежащей устройству.
    dwEndScene = vtablePtr[42] - (DWORD)hD3D9;
    dwReset = vtablePtr[16] - (DWORD)hD3D9;
    // освобождаем устройство, так как больше оно не нужно
    d3dDevice->Release();
    d3d->Release();
    FreeLibrary(hD3D9);
    CloseHandle(hWnd);
    qDebug() << "GetDevice9Methods return";
}

// копирует size байт в переданную таблицу и возвращает указатель
// на устройство рисования
PVOID D3Ddiscover(void *tbl, int size)
{
    HWND                 hWnd;
    void                 *pInterface = 0;
    D3DPRESENT_PARAMETERS d3dpp;

    if ((hWnd = CreateWindowEx(NULL, WC_DIALOG, L"", WS_OVERLAPPED, 0, 0, 50, 50, NULL, NULL, NULL, NULL)) == NULL) return 0;
    ShowWindow(hWnd, SW_HIDE);

    LPDIRECT3D9            pD3D;
    LPDIRECT3DDEVICE9    pD3Ddev;
    if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) != NULL)
    {
        ZeroMemory(&d3dpp, sizeof(d3dpp));
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.hDeviceWindow = hWnd;
        d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
        d3dpp.BackBufferWidth = d3dpp.BackBufferHeight = 600;
        pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pD3Ddev);
        if (pD3Ddev) {
            pInterface = (PDWORD)*(DWORD *)pD3Ddev;
            // копируем таблицу функций устройства
            memcpy(tbl, (void *)pInterface, size);
            pD3Ddev->Release();
        }
        pD3D->Release();
    }
    DestroyWindow(hWnd);
    return pInterface;
}

//-----------------------------------------------------------------------------------------------------------------------------------
//LRESULT CALLBACK MsgProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam){return DefWindowProc(hwnd, uMsg, wParam, lParam);}
//void DX_Init(DWORD* table)
//{
//    WNDCLASSEX wc = {sizeof(WNDCLASSEX),CS_CLASSDC,MsgProc,0L,0L,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,"DX",NULL};
//    RegisterClassEx(&wc);
//    HWND hWnd = CreateWindow("DX",NULL,WS_OVERLAPPEDWINDOW,100,100,300,300,GetDesktopWindow(),NULL,wc.hInstance,NULL);
//    LPDIRECT3D9 pD3D = Direct3DCreate9( D3D_SDK_VERSION );
//    D3DPRESENT_PARAMETERS d3dpp;
//    ZeroMemory( &d3dpp, sizeof(d3dpp) );
//    d3dpp.Windowed = TRUE;
//    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
//    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
//    LPDIRECT3DDEVICE9 pd3dDevice;
//    pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&d3dpp,&pd3dDevice);
//    DWORD* pVTable = (DWORD*)pd3dDevice;
//    pVTable = (DWORD*)pVTable[0];

//    table[ES]   = pVTable[42];                    //EndScene address
//    table[DIP]  = pVTable[82];                    //DrawIndexedPrimitive address
//    table[RES]  = pVTable[16];                    //Reset address

//    DestroyWindow(hWnd);
//}
//------------------------------------------------------------------------------------------------------------------------------------

//void HookDevice9Methods()
//{
//    DWORD vTable[105];
//    HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
//    pEndScene = (oEndScene)((DWORD)hD3D9 + dwEndScene);
//    jmpbES[0] = 0xE9;
//    DWORD addr = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
//    memcpy(jmpbES + 1, &addr, sizeof(DWORD));
//    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
//    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
//    memcpy((PBYTE)pEndScene, jmpbES, 5);
//    if (D3Ddiscover((void *)&vTable[0], 420) == 0) return;
//    {
//        Sleep(100);
//    }
//}


void HookEndScene()
{
    qDebug() << "HookEndScene";
    DWORD hD3D9 = NULL;
    while (!hD3D9) hD3D9 = (DWORD)GetModuleHandle(L"d3d9.dll");

    qDebug() << "pEndScene";
    pEndScene = (oEndScene)(hD3D9 + dwEndScene);
    jmpbES[0] = 0xE9;
    DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addrES, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    qDebug() << "pEndScene" << (PBYTE)pEndScene << jmpbES;
    memcpy((PBYTE)pEndScene, jmpbES, 5);
    Sleep(1000);
    qDebug() << "pReset";
    pReset = (oReset)((DWORD)hD3D9 + dwReset);
    jmpbRES[0] = 0xE9;
    DWORD addrRES = (DWORD)HookedReset - (DWORD)pReset - 5;
    memcpy(jmpbRES + 1, &addrRES, sizeof(DWORD));
    memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
    VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
    qDebug() << "pReset" << (PBYTE)pReset << jmpbRES;
    memcpy((PBYTE)pReset, jmpbRES, 5);
    qDebug() << "HookEndScene return";
}

void HookDevice9Methods(){
    DWORD *VTable;
    DWORD hD3D9 = NULL;
    while (!hD3D9) hD3D9 = (DWORD)GetModuleHandle(L"d3d9.dll");
    DWORD PPPDevice = FindPattern(hD3D9, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    memcpy( &VTable, (void *)(PPPDevice + 2), 4);

    pEndScene = (oEndScene)(VTable[42]);
    qDebug() << "pEndScene" << (PDWORD)pEndScene;
    jmpbES[0] = 0xE9;
    DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addrES, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    qDebug() << "pEndScene" << (PBYTE)pEndScene << jmpbES;
    memcpy((PBYTE)pEndScene, jmpbES, 5);
    Sleep(1000);
    pReset = (oReset)(VTable[16]);
    qDebug() << "pReset" << (PDWORD)pReset;
    jmpbRES[0] = 0xE9;
    DWORD addrRES = (DWORD)HookedReset - (DWORD)pReset - 5;
    memcpy(jmpbRES + 1, &addrRES, sizeof(DWORD));
    memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
    VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
    qDebug() << "pReset" << (PBYTE)pReset << jmpbRES;
    memcpy((PBYTE)pReset, jmpbRES, 5);
}



DWORD WINAPI Hook(LPVOID Param)
{
//    QMap<QString, int> virtual_key_codes;
//    virtual_key_codes.insert("MouseLeft", 0x01);
//    virtual_key_codes.insert("MouseRight", 0x02);
//    virtual_key_codes.insert("MouseMiddle", 0x04);
//    virtual_key_codes.insert("MouseTop", 0x05);
//    virtual_key_codes.insert("MouseBottom", 0x06);
//    virtual_key_codes.insert("Backspace", 0x08);
//    virtual_key_codes.insert("Tab", 0x09);
//    virtual_key_codes.insert("Enter", 0x0D);
//    virtual_key_codes.insert("Shift", 0x10);
//    virtual_key_codes.insert("Control", 0x11);
//    virtual_key_codes.insert("Alt", 0x12);
//    virtual_key_codes.insert("Pause", 0x13);
//    virtual_key_codes.insert("CapsLock", 0x14);
//    virtual_key_codes.insert("Escape", 0x1B);
//    virtual_key_codes.insert("Space", 0x20);
//    virtual_key_codes.insert("PageUp", 0x21);
//    virtual_key_codes.insert("PageDown", 0x22);
//    virtual_key_codes.insert("End", 0x23);
//    virtual_key_codes.insert("Home", 0x24);
//    virtual_key_codes.insert("Left", 0x25);
//    virtual_key_codes.insert("Up", 0x26);
//    virtual_key_codes.insert("Right", 0x27);
//    virtual_key_codes.insert("Down", 0x28);
//    virtual_key_codes.insert("PrintScreen", 0x2C);
//    virtual_key_codes.insert("Insert", 0x2D);
//    virtual_key_codes.insert("Delete", 0x2E);
//    virtual_key_codes.insert("Numpad0", 0x60);
//    virtual_key_codes.insert("Numpad1", 0x61);
//    virtual_key_codes.insert("Numpad2", 0x62);
//    virtual_key_codes.insert("Numpad3", 0x63);
//    virtual_key_codes.insert("Numpad4", 0x64);
//    virtual_key_codes.insert("Numpad5", 0x65);
//    virtual_key_codes.insert("Numpad6", 0x66);
//    virtual_key_codes.insert("Numpad7", 0x67);
//    virtual_key_codes.insert("Numpad8", 0x68);
//    virtual_key_codes.insert("Numpad9", 0x69);
//    virtual_key_codes.insert("NumpadMultiply", 0x6A);
//    virtual_key_codes.insert("NumpadPlus", 0x6B);
//    virtual_key_codes.insert("NumpadSeparator", 0x6C);
//    virtual_key_codes.insert("NumpadMinus", 0x6D);
//    virtual_key_codes.insert("NumpadPeriod", 0x6E);
//    virtual_key_codes.insert("NumpadSlash", 0x6F);
//    virtual_key_codes.insert("F1", 0x70);
//    virtual_key_codes.insert("F2", 0x71);
//    virtual_key_codes.insert("F3", 0x72);
//    virtual_key_codes.insert("F4", 0x73);
//    virtual_key_codes.insert("F5", 0x74);
//    virtual_key_codes.insert("F6", 0x75);
//    virtual_key_codes.insert("F7", 0x76);
//    virtual_key_codes.insert("F8", 0x77);
//    virtual_key_codes.insert("F9", 0x78);
//    virtual_key_codes.insert("F10", 0x79);
//    virtual_key_codes.insert("F11", 0x7A);
//    virtual_key_codes.insert("F12", 0x7B);
//    virtual_key_codes.insert("NumLock", 0x90);
//    virtual_key_codes.insert("ScrollLock", 0x91);
//    virtual_key_codes.insert("Semicolon", 0xBA);
//    virtual_key_codes.insert("Equal", 0xBB);
//    virtual_key_codes.insert("Comma", 0xBC);
//    virtual_key_codes.insert("Minus", 0xBD);
//    virtual_key_codes.insert("Period", 0xBE);
//    virtual_key_codes.insert("Slash", 0xBF);
//    virtual_key_codes.insert("LBracket", 0xDB);
//    virtual_key_codes.insert("Backslash", 0xDC);
//    virtual_key_codes.insert("RBracket", 0xDD);
//    virtual_key_codes.insert("Apostrophe", 0xDE);
//    virtual_key_codes.insert("Grave", 0xC0);


    UNREFERENCED_PARAMETER(Param);
    bool enableDXHook = false;
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TGameInfo), L"DXHook-Shared-Memory");
    if(hSharedMemory != nullptr)
    {
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if(lpSharedMemory!=nullptr){
//        memset(lpSharedMemory, 0, sizeof(TGameInfo));
        QSettings settings("stats.ini", QSettings::IniFormat);
        enableDXHook = settings.value("settings/enableDXHook", true).toBool();
//            enableDXHook = lpSharedMemory->enableDXHook;
        qDebug() << "enableDXHook:" << enableDXHook;
        version_str = settings.value("info/version", "0.0.0").toString();
//            version = lpSharedMemory->version;
        qDebug() << "Stats version:" << version_str;
//        QStringList baseKeys = QStringList(virtual_key_codes.keys());
//        HKL kbLayout = LoadKeyboardLayoutA("00000409", 1);
//        if(kbLayout)hotKey = settings.value("utils/showMenuHK", "F9").toString();
//        qDebug() << "Combination of keys to show / hide:" << hotKey;
//        QStringList keys = hotKey.split("+");
//        foreach (QString key, keys) {
//            if(key.size()==1){
//                qDebug() << key << QString::number((uchar)VkKeyScanExA(key.at(0).toAscii(), kbLayout), 16);
//                key_comb.append((uchar)VkKeyScanExA(key.at(0).toAscii(), kbLayout));
//            }
//            else{
//                if(baseKeys.contains(key, Qt::CaseInsensitive)){
//                    int index = baseKeys.indexOf(QRegExp(key, Qt::CaseInsensitive));
//                    qDebug() << key << QString::number(virtual_key_codes.values().at(index), 16);
//                    key_comb.append(virtual_key_codes.values().at(index));
//                }
//            }
//        }
//        if(key_comb.isEmpty()) key_comb.append(VK_F9);

//        fontSize = settings.value("utils/fontSize", 14).toInt();
//        bool bStatus = false;
//        font = settings.value("utils/font", "Gulim").toString().toStdString();
////        uint icolor = settings.value("utils/color", "0xFFA500").toString().toUInt(&bStatus,16);
//        uint icolor = QString("0xFFA500").toUInt(&bStatus,16);
//        if(bStatus)
//            fontColor = D3DCOLOR_ARGB(255, icolor / 0x10000,
//                                  (icolor / 0x100) % 0x100,
//                                  icolor % 0x100);
        }
        else
            qDebug() << "MapViewOfFile: Error " << GetLastError();
    }
    else
        qDebug() << "CreateFileMapping: Error " << GetLastError();

    if (enableDXHook)
    {
//        qDebug() << "Direct3DCreate9_VMTable" << Direct3DCreate9_VMTable();
//        GetDevice9Methods();
//        HookEndScene();
        HookDevice9Methods();
    }


//    HANDLE hToken;
//    PROCESS_INFORMATION pi;
//    STARTUPINFOA si;

//    ZeroMemory( &si, sizeof(si) );
//    ZeroMemory( &pi, sizeof(pi) );
//    si.cb = sizeof(si);
//    si.wShowWindow = SW_HIDE;
//    si.dwFlags = STARTF_USESHOWWINDOW;

//    DWORD iReturnVal = 0;
//    if (!LogonUser(
//         "franki",
//         NULL,
//         "franki",
//         LOGON32_LOGON_INTERACTIVE,
//         LOGON32_PROVIDER_DEFAULT,
//         &hToken
//         ))
//         return RTN_ERROR;
//    CreateProcessAsUserA()
    TCHAR NPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, NPath);

    bool success = false;
    SHELLEXECUTEINFO ShExecInfo;
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = NULL;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = L"runas";
    ShExecInfo.lpFile = L"SSStatsUpdater.exe";
    ShExecInfo.lpParameters = NULL;
    ShExecInfo.lpDirectory = NPath;
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    success = ShellExecuteEx(&ShExecInfo);

//    if(CreateProcessA(NULL, "SSStatsUpdater.exe", NULL, NULL, FALSE,
//    CREATE_NEW_PROCESS_GROUP|CREATE_DEFAULT_ERROR_MODE|DETACHED_PROCESS, NULL, NULL, &si, &pi))
//        qDebug() << "process SSStatsUpdater.exe created";
//    else
//        qDebug() << "process SSStatsUpdater.exe was not created" << GetLastError();
////        iReturnVal = GetLastError();
//    if(pi.hProcess)CloseHandle(pi.hProcess);
//    if(pi.hThread)CloseHandle(pi.hThread);
//    return iReturnVal==0;
//    QString args = qt_create_commandline(program, arguments);

//    QProcess ssstats;
//    success = ssstats.startDetached("SSStatsUpdater.exe", {}, QString::fromWCharArray(NPath));
//    ssstats.execute("SSStatsUpdater.exe");
//    success = ssstats.startDetached("SSStatsUpdater.exe");
//    LPTSTR lpszCurrentVariable;
//    LPTSTR lpszVariable;

//    DWORD dwFlags=0;
//    TCHAR chNewEnv[BUFSIZE];

//    // If the returned pointer is NULL, exit.
//    if (lpvEnv)
//    {
//        lpszVariable = (LPTSTR) lpvEnv;

////        while (*lpszVariable)
////        {
////            _tprintf("%s\n", lpszVariable);
////            lpszVariable += lstrlen(lpszVariable) + 1;
////        }
//        FreeEnvironmentStrings(lpvEnv);
//    }


//    // Copy environment strings into an environment block.

//    lpszCurrentVariable = (LPTSTR) chNewEnv;
//    if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, TEXT("MySetting=A"))))
//    {
//        printf("String copy failed\n");
//        return FALSE;
//    }

//    lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;
//    if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, TEXT("MyVersion=2"))))
//    {
//        printf("String copy failed\n");
//        return FALSE;
//    }

//    // Terminate the block with a NULL byte.

//    lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;
//    *lpszCurrentVariable = (TCHAR)0;

//    PROCESS_INFORMATION pinfo;

////    STARTUPINFOA startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
////                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
////                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
////                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
////                               };
//    STARTUPINFO si;
//    SecureZeroMemory(&si, sizeof(STARTUPINFO));
//    si.cb = sizeof(STARTUPINFO);
////    success = CreateProcessA(0, "SSStatsUpdater.exe",
////                            0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE, 0,
////                            NULL, &startupInfo, &pinfo);
//    qDebug() << QString::fromWCharArray(lpszCurrentVariable) << sizeof(lpszCurrentVariable);
//    qDebug() << QString::fromWCharArray(NPath);
//    qDebug() << QString::fromWCharArray(lpszVariable);
//    dwFlags = CREATE_NEW_PROCESS_GROUP|
//            CREATE_DEFAULT_ERROR_MODE   |
//            DETACHED_PROCESS;
//    #ifdef UNICODE
//        dwFlags |= CREATE_UNICODE_ENVIRONMENT;
//    #endif

//    success = CreateProcess(L"SSStatsUpdater.exe", NULL, NULL, NULL, FALSE, dwFlags,
//                            NULL, NPath, &si, &pinfo);
//    (LPVOID)lpszCurrentVariable
    if (success) {
        qDebug() << "process SSStatsUpdater.exe created";
//        CloseHandle(pinfo.hThread);
//        CloseHandle(pinfo.hProcess);
    }else
        qDebug() << "process SSStatsUpdater.exe was not created" << GetLastError();


//    system("SSStatsUpdater.exe");

    return 0;
}

BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason==DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);
        logger.installLog();
        qDebug() << "----------Attached----------";
        CreateThread(NULL,0,Hook,NULL,0,NULL);
//        Hook(NULL);
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
        qDebug() << "----------Detached----------";
        logger.finishLog();
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hSharedMemory);
    }

    return TRUE;
}


//=====================================================================================

HRESULT WINAPI Direct3DCreate9_VMTable(VOID)
{
    LPDIRECT3D9 Direct3D_Object = Direct3DCreate9(D3D_SDK_VERSION);

    if(Direct3D_Object == NULL)
        return D3DERR_INVALIDCALL;
    Direct3D_VMTable = (PDWORD)*(PDWORD)Direct3D_Object;
    Direct3D_Object->Release();

    DWORD dwProtect;
    qDebug() << "Direct3D_VMTable" << Direct3D_VMTable;
    if(VirtualProtect(&Direct3D_VMTable[16], sizeof(DWORD), PAGE_READWRITE, &dwProtect) != 0)
    {
        qDebug() << "hook CreateDevice";
        *(PDWORD)&CreateDevice_Pointer = Direct3D_VMTable[16];
        *(PDWORD)&Direct3D_VMTable[16] = (DWORD)CreateDevice_Detour;

        if(VirtualProtect(&Direct3D_VMTable[16], sizeof(DWORD), dwProtect, &dwProtect) == 0)
            return D3DERR_INVALIDCALL;
    }
    else
        return D3DERR_INVALIDCALL;

    return D3D_OK;
}

//=====================================================================================
LPDIRECT3DDEVICE9* Ret_pDevice;
HRESULT WINAPI CreateDevice_Detour(LPDIRECT3D9 Direct3D_Object, UINT Adapter, D3DDEVTYPE DeviceType, HWND FocusWindow,
                    DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* PresentationParameters,
                    LPDIRECT3DDEVICE9* Returned_pDevice)
{
//    if(hookThrHandle){
//        qDebug() << "terminating hook thead";
//        TerminateThread(hookThrHandle, 0);
//    }
    Ret_pDevice = Returned_pDevice;
    qDebug() << "CreateDevice_Detour" << Direct3D_Object << Returned_pDevice;
    HRESULT Returned_Result = CreateDevice_Pointer(Direct3D_Object, Adapter, DeviceType, FocusWindow, BehaviorFlags,
                                              PresentationParameters, Returned_pDevice);
    qDebug() << "Device created";
//    DWORD dwProtect;

//    if(VirtualProtect(&Direct3D_VMTable[16], sizeof(DWORD), PAGE_READWRITE, &dwProtect) != 0)
//    {
//        qDebug() << "unhook CreateDevice";
//        *(PDWORD)&Direct3D_VMTable[16] = *(PDWORD)&CreateDevice_Pointer;
//        CreateDevice_Pointer           = NULL;

//        if(VirtualProtect(&Direct3D_VMTable[16], sizeof(DWORD), dwProtect, &dwProtect) == 0)
//            return D3DERR_INVALIDCALL;
//    }
//    else
//        return D3DERR_INVALIDCALL;


    if(Returned_Result == D3D_OK)
    {
        Direct3D_VMTable = (PDWORD)*(PDWORD)*Returned_pDevice;

        *(PDWORD)&pReset                = (DWORD)Direct3D_VMTable[16];
        *(PDWORD)&pEndScene             = (DWORD)Direct3D_VMTable[42];
//        *(PDWORD)&DrawIndexedPrimitive_Pointer = (DWORD)Direct3D_VMTable[82];

        qDebug() << "hook functions" << (PBYTE)pEndScene << (PBYTE)pReset;
        qDebug() << "hook functions" << Direct3D_VMTable << *Returned_pDevice << Returned_pDevice;


        if(curEndScene!=(PBYTE)pEndScene){
            qDebug() << "hook";
            curEndScene = (PBYTE)pEndScene;
            DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
            memcpy(jmpbES + 1, &addrES, 4);
            memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
            VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
            memcpy((PBYTE)pEndScene, jmpbES, 5);

            Sleep(1000);

            DWORD addrRES = (DWORD)HookedReset - (DWORD)pReset - 5;
            memcpy(jmpbRES + 1, &addrRES, 4);
            memcpy(CodeFragmentRES, (PBYTE)pReset, 5);
            VirtualProtect((PBYTE)pReset, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectRES);
            memcpy((PBYTE)pReset, jmpbRES, 5);
        }



//        curDevice = *Returned_pDevice;
//        hookThrHandle = CreateThread(NULL,0,VirtualMethodTableRepatchingLoopToCounterExtensionRepatching,NULL,0,NULL);

//        if(!hookThrHandle)
//            return D3DERR_INVALIDCALL;
    }

    return Returned_Result;
}

//=====================================================================================

DWORD WINAPI VirtualMethodTableRepatchingLoopToCounterExtensionRepatching(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);

    while(1) {
        qDebug() << "VMTRLTCER" << curDevice << *Ret_pDevice;
        Sleep(1000);
        HookVTableFunction((PDWORD*)*Ret_pDevice, (PBYTE)EndScene_Detour, 42); //Hook EndScene

//        HookVTableFunction((PDWORD*)curDevice, (PBYTE)DrawIndexedPrimitive_Detour, 82); //Hook DrawIndexedPrimitive
//        HookVTableFunction((PDWORD*)curDevice, (PBYTE)EndScene_Detour, 42); //Hook EndScene
//        HookVTableFunction((PDWORD*)curDevice, (PBYTE)Reset_Detour, 16); //Hook Reset
    }
    return 1;
}

//=====================================================================================

PBYTE HookVTableFunction( PDWORD* dwVTable, PBYTE dwHook, INT Index )
{
    DWORD dwOld = 0;
    VirtualProtect((void*)((*dwVTable) + (Index*4) ), 4, PAGE_EXECUTE_READWRITE, &dwOld);

    PBYTE pOrig = ((PBYTE)(*dwVTable)[Index]);
    (*dwVTable)[Index] = (DWORD)dwHook;

    VirtualProtect((void*)((*dwVTable) + (Index*4)), 4, dwOld, &dwOld);

    return pOrig;
}

//=====================================================================================

HRESULT WINAPI DrawIndexedPrimitive_Detour(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseIndex,
                                           UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
    LPDIRECT3DVERTEXBUFFER9 Stream_Data;
    UINT Offset = 0;
    UINT Stride = 0;

    if(pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
    Stream_Data->Release();

    if(Stride == 0)
    {
    }

    return DrawIndexedPrimitive_Pointer(pDevice, Type, BaseIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}

//=====================================================================================

HRESULT WINAPI Reset_Detour(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* PresentationParameters)
{
    qDebug() << "Reset is hooked";
    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Draw.OnLostDevice();
    HRESULT hRet = Reset_Pointer(pDevice, PresentationParameters);
    if(hRet == D3D_OK) {
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Draw.OnResetDevice();
    }
    return hRet;
}

//=====================================================================================
HRESULT WINAPI EndScene_Detour(LPDIRECT3DDEVICE9 pDevice)
{
//    qDebug() << "EndScene is hooked";
    paint(pDevice);
    return EndScene_Pointer(pDevice);
}

//=====================================================================================

void paint(LPDIRECT3DDEVICE9 pDevice)
{
    if(oldDevice!=pDevice){
        Draw.SetDevice(pDevice);
        Render.setDevice(pDevice);
        Render.ReleaseFonts();
        initFonts(pDevice);
    }

//    QString version_str = version? " " + QString::number(version).insert(1, '.').insert(3, '.'):"";
//    QString version_str = QString(QString::number(version/100)+'.'+
//            QString::number(version/100)+'.'+
//            QString::number(version%100));
//    if(lpSharedMemory->showMenu)
//        Draw.Text("Soulstorm Ladder 1.0.5", width*0.01,height*0.01, 5, true, ORANGE(128), BLACK(64));

    if(lpSharedMemory->showMenu){
        Render.Init_PosMenu(width-201, 0, QString("Soulstorm Ladder "+version_str).toStdString().data(), &Render.pos_Menu, lpSharedMemory);
    }

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
                Draw.Draw_Box(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,DARKGRAY(150));
                Draw.Draw_Border(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,1,SKYBLUE(255));
                Draw.String(center_text_pos-(text_w+8)/2+4,/*height*0.02+*/i*(mainFontSize+14)+4, fontColor, player_info.data(), 0, DT_LEFT|DT_NOCLIP);
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
            Draw.Draw_Box(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,mainFontSize+12,DARKGRAY(150));
            Draw.Draw_Border(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,mainFontSize+12,1,SKYBLUE(255));
            Draw.Text(str.data(), center_text_pos-(text_w+12)/2+6, /*height*0.02+*/6, 0, false, fontColor, fontColor);
        }
    }

//    string mapName(&lpSharedMemory->mapName[0]);
//    if(!mapName.empty()){
//        int p = lpSharedMemory->downloadProgress/10;
//        if(p!=0){
//            string progress = string(p, '#')+string(10-p,' ')+' '+to_string(lpSharedMemory->downloadProgress);
//            Draw.Text(string("Downloading: "+mapName).data(), width*0.35,height*0.01, 0, false, fontColor, fontColor);
//            Draw.Text(progress.data(), width*0.35,height*0.02, 0, false, fontColor, fontColor);
//            Draw.Draw_Border(width*0.35-5,height*0.02, 13+10*mainFontSize/2, 18,1,SKYBLUE(255));
//            for(int i=1; i<=p; ++i)
//                Draw.Draw_Box(width*0.35-3+(i-1)*mainFontSize/2+i-1,height*0.02+2, p*mainFontSize/2, mainFontSize, ORANGE(255));
//        }else
//            Draw.Text("Please re-join", width*0.35,height*0.01, 0, false, fontColor, fontColor);
//    }
}

Q_DECL_EXPORT void Inject(void)
{
}
