#include <qglobal.h>
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <QString>
#define D3DparamX		, UINT paramx
#define D3DparamvalX	, paramx
#define new_My_Thread(Function) CreateThread(0,0,(LPTHREAD_START_ROUTINE)Function,0,0,0);

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
//typedef HRESULT ( WINAPI* oReset )( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );//edit if you want to work for the game
typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
oEndScene pEndScene = NULL;
//oReset pReset;
ID3DXFont* pFont;
bool hooked = false;
bool Init = false;
DWORD dwEndScene;
BYTE CodeFragmentES[5] = { 0, 0, 0, 0, 0 };
BYTE jmpbES[5] = { 0, 0, 0, 0, 0 };
DWORD dwOldProtectES = 0;

typedef struct{
    int AverageAPM;
    int CurrentAPM;
    int MaxAPM;
    int downloadProgress;
    char player[7][50];
    char mapName[50];
} TGameInfo;

typedef TGameInfo *PGameInfo;

HANDLE hSharedMemory;
PGameInfo lpSharedMemory;

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

HRESULT APIENTRY myEndScene( LPDIRECT3DDEVICE9 pDevice )
{
    hSharedMemory = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, L"DXHook-Shared-Memory");
    if(hSharedMemory != NULL)
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);


    BYTE* CDES = (BYTE*)pEndScene;
    CDES[0] = CodeFragmentES[0];
    *((DWORD*)(CDES + 1)) = *((DWORD*)(CodeFragmentES + 1));

    if (!Init)
    {
//        Beep(100, 100);
        D3DXCreateFont(pDevice, 13, 0, FW_BLACK, 0, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma", &pFont);
        Init = true;
    }

    DWORD h=1024, w=1280;
    D3DDEVICE_CREATION_PARAMETERS cparams;
    RECT rect;

    pDevice->GetCreationParameters(&cparams);
    GetWindowRect(cparams.hFocusWindow, &rect);
    h = abs(rect.bottom - rect.top);
    w = abs(rect.right - rect.left);
//    if(pDevice!=NULL)
//    {
//        D3DVIEWPORT9* pViewport;
//        pDevice->GetViewport(pViewport);
//        h = pViewport->Height;
//        w = pViewport->Width;
//    }

    if (!pFont)
        pFont->OnLostDevice();
    else
    {
        if(lpSharedMemory!=0){
            QString str;
            if(lpSharedMemory->CurrentAPM!=0){
            str = "CurrentAPM: "+QString::number(lpSharedMemory->CurrentAPM);
            }
            if(lpSharedMemory->AverageAPM!=0){
            str += " AverageAPM: "+QString::number(lpSharedMemory->AverageAPM);
            }
            if(lpSharedMemory->MaxAPM!=0){
            str += " MaxAPM: "+QString::number(lpSharedMemory->MaxAPM);
            }
            QString player_info;
            for(int i=0; i<7; i++)
            {
                player_info = QString::fromLatin1(&lpSharedMemory->player[i][0]);
                if(!player_info.isEmpty())
                    String(w*0.5, 10*(i+2), D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, player_info.toStdString().data());
            }
            QString mapName = QString::fromLatin1(&lpSharedMemory->mapName[0]);
            if(!mapName.isEmpty())
            {
                int p = lpSharedMemory->downloadProgress/10;
                if(p!=0)
                {
                    String(w*0.5, 20, D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, QString("Downloading: "+mapName).toStdString().data());
                    QString progress = QString("#").repeated(p)+QString(" ").repeated(10-p)+" "+QString::number(lpSharedMemory->downloadProgress)+"%";
                    String(w*0.5, 30, D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, progress.toStdString().data());
                }
                else{
                    String(w*0.5, 20, D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, QString("Please re-join").toStdString().data());
                }
            }
            if(!str.isEmpty())
                String(w*0.5, 10, D3DCOLOR_ARGB(255, 0, 255, 0), DT_LEFT | DT_NOCLIP, str.toStdString().data());
        }

        pFont->OnLostDevice();
        pFont->OnResetDevice();
    }

    DWORD res = pEndScene(pDevice);
    CDES[0] = jmpbES[0];
    *((DWORD*)(CDES + 1)) = *((DWORD*)(jmpbES + 1));

    return res;

}

void GetDevice9Methods()
{
    HWND hWnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    HMODULE hD3D9 = LoadLibrary(L"d3d9");
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
    dwEndScene = vtablePtr[42] - (DWORD)hD3D9;
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

HRESULT WINAPI Hook()
{
    if (hooked == false)
    {
        GetDevice9Methods();
        HookDevice9Methods();
        hooked = true;
    }
    return 0;
}

BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason==DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);
        new_My_Thread(Hook);
    }
    return TRUE;
}

Q_DECL_EXPORT void Inject(void)
{
}
