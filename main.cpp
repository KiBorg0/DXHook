#include <qglobal.h>
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include "Colors.h"
#include <QSettings>
#include <QProcess>
//#include "cRender.h"
#define D3DparamX		, UINT paramx
#define D3DparamvalX	, paramx
#define new_My_Thread(Function) CreateThread(0,0,(LPTHREAD_START_ROUTINE)Function,0,0,0);

//wchar_t *MyCharToWideChar(const char *data);
//void paint(IDirect3DDevice9 *pDevice);

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
//edit if you want to work for the game
//typedef HRESULT (WINAPI* oReset)( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );
//typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef HRESULT	(WINAPI* oEndScene)(IDirect3DDevice9* pDevice, const RECT* src, const RECT* dest, HWND hWnd, void* unused);
oEndScene pEndScene = NULL;
//oPresent9 pPresent9 = NULL;
//oReset pReset = NULL;
ID3DXFont* pFont;
bool hooked = false;
bool Init = false;
DWORD dwEndScene;
DWORD present9;
BYTE CodeFragmentES[5] = { 0, 0, 0, 0, 0 };
BYTE jmpbES[5] = { 0, 0, 0, 0, 0 };
DWORD dwOldProtectES = 0;


typedef struct{
    char players[8][100];
    char mapName[50];
    int AverageAPM;
    int CurrentAPM;
    int MaxAPM;
    int downloadProgress;
    bool initFont;
} TGameInfo;

typedef TGameInfo *PGameInfo;

HANDLE hSharedMemory = nullptr;
PGameInfo lpSharedMemory = nullptr;
int fontSize;
D3DCOLOR fontColor = Colors::WHITE;
std::string font = "Gulim";

void __fastcall String(int x, int y, DWORD Color, DWORD Style, const char *Format, ...)
{
    RECT rect;
    SetRect(&rect, x, y, x, y);
    char Buffer[1024] = { '\0' };
    va_list va_alist;
    va_start(va_alist, Format);
    vsprintf_s(Buffer, Format, va_alist);
    va_end(va_alist);
    // определим размер памяти, необходимый для хранения Unicode-строки
    int length = MultiByteToWideChar(CP_UTF8, 0, Format, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    // конвертируем ANSI-строку в Unicode-строку
    MultiByteToWideChar(CP_UTF8, 0, Format, -1, ptr, length);
    pFont->DrawText(NULL, ptr, -1, &rect, Style, Color);
    return;
}

int GetTextLen(const char *szString)
{
    RECT rect = {0,0,0,0};
    pFont->DrawTextA(NULL, szString, -1, &rect, DT_CALCRECT, 0);
    return rect.right;
}

void Draw_Box(int x, int y, int w, int h,   D3DCOLOR Color,IDirect3DDevice9* mDevice)
{
    D3DRECT rec;
    rec.x1 = x;
    rec.x2 = x + w;
    rec.y1 = y;
    rec.y2 = y + h;
//    mDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
    mDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,	D3DPT_TRIANGLESTRIP);
    mDevice->Clear( 1, &rec, D3DCLEAR_TARGET, Color, 1, 1 );
}

void Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color,IDirect3DDevice9* mDevice)
{
    Draw_Box(x,  y, s,  h,Color,mDevice);
    Draw_Box(x,y+h, w,  s,Color,mDevice);
    Draw_Box(x,  y, w,  s,Color,mDevice);
    Draw_Box(x+w,y, s,h+s,Color,mDevice);
}

bool fileMapping = false;

long __stdcall HookedPresent9(IDirect3DDevice9* pDevice, const RECT* src, const RECT* dest, HWND hWnd, void* unused)
{
//HRESULT APIENTRY myEndScene( LPDIRECT3DDEVICE9 pDevice )
//{
    // восстанавливаем первые 5 байт для того чтобы вызывать оригинальную функцию
    // при рисовании в этой
    BYTE* CDES = (BYTE*)pEndScene;
    CDES[0] = CodeFragmentES[0];
    *((DWORD*)(CDES + 1)) = *((DWORD*)(CodeFragmentES + 1));


    if (!Init)
    {
        // создаем шрифт и получаем указатель на него
        D3DXCreateFont(pDevice, fontSize, 0, FW_BLACK, 0, FALSE, DEFAULT_CHARSET,
                       OUT_TT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       L"Gulim", &pFont);
        Init = true;
    }


    // получаем высоту и ширину экрана
    D3DDEVICE_CREATION_PARAMETERS cparams;
    RECT rect;
    pDevice->GetCreationParameters(&cparams);
    GetWindowRect(cparams.hFocusWindow, &rect);
    DWORD h = abs(rect.bottom - rect.top);
    DWORD w = abs(rect.right - rect.left);

    if (!pFont){
        pFont->OnLostDevice();
    }else{
        String(10, 60, fontColor, DT_LEFT | DT_NOCLIP, "Тест 1");
        if(lpSharedMemory!=0){
            std::string str="";
            String(10, 75, fontColor, DT_LEFT | DT_NOCLIP, "Тест 2");
            if(lpSharedMemory->CurrentAPM>0)
                str += "CurrentAPM: "+std::to_string(lpSharedMemory->CurrentAPM);
            if(lpSharedMemory->AverageAPM>0)
                str += " AverageAPM: "+std::to_string(lpSharedMemory->AverageAPM);
            if(lpSharedMemory->MaxAPM>0)
                str += " MaxAPM: "+std::to_string(lpSharedMemory->MaxAPM);

            for(int i=0; i<8; i++){
                std::string player_info(&lpSharedMemory->players[i][0]);
                if(!player_info.empty()){
                    int text_w = GetTextLen(player_info.data());
                    Draw_Box(w*0.35-5, 12+i*(fontSize+10),text_w+10,fontSize+4,C_BOX,pDevice);
                    Draw_Border(w*0.35-5, 12+i*(fontSize+10),text_w+10,fontSize+4,1,C_BORDER,pDevice);
                    String(w*0.35, 12+i*(fontSize+10)+2, fontColor, DT_LEFT | DT_NOCLIP, player_info.data());
                }
            }
            std::string mapName(&lpSharedMemory->mapName[0]);
            if(!mapName.empty()){
                int p = lpSharedMemory->downloadProgress/10;
                if(p!=0){
                    std::string progress = std::string(p, '#')+std::string(10-p,' ')+' '+std::to_string(lpSharedMemory->downloadProgress);
                    String(w*0.35, 15, fontColor, DT_LEFT | DT_NOCLIP, std::string("Downloading: "+mapName).data());
                    String(w*0.35, 30, fontColor, DT_LEFT | DT_NOCLIP, progress.data());
                }else
                    String(w*0.35, 15, fontColor, DT_LEFT | DT_NOCLIP, "Please re-join");
            }
            if(!str.empty()){
                int text_w = GetTextLen(str.data());
                Draw_Box(w*0.35-5,12,text_w+10,fontSize+6,C_BOX,pDevice);
                Draw_Border(w*0.35-5,12,text_w+10,fontSize+6,1,C_BORDER,pDevice);
                String(w*0.35, 16, fontColor, DT_LEFT | DT_NOCLIP, str.data());
            }
        }

        pFont->OnLostDevice();
        pFont->OnResetDevice();
    }

    // заменяем первые 5 байт обратно для того чтобы при следующем вызове
    // опять вызывалась эта функция
//    DWORD res = pEndScene(pDevice);
    pDevice->EndScene();
    DWORD res = pEndScene(pDevice, src, dest, hWnd, unused);
    CDES[0] = jmpbES[0];
    *((DWORD*)(CDES + 1)) = *((DWORD*)(jmpbES + 1));

    return res;

}

void GetDevice9Methods()
{
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
    present9 = vtablePtr[17] - (DWORD)hD3D9;
//    dwReset = vtablePtr[16] - (DWORD)hD3D9;
    // освобождаем устройство, так как больше оно не нужно
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
    DWORD vTable[105];
    HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
    pEndScene = (oEndScene)((DWORD)hD3D9 + present9);
    jmpbES[0] = 0xE9;
    DWORD addr = (DWORD)HookedPresent9 - (DWORD)pEndScene - 5;
    memcpy(jmpbES + 1, &addr, sizeof(DWORD));
    memcpy(CodeFragmentES, (PBYTE)pEndScene, 5);
    VirtualProtect((PBYTE)pEndScene, 8, PAGE_EXECUTE_READWRITE, &dwOldProtectES);
    memcpy((PBYTE)pEndScene, jmpbES, 5);
    if (D3Ddiscover((void *)&vTable[0], 420) == 0) return;
    {
        Sleep(100);
    }
}

HRESULT WINAPI Hook()
{
    QSettings settings("stats.ini", QSettings::IniFormat);
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TGameInfo), L"DXHook-Shared-Memory");
    if(hSharedMemory != nullptr)
    {
        lpSharedMemory = (PGameInfo)MapViewOfFile(hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        memset(lpSharedMemory, 0, sizeof(TGameInfo));

        fontSize = settings.value("utils/fontSize", 14).toInt();
        bool bStatus = false;
        font = settings.value("utils/font", "Gulim").toString().toStdString();
        uint icolor = settings.value("utils/color", "0xFFFFFF").toString().toUInt(&bStatus,16);
        if(bStatus)
            fontColor = D3DCOLOR_ARGB(255, icolor / 0x10000,
                                  (icolor / 0x100) % 0x100,
                                  icolor % 0x100);

    }
    bool enableDXHook = settings.value("settings/enableDXHook", true).toBool();
    if (enableDXHook&&!hooked)
    {
        GetDevice9Methods();
        HookDevice9Methods();
//        HookPresent9();
//        HookEndScene();
//        HookReset();
        hooked = true;
    }
//    QProcess ssstats;
//    ssstats.startDetached("SSStatsUpdater.exe");
    return 0;
}

BOOL WINAPI DllMain(HMODULE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason==DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);
        new_My_Thread(Hook);
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hSharedMemory);
    }
    return TRUE;
}

Q_DECL_EXPORT void Inject(void)
{
}
