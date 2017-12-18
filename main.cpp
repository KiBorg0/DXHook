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
//typedef HRESULT	(WINAPI* oEndScene)(LPDIRECT3DDEVICE9 pDevice);
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

LPDIRECT3DDEVICE9 oldDevice = nullptr;
HANDLE hSharedMemory = nullptr;
PGameInfo lpSharedMemory = nullptr;
int fontSize = 14;
int mainFontSize = 14;
int titleFontSize = 14;

bool closeWithGame = false;
QString version_str = "0.0.0";

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

    bool success = false;
    if(AddFontResourceA("Engine/Locale/English/data/font/engo.ttf"))
        success = Render.AddFont("GothicRus", titleFontSize, false, false);
    if(!success)
        Render.AddFont("Arial", titleFontSize, false, false);
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

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();

    memcpy((void *)pReset, CodeFragmentRES, 5);
    hr = pDevice->Reset(pPresentationParameters);
    memcpy((void *)pReset, jmpbRES, 5);

    if(SUCCEEDED(hr)) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr);


    return hr;
}

HRESULT APIENTRY HookedEndScene(LPDIRECT3DDEVICE9 pDevice/*, const RECT* src, const RECT* dest, HWND hWnd, const RGNDATA* unused*/)
{
    HRESULT hRet = D3D_OK;
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
        qDebug() << "HookedEndScene TestCooperativeLevel" << DXGetErrorString9A(hr);
    paint(pDevice);

    memcpy((void *)pEndScene, CodeFragmentES, 5);
    hRet = pDevice->EndScene();
    memcpy((void *)pEndScene, jmpbES, 5);

    if(FAILED(hRet))
        qDebug() << "HookedEndScene return is" << DXGetErrorString9A(hRet);

    return hRet;
}

void HookDevice9Methods(){
    DWORD *VTable;
    DWORD hD3D9 = 0;
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

    if (enableDXHook)
        HookDevice9Methods();

    if(!runWithGame) return 0;

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
        ShExecInfo.fMask = /*SEE_MASK_NOCLOSEPROCESS|*/SEE_MASK_FLAG_NO_UI/*|SEE_MASK_CLASSNAME*/;
//        ShExecInfo.lpClass = L"exefile";
        ShExecInfo.hwnd = NULL;
        ShExecInfo.lpVerb = L"runas";
        ShExecInfo.lpFile = L"SSStats.exe";
        ShExecInfo.lpParameters = NULL;
        ShExecInfo.lpDirectory = NPath;
        ShExecInfo.nShow = SW_HIDE;
        ShExecInfo.hInstApp = NULL;
        qDebug() << QString::fromWCharArray(NPath);

        if(ShellExecuteEx(&ShExecInfo))
            qDebug() << "startDetached - Success!";
        else
            qDebug() << "startDetached - Failed!" << GetLastError();
//        CloseHandle(ShExecInfo.hProcess);
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
    }
    if (dwReason==DLL_PROCESS_DETACH)
    {
        Draw.ReleaseFonts();
        Render.ReleaseFonts();
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
                Draw.Draw_Box(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,DARKGRAY(150));
                Draw.Draw_Border(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,1,SKYBLUE(255));
                Draw.String(center_text_pos-(text_w+8)/2+4,/*height*0.02+*/i*(mainFontSize+14)+4, ORANGE(255), player_info.data(), 0, DT_LEFT|DT_NOCLIP);
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
            Draw.Text(str.data(), center_text_pos-(text_w+12)/2+6, /*height*0.02+*/6, 0, false, ORANGE(255), ORANGE(255));
        }
    }
}

Q_DECL_EXPORT void Inject(void)
{
}
