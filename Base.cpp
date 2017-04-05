#include <Windows.h>
#include <QCoreApplication>
#include <d3d9.h>
#include <d3dx9.h>
#include <QDebug>
#include <stdio.h>
#define D3DparamX		, UINT paramx
#define D3DparamvalX	, paramx
//#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")

#include "Structure.h"
cFun Fun;


#include "Menu.h"

#include "cMemory.h"
cMemory *MemHack;


#include "cRender.h"
cRender Render;
ID3DXFont* pFont;

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
DWORD dwEndScene;
BYTE CodeFragmentES[5] = { 0, 0, 0, 0, 0 };
BYTE jmpbES[5] = { 0, 0, 0, 0, 0 };
DWORD dwOldProtectES = 0;

typedef HRESULT ( WINAPI* oReset )( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );//edit if you want to work for the game
typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
oEndScene pEndScene = NULL;
oReset pReset;

void __fastcall String(int x, int y, DWORD Color, DWORD Style, const char *Format, ...)
{
    RECT rect;
    SetRect(&rect, x, y, x, y);
    char Buffer[1024] = { '\0' };
    va_list va_alist;
    va_start(va_alist, Format);
    vsprintf_s(Buffer, Format, va_alist);
    va_end(va_alist);
    pFont->DrawTextA(NULL, Buffer, -1, &rect, Style, Color);
    return;
}

//HRESULT WINAPI Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters )
//{
//dMenu.pFont->OnLostDevice();

//HRESULT hRet = pReset(pDevice, pPresentationParameters);

//dMenu.pFont->OnResetDevice();

//return hRet;
//}

bool Init = false;
HRESULT APIENTRY myEndScene( LPDIRECT3DDEVICE9 pDevice )
{
    qDebug() << "hlop";

//    BYTE* CDES = (BYTE*)pEndScene;
//    CDES[0] = CodeFragmentES[0];
//    *((DWORD*)(CDES + 1)) = *((DWORD*)(CodeFragmentES + 1));

//    if (!Init)
//    {
//        Beep(100, 100);
//        D3DXCreateFont(pDevice, 13, 0, FW_BLACK, 0, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma", &pFont);
//        Init = true;
//    }

//    if (!pFont)
//        pFont->OnLostDevice();
//    else
//    {
//        String(10, 10, D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, "Моя первая DLL, с ипользованием DirectX библиотек");
//        pFont->OnLostDevice();
//        pFont->OnResetDevice();
//    }

//    DWORD res = pEndScene(pDevice);
//    CDES[0] = jmpbES[0];
//    *((DWORD*)(CDES + 1)) = *((DWORD*)(jmpbES + 1));

//    return res;

    if( !Render.Init )
    {
        qDebug() << D3DXCreateFont(pDevice, 15, 0, FW_BLACK,0, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, (WCHAR *)"Arial", &Render.pFont);
        Render.Init = true;
    }
	
    Render.Init_PosMenu(20,20,VK_END,"D3D9 Menu от А до Я",&Render.pos_Menu,pDevice);

    return pEndScene( pDevice );
}

void *DetourFunction (BYTE *src, const BYTE *dst, const int len)//edit if you want to work for the game
{
BYTE *jmp = (BYTE*)malloc(len+5);
DWORD dwBack;

VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwBack);
memcpy(jmp, src, len);
jmp += len;
jmp[0] = 0xE9;
*(DWORD*)(jmp+1) = (DWORD)(src+len - jmp) - 5;
src[0] = 0xE9;
*(DWORD*)(src+1) = (DWORD)(dst - src) - 5;
for (int i=5; i<len; i++) src[i]=0x90;
VirtualProtect(src, len, dwBack, &dwBack);
return (jmp-len);
}


int D3d9_Hook(void)
{
	DWORD*vtbl=0;
    // загрузка directx библиотеки
	DWORD hD3D9=(DWORD)LoadLibraryA("d3d9.dll");
    qDebug() << (PBYTE)hD3D9;
    // поиск комбинации байт по адресу загруженной в память библиотеки
	DWORD table=MemHack->FindPattern(hD3D9,0x128000,(PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86","xx????xx????xx");
    qDebug() << (PBYTE)table;

//    memcpy(&vtbl,(void*)(table+2),4);

//    qDebug() << &vtbl << (PBYTE)vtbl[42] << (PBYTE)vtbl[0];
//    qDebug() << vtbl << (PBYTE)myEndScene << (PBYTE)vtbl[42];
    if (table) {
    memcpy(&vtbl,(void *)(table+2),4);
//    pReset = (oReset) DetourFunction((PBYTE)vtbl[16] , (PBYTE)Reset ,5);
    pEndScene = (oEndScene) DetourFunction((PBYTE)vtbl[42], (PBYTE)myEndScene,5);
    }
//    pEndScene=(oEndScene)MemHack->Create_Hook((PBYTE)vtbl[42],(PBYTE)myEndScene,5);

//    qDebug() << (PBYTE)pEndScene;
    return 0;
}

//BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
//{
//	if (dwReason==DLL_PROCESS_ATTACH)
//	{
//		DisableThreadLibraryCalls(hDll);
//		new_My_Thread(D3d9_Hook);
//	}
//	return TRUE;
//}

void GetDevice9Methods()
{
    qDebug() << "GetDevice9Methods";
    HWND hWnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
//    WNDCLASSEX wc = { sizeof(WNDCLASSEX),CS_CLASSDC,TempWndProc,0L,0L,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,("1"),NULL};
//    RegisterClassEx(&wc);
//    HWND hWnd = CreateWindowA("1",NULL,WS_OVERLAPPEDWINDOW,100,100,300,300,GetDesktopWindow(),NULL,wc.hInstance,NULL);
    HMODULE hD3D9 = LoadLibrary(L"d3d9");
//    HMODULE hD3D9 = GetModuleHandleA("d3d9");
    DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(hD3D9, "Direct3DCreate9");
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DDISPLAYMODE d3ddm;
    d3d->GetAdapterDisplayMode(0, &d3ddm);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    IDirect3DDevice9* d3dDevice = 0;
    d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
    DWORD* vtablePtr = (DWORD*)(*((DWORD*)d3dDevice));
    qDebug() << (PBYTE)vtablePtr[42];
    dwEndScene = vtablePtr[42] - (DWORD)hD3D9;
    qDebug() << (PBYTE)dwEndScene;
    d3dDevice->Release();
    d3d->Release();
    FreeLibrary(hD3D9);
    CloseHandle(hWnd);
}

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
            memcpy(tbl, (void *)pInterface, size);
            pD3Ddev->Release();
        }
        pD3D->Release();
    }
    DestroyWindow(hWnd);
    return pInterface;
}
void HookDevice9Methods()
{
    qDebug() << "HookDevice9Methods";
    DWORD        vTable[105];
    HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
    pEndScene = (oEndScene)((DWORD)hD3D9 + dwEndScene);
    jmpbES[0] = 0xE9;
    DWORD addr = (DWORD)myEndScene - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addr, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    if (D3Ddiscover((void *)&vTable[0], 420) == 0) return;
    {
        Sleep(100);
    }
    memcpy((PBYTE)pEndScene, jmpbES, 5);
}

bool hooked = false;
HRESULT WINAPI Hook()
{

    if (hooked == false)
    {
        qDebug() << "Hook";
        GetDevice9Methods();
        HookDevice9Methods();
        hooked = true;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
//    new_My_Thread(D3d9_Hook);
    D3d9_Hook();
//    Hook();
    return a.exec();
}
