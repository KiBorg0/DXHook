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
//#include "..\SSStats\systemwin32.h"

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
//typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice/*, const RECT* src, const RECT* dest, HWND hWnd, const RGNDATA* unused*/);
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
bool closeWithGame = false;
bool mCanCurrentlyRender = true;
QString version_str = "0.0.0";
D3DCOLOR fontColor = ORANGE(255);

#define Detour_Type_0xE9 1
#define Detour_Type_0xB8 2
#define Detour_Type_0x68 3
#define Detour_Type_0xE8 4

DWORD CreateDetour(DWORD dwThread,DWORD dwAdress,DWORD dwType,DWORD dwSize)
{
        DWORD dwDetour,dwProtect,i;
        if (dwAdress&&dwThread&&dwSize>= dwSize)
        {
                dwDetour = (DWORD)VirtualAlloc(0,dwSize+dwSize,0x1000,0x40);
                if (dwDetour&&VirtualProtect((VOID*)dwAdress,dwSize,0x40,&dwProtect))
                {
                        for (i=0;i<dwSize;i++)
                        {
                                *(BYTE*)(dwDetour+i)=*(BYTE*)(dwAdress+i);
                        }
                        switch (dwType)
                        {
                            case Detour_Type_0xE9:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0xE9;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress-dwDetour-dwSize);
                                    *(BYTE*)(dwAdress+0)=0xE9;
                                    *(DWORD*)(dwAdress+1)=(dwThread-dwAdress-dwSize);
                                }
                                break;
                                case Detour_Type_0xB8:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0xB8;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
                                    *(WORD*)(dwDetour+dwSize+5)=0xE0FF;
                                    *(BYTE*)(dwAdress+0)=0xB8;
                                    *(DWORD*)(dwAdress+1)=(dwThread);
                                    *(WORD*)(dwAdress+5)=0xE0FF;
                                }
                                break;
                                case Detour_Type_0x68:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0x68;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
                                    *(WORD*)(dwDetour+dwSize+5)=0xC3;
                                    *(BYTE*)(dwAdress+0)=0x68;
                                    *(DWORD*)(dwAdress+1)=(dwThread);
                                    *(WORD*)(dwAdress+5)=0xC3;
                                }
                                break;
                        }
                        VirtualProtect((VOID*)dwAdress,dwSize,dwProtect,&dwProtect);
                        VirtualProtect((VOID*)dwDetour,dwSize+dwSize,0x20,&dwProtect);
                        return dwDetour;
                }
        }
        Sleep(10);
        return (0);
}

static LPDIRECT3DDEVICE9 pNDevice;
DWORD GetEndScene;
DWORD RetEndScene;

BYTE HOOK_PAT_8[] = { "\x8B\xFF\x55\x8B\xEC\xFF\x75\x08\x8B\x01\x6A\x3E\xFF\x90\xF4\x00" };
CHAR HOOK_MAS_8[] = { "xxxxxxxxxxxxxxx?" };

BYTE HOOK_PAT_7[] = { "\x8B\xFF\x55\x8B\xEC\x8B\x55\x08\x8B\x01\x8B\x80\xF4\x00\x00\x00\x52\x6A\x3E\xFF\xD0\x5D\xC2\x04\x00" };
CHAR HOOK_MAS_7[] = { "xxxxxxxxxxxxxxxxxxxxxxxxx" };

BYTE HOOK_PAT_XP[] = {"\x8B\xFF\x55\x8B\xEC\x8B\x55\x08\x8B\x01\x52\x6A\x3E"};
CHAR HOOK_MAS_XP[] = {"xxxxxxxxxxxxx"};


void InstallES( LPDIRECT3DDEVICE9 pNDevice )
{
    UNREFERENCED_PARAMETER(pNDevice);
    //Draw Something
}

void MyRender()
{
    qDebug() << "MyRender";
    InstallES(pNDevice);
}

__declspec() void MidFunction_EndScene()
{
    asm volatile (
        "mov %edi, %edi\n\t"
        "push %ebp\n\t"
        "mov %ebp, %esp\n\t");
    asm volatile (
        "mov %0, %%esi\n\t"
        :"=r"(pNDevice));
    asm volatile ("pushal\n\t"
        "call %P0\n\t"::"i"(MyRender)
        );
    asm volatile ("popal\n\t"
        "jmp *%0\n\t"
        :"=r"(RetEndScene)
        );
//        : "r"(pNDevice)

//        : "r"(RetEndScene)
//    );
//    asm volatile ("call %P0\n\t": :"i"(MyRender));
//    __asm__ (".intel_syntax noprefix\n\t"
//    "mov edi, edi\n\t"
//    "push ebp\n\t"
//    "mov ebp, esp\n\t"
//    "mov pNDevice, esi\n\t"
//    "pushad\n\t"
//    "call[MyRender]\n\t"
//    "popad\n\t"
//    "jmp[RetEndScene]\n\t"
//    );


//    __asm
//    {
//        mov edi, edi
//        push ebp
//        mov ebp, esp
//        mov pNDevice, esi
//        pushad
//        call[Render]
//        popad
//        jmp[RetEndScene]
//    }
}

bool CheckWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwProductType)
{
    OSVERSIONINFOEX VersionInfo;
    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFOEX));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionEx((OSVERSIONINFO*) &VersionInfo);
    if (VersionInfo.dwMajorVersion == dwMajorVersion)
    {
        if (VersionInfo.dwMinorVersion == dwMinorVersion)
        {
            if (VersionInfo.wProductType == dwProductType)
            {
                qDebug() << dwMajorVersion << dwMinorVersion << "VER_NT_WORKSTATION";
                return (TRUE);
            }
        }
    }
    return (FALSE);
}


unsigned __stdcall InstallHook(LPVOID Param)
{
    UNREFERENCED_PARAMETER(Param);
    DWORD hd3d9 = 0;
    while (!hd3d9) hd3d9 = (DWORD)GetModuleHandle(L"d3d9.dll");
    DWORD *vtbl;
    DWORD adr;
    if (CheckWindowsVersion(6, 2, VER_NT_WORKSTATION)) // Windows 8 / 8.1
    {
        qDebug() << "GetEndScene";
        GetEndScene = FindPattern((DWORD) hd3d9, 0xFFFFFF, (PBYTE) HOOK_PAT_8, (PCHAR) HOOK_MAS_8);
    }
    else if (CheckWindowsVersion(6, 0, VER_NT_WORKSTATION) || CheckWindowsVersion(6, 1, VER_NT_WORKSTATION)) // Windows 7 / Vista
    {
        GetEndScene = FindPattern((DWORD) hd3d9, 0xFFFFFF, (PBYTE) HOOK_PAT_7, (PCHAR) HOOK_MAS_7);
    }
    else if (CheckWindowsVersion(5, 1, VER_NT_WORKSTATION) || CheckWindowsVersion(5, 2, VER_NT_WORKSTATION)) // Windows XP
    {
        GetEndScene = FindPattern((DWORD) hd3d9, 0xFFFFFF, (PBYTE) HOOK_PAT_XP, (PCHAR) HOOK_MAS_XP);
    }
    qDebug() << "adr";
    adr = FindPattern((DWORD) hd3d9, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    qDebug() << "memcpy";
    memcpy(&vtbl, (void*) (adr + 2), 4);
    RetEndScene = GetEndScene + 0x5;
//    RetEndScene = (oEndScene)(VTable[42]);
//    RetEndScene = vtbl[42];
    qDebug() << "CreateDetour";
    CreateDetour((DWORD) MidFunction_EndScene, (DWORD) GetEndScene, Detour_Type_0xE9, 5);
    qDebug() << "InstallHook return";
    return 1;
}

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
    if(height<640){
        mainFontSize =  12;
        titleFontSize = 12;
    } else if(height<768){
        mainFontSize =  14;
        titleFontSize = 14;
    } else if(height<1024){
        mainFontSize =  16;
        titleFontSize = 16;
    } else if(height<1280){
        mainFontSize =  18;
        titleFontSize = 18;
    } else if(height<1600){
        mainFontSize =  20;
        titleFontSize = 20;
    } else if(height<2200){
        mainFontSize =  22;
        titleFontSize = 22;
    }
    qDebug() << "screen width:" << width
             << "screen height:" << height
             << "font size:" << mainFontSize;
    Render.setMenuParams(titleFontSize, width, height);

    qDebug() << "Release fonts";
    Draw.SetDevice(pDevice);
    Draw.ReleaseFonts();
    Render.setDevice(pDevice);
    Render.ReleaseFonts();

    qDebug() << "Init fonts";
    Draw.AddFont("Gulim", mainFontSize, false, false);
//    Draw.AddFont("Tahoma", 15, false, false);
//    Draw.AddFont("Verdana", 15, true, false);
//    Draw.AddFont("Verdana", 13, true, false);
//    Draw.AddFont("Comic Sans MS", 30, true, false);
    bool success = false;
    if(AddFontResourceA("Engine/Locale/English/data/font/engo.ttf"))
        success = Render.AddFont("GothicRus", titleFontSize, false, false);
    if(!success)
        Render.AddFont("Arial", titleFontSize, false, false);
//    success = false;
//    if(AddFontResourceA("Engine/Locale/English/data/font/ansnb___.ttf"))
//        success = Render.AddFont("Arial Unicode MS", mainFontSize, false, false);
//    if(!success)
    Render.AddFont("Arial", mainFontSize, false, false);
}

// проблема заключается в стимовском оверлее,
// дело в том что выполняя сброс устройства здесь,
// мы не ждем пока освободятся ресурсы стимовского хука
// из за этого резет не выполняется
HRESULT APIENTRY HookedReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters )
{
    if(oldDevice!=pDevice) initFonts(pDevice);

    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << DXGetErrorString9A(hr);
//    {

//        // Device lost, stay idle till we can reset it
//        if (hr == D3DERR_DEVICELOST)
//            Sleep(50);
//        else if (hr == D3DERR_DEVICENOTRESET)
//    }

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
//    Draw.OnLostDevice();
//    Render.ReleaseFonts();
    memcpy((void *)pReset, CodeFragmentRES, 5);
    hr = pDevice->Reset(pPresentationParameters);
    memcpy((void *)pReset, jmpbRES, 5);

    if(SUCCEEDED(hr)) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
//        Draw.OnResetDevice();
//        Render.AddFont("Arial", titleFontSize, false, false);
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr);


    return hr;
}

//bool beginSceneInternal(LPDIRECT3DDEVICE9 mD3DDevice)
//{
//   // Make sure we have a device
//   HRESULT res = mD3DDevice->TestCooperativeLevel();

//   int attempts = 0;
//   const int MaxAttempts = 40;
//   const int SleepMsPerAttempt = 50;
//   while(res == D3DERR_DEVICELOST && attempts < MaxAttempts)
//   {
//      // Lost device! Just keep querying
//      res = mD3DDevice->TestCooperativeLevel();

////      Con::warnf("GFXD3D9Device::beginScene - Device needs to be reset, waiting on device...");
//      qDebug() << "GFXD3D9Device::beginScene - Device needs to be reset, waiting on device...";
//      Sleep(SleepMsPerAttempt);
//      attempts++;
//   }

//   if (attempts >= MaxAttempts && res == D3DERR_DEVICELOST)
//   {
////      Con::errorf("GFXD3D9Device::beginScene - Device lost and reset wait time exceeded, skipping reset (will retry later)");
//      mCanCurrentlyRender = false;
//      return false;
//   }

//   // Trigger a reset if we can't get a good result from TestCooperativeLevel.
//   if(res == D3DERR_DEVICENOTRESET)
//   {
////      Con::warnf("GFXD3D9Device::beginScene - Device needs to be reset, resetting device...");
//      qDebug() << "GFXD3D9Device::beginScene - Device needs to be reset, resetting device...";
//      // Reset the device!
//      GFXResource *walk = mResourceListHead;
//      while(walk)
//      {
//         // Find the window target with implicit flag set and reset the device with its presentation params.
//         if(GFXD3D9WindowTarget *gdwt = dynamic_cast<GFXD3D9WindowTarget*>(walk))
//         {
//            if(gdwt->mImplicit)
//            {
//               reset(gdwt->mPresentationParams);
//               break;
//            }
//         }

//         walk = walk->getNextResource();
//      }
//   }

//   HRESULT hr = mD3DDevice->BeginScene();
////   D3D9Assert(hr, "GFXD3D9Device::beginSceneInternal - failed to BeginScene");
//   mCanCurrentlyRender = SUCCEEDED(hr);
//   return mCanCurrentlyRender;
//}
//void PostReset(LPDIRECT3DDEVICE9 pDevice)
//{
//    CD3DFont *font = new CD3DFont("Tahoma", 7, D3DFONT_BOLD);
//    font->InitDeviceObjects(pDevice);
//    font->RestoreDeviceObjects();

//}
//void PreReset(void)
//{
//    font->InvalidateDeviceObjects();
//    font->DeleteDeviceObjects();
//    delete font;
//    font = NULL;
//}

HRESULT APIENTRY HookedEndScene(LPDIRECT3DDEVICE9 pDevice/*, const RECT* src, const RECT* dest, HWND hWnd, const RGNDATA* unused*/)
{
    HRESULT hRet = D3D_OK;
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
        qDebug() << "HookedEndScene TestCooperativeLevel" << DXGetErrorString9A(hr);
//    pDevice->BeginScene();
    paint(pDevice);
//    pDevice->EndScene();

    memcpy((void *)pEndScene, CodeFragmentES, 5);
//    hRet = pDevice->Present(src,dest,hWnd,unused);
    hRet = pDevice->EndScene();
    memcpy((void *)pEndScene, jmpbES, 5);

    if(FAILED(hRet))
        qDebug() << "HookedEndScene return is" << DXGetErrorString9A(hRet);

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

    if ((hWnd = CreateWindowEx(0, WC_DIALOG, L"", WS_OVERLAPPED, 0, 0, 50, 50, NULL, NULL, NULL, NULL)) == NULL) return 0;
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
    DWORD hD3D9 = 0;
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
    DWORD hD3D9 = 0;
    while (!hD3D9) hD3D9 = (DWORD)GetModuleHandle(L"d3d9.dll");
    DWORD PPPDevice = FindPattern(hD3D9, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
//    DWORD PPPDevice = FindPattern(hD3D9, 0x128000, (PBYTE)"\x8B\xFF\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x56\x8B\x75\x08\x85\xF6\x74\x48\x8D\x46\x04\x83\x64\x24\x0C\x00\x83\x78\x18\x00\x89\x44\x24\x08\x75\x3A\xF7\x46\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\x6A\x00\xFF\x75\x18\x8B\xCE\xFF\x75\x14\xFF\x75\x10\xFF\x75\x0C\xE8\x00\x00\x00\x00\x8B\xF0\x8D\x4C\x24\x08\xE8\x00\x00\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?????xx????xxxxxxxxxxxxxxxxx????xxxxxxx????");
    memcpy( &VTable, (void *)(PPPDevice + 2), 4);
//    pEndScene = (oEndScene)(PPPDevice + 0xC);
    pEndScene = (oEndScene)(VTable[42]);
    qDebug() << "pEndScene" << (PDWORD)pEndScene;
    jmpbES[0] = 0xE9;
    DWORD addrES = (DWORD)HookedEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addrES, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    qDebug() << "pEndScene" << (PBYTE)pEndScene << jmpbES;
    memcpy((PBYTE)pEndScene, jmpbES, 5);
//    Sleep(1000);
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
    UNREFERENCED_PARAMETER(Param);
    bool enableDXHook = false, runWithGame = true;
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TGameInfo), L"DXHook-Shared-Memory");
    if(hSharedMemory != nullptr)
    {
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if(lpSharedMemory!=nullptr){
        Render.setGameInfo(lpSharedMemory);
//        memset(lpSharedMemory, 0, sizeof(TGameInfo));
        QSettings settings("stats.ini", QSettings::IniFormat);
        runWithGame = settings.value("settings/runWithGame", true).toBool();
        enableDXHook = settings.value("settings/enableDXHook", true).toBool();
//            enableDXHook = lpSharedMemory->enableDXHook;
        qDebug() << "enableDXHook:" << enableDXHook;
        version_str = settings.value("info/version", "0.0.0").toString();
        closeWithGame = settings.value("settings/closeWithGame", false).toBool();
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
//        InstallHook(Param);
    }
    if(!runWithGame) return 0;

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

    QProcess schtasks;
    QTextCodec *codec = QTextCodec::codecForName("CP866");
    schtasks.start("schtasks", {"/Run", "/I", "/tn", "Soulstorm Ladder v2.0", "/HRESULT"});
    schtasks.waitForFinished();
    qDebug() << codec->toUnicode(schtasks.readAllStandardOutput()).remove("\r\n")
             << QString("%1").arg((ulong)schtasks.exitCode(),8,16,QLatin1Char('0')).toUpper();
    if (schtasks.exitCode()!=0){
        TCHAR NPath[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, NPath);
        SHELLEXECUTEINFO ShExecInfo;
        ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;
//        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_CLASSNAME;
//        ShExecInfo.lpClass = L"exefile";
        ShExecInfo.hwnd = NULL;
        ShExecInfo.lpVerb = L"runas";
//        ShExecInfo.lpFile = L"SSStatsUpdater.exe";
        ShExecInfo.lpFile = L"SSStats.exe";
        ShExecInfo.lpParameters = NULL;
        ShExecInfo.lpDirectory = NPath;
        ShExecInfo.nShow = SW_HIDE;
        ShExecInfo.hInstApp = NULL;
        qDebug() << QString::fromWCharArray(NPath);

        if(ShellExecuteEx(&ShExecInfo)/*QProcess::startDetached("SSStatsUpdater.exe")*/)
            qDebug() << "startDetached - Success!";
        else
            qDebug() << "startDetached - Failed!" << GetLastError();
        CloseHandle(ShExecInfo.hProcess);
    } else
        qDebug() << "Soulstorm Ladder task successfully runned";

//        qDebug() << "process SSStatsUpdater.exe was not created" << GetLastError();

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
//    if (return_code) {
//        qDebug() << "process SSStatsUpdater.exe created";
////        CloseHandle(pinfo.hThread);
////        CloseHandle(pinfo.hProcess);
//    }else
//        qDebug() << "process SSStatsUpdater.exe was not created" << GetLastError();


//    system("SSStatsUpdater.exe");

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
//        Hook(NULL);
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
//        if(closeWithGame){
//            qDebug() << "closeWithGame" << QProcess::execute("schtasks", {"/End", "/tn", "Soulstorm Ladder v2.0", "/HRESULT"});
//            systemWin32 processes;
//            if(processes.findProcess("SSStats.exe")){
//                processes.closeProcessByName("SSStats.exe");
//            }
//        }
        Draw.ReleaseFonts();
        Render.ReleaseFonts();
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
        CreateDevice_Pointer = (CreateDevice_Prototype)Direct3D_VMTable[16];
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

        pReset                = (oReset)Direct3D_VMTable[16];
        pEndScene             = (oEndScene)Direct3D_VMTable[42];
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
    if(oldDevice!=pDevice) initFonts(pDevice);
//    QString version_str = version? " " + QString::number(version).insert(1, '.').insert(3, '.'):"";
//    QString version_str = QString(QString::number(version/100)+'.'+
//            QString::number(version/100)+'.'+
//            QString::number(version%100));
//    if(lpSharedMemory->showMenu)
//        Draw.Text("Soulstorm Ladder 1.0.5", width*0.01,height*0.01, 5, true, ORANGE(128), BLACK(64));
//    if(!Draw.Font()||!Render.Font()){
//        Render.OnLostDevice();
//        Draw.OnLostDevice();
//        Render.ReleaseFonts();
//        initFonts(pDevice);
//    }
//    else{

    if(lpSharedMemory->showMenu)
        Render.Init_PosMenu(QString("Soulstorm Ladder "+version_str).toStdString().data());

//    HDC hdc;
//    PAINTSTRUCT ps;
//    hdc = CreateCompatibleDC(NULL);
//    hdc = BeginPaint(hWnd, &ps);
//    RECT rect{0,0,0,0};
//    HFONT hFont = CreateFontA(18, 0, 0, 0, 300, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
//    SelectObject(hdc, hFont);
//    SetTextColor(hdc, RGB(0,128,0));
//    DrawTextA(hdc, QString("Soulstorm Ladder "+version_str).toStdString().data(), -1, &rect, DT_LEFT|DT_NOCLIP);
//    if(Render.pFont[0])Render.pFont[0]->DrawTextA(NULL,QString("Soulstorm Ladder "+version_str).toStdString().data(),-1,&rect, DT_LEFT|DT_NOCLIP, ORANGE);

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
//        Render.OnLostDevice();
//        Draw.OnLostDevice();
//        Render.OnResetDevice();
//        Draw.OnResetDevice();
//    }
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
