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

HRESULT STDMETHODCALLTYPE hooks::user_release(LPDIRECT3DDEVICE9 pDevice) {
    qDebug() << "user_release";
    return hooks::original_release(pDevice);
}


HRESULT STDMETHODCALLTYPE hooks::user_present(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) {
//    qDebug() << "user_present";
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
        qDebug() << "Present TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    paint(pDevice);
//    D3DCOLOR color = D3DCOLOR_XRGB(255, 0, 0);
//    D3DRECT position = {100, 100, 200, 200};
//    pDevice->Clear(1, &position, D3DCLEAR_TARGET | D3DCLEAR_TARGET, color, 0, 0);

//    ID3DXFont *pFont = nullptr;

//    HRESULT result = D3DXCreateFont(pDevice, 30, 0, FW_NORMAL, 1, false,
//        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
//        DEFAULT_PITCH | FF_DONTCARE, L"Consolas", &pFont);
//    if (FAILED(result))
//        qDebug() << "Could not create font. Error =" << result;
//    else
//    {
//        RECT rect = { 0 };
//        (void)SetRect(&rect, 0, 0, 300, 100);
//        int height = pFont->DrawText(nullptr, L"Hello, World!", -1, &rect,
//            DT_LEFT | DT_NOCLIP, -1);
//        if (height == 0)
//        {
//            qDebug() << "Could not draw text.";
//        }
//        (void)pFont->Release();
//    }


    return hooks::original_present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT STDMETHODCALLTYPE hooks::user_reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    qDebug() << "user_reset";
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Reset TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
    Draw.OnLostDevice();
    hr = hooks::original_reset(pDevice, pPresentationParameters);

    if(SUCCEEDED(hr)){
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
        Draw.OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);


    return hr;
}

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
    NULL);// DWORD dwThreadId
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

typedef HRESULT (WINAPI* oReset)( LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters );
typedef HRESULT	(WINAPI* oPresent)(LPDIRECT3DDEVICE9 pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
oReset pReset = NULL;
oPresent pPresent = NULL;
DWORD height, width;
BYTE CodeFragmentPR[5] = {0};
BYTE jmpbPR[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectPR = 0;
BYTE CodeFragmentRES[5] = {0};
BYTE jmpbRES[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};
DWORD dwOldProtectRES = 0;
DWORD SSStatsMainThreadID;

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
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr)) qDebug() << "Reset TCL" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);

    // Освобождает все ссылки к ресурсам видеопамяти и удаляет все блоки состояния.
    Render.OnLostDevice();
    Draw.OnLostDevice();
    memcpy((void *)pReset, CodeFragmentRES, 5);
    hr = pDevice->Reset(pPresentationParameters);
    memcpy((void *)pReset, jmpbRES, 5);

    if(SUCCEEDED(hr)) {
        qDebug() << "Reset return is D3D_OK";
        // Этот метод необходимо вызывать после сброса устройства, и перед вызовом любых других методов,
        // если свойство IsUsingEventHandlers установлено на false.
        Render.OnResetDevice();
        Draw.OnResetDevice();
    }
    else
        qDebug() << "Reset return is" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);


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

void HookDevice9Methods(){
    // Perform signature scans inside the 'gameoverlayrenderer.dll' library.
    std::uintptr_t present_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 DB 74 1F") + 2;
    std::uintptr_t reset_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F8 85 FF 78 18") + 2;
//    std::uintptr_t release_addr = FindPattern("gameoverlayrenderer.dll", "FF 15 ? ? ? ? 8B F0  85 F6 75 15") + 2;
//    qDebug() << (void*) present_addr << (void*)reset_addr << (void*)release_addr;
//    qDebug() << (void*)((DWORD*)present_addr)[0] << (void*)((DWORD*)reset_addr)[0] << (void*)((DWORD*)release_addr)[0];
    // Store the original contents of the pointers for later usage.
    hooks::original_present = **reinterpret_cast<decltype(&hooks::original_present)*>(present_addr);
    hooks::original_reset = **reinterpret_cast<decltype(&hooks::original_reset)*>(reset_addr);
//    hooks::original_release = **reinterpret_cast<decltype(&hooks::original_release)*>(release_addr);
    qDebug() << reinterpret_cast<void*>(&hooks::user_present)
             << reinterpret_cast<void*>(&hooks::user_reset);
    qDebug() << (void*)(**reinterpret_cast<void***>(present_addr))
             << (void*)(**reinterpret_cast<void***>(reset_addr));
    // Switch the contents to point to our replacement functions.
    **reinterpret_cast<void***>(present_addr) = reinterpret_cast<void*>(&hooks::user_present);
    **reinterpret_cast<void***>(reset_addr) = reinterpret_cast<void*>(&hooks::user_reset);
//    **reinterpret_cast<void***>(release_addr) = reinterpret_cast<void*>(&hooks::user_release);
    qDebug() << (void*)(**reinterpret_cast<void***>(present_addr)) <<
                (void*)(**reinterpret_cast<void***>(reset_addr));
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
void APIENTRY GetSIDSAddr(PCHAR addr) //не забываем _stdcall, иначе компилятор может подкинуть нам свинью
{
    if(currentAddr>=50) currentAddr = 0;
    lpSharedMemory->sidsAddr[currentAddr] = addr;
    ++currentAddr;
}

LPVOID _injectedGetSIDSAddrRetAddr, _injectedGetMatchResultAddr; //Нужно указать при создании инжекта, чтобы функция знала, куда ей возвращаться
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
        "pushl %esi\n\t");
    asm("call %P0\n\t" : : "i"(GetSIDSAddr));
    asm("popal\n\t"
        "movl %eax,%ecx\n\t"
        "decl %esi\n\t"
        "subl %ebx,%ecx\n\t"
        "pushl $0x00A35E0A\n\t"
        "ret\n"
    );
}
void APIENTRY GetMatchResult(PCHAR addr)
{
    qDebug() << "GetMatchResult" << (PVOID)addr;
}
void APIENTRY _injectedGetMatchResult()
{
    asm(
        "popl %ebp\n\t"
        "pushal\n\t"
        "pushl %eax\n\t");
    asm("call %P0\n\t" : : "i"(GetMatchResult));
    asm("popal\n\t"
        "movl 0x68(%ebx),%edx\n\t"
        "movl (%eax),%eax\n\t"
        "pushl $0x004683DA\n\t"
        "ret\n"
    );
}

DWORD WINAPI Hook(LPVOID Param)
{
    Sleep(10000);
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

    _injectedGetSIDSAddrRetAddr = (LPVOID)(0x00A35E0A); //0x00A35E05 + 5 конец опкода, которые мы перезаписываем
    InjectJump(0x00A35E05, (DWORD)&_injectedGetSIDSAddr);
    _injectedGetMatchResultAddr = (LPVOID)(0x004683DA);
    InjectJump(0x004683D5, (DWORD)&_injectedGetMatchResult);


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

//    int center_text_pos = width*0.5;
//    if(lpSharedMemory->showRaces){
//        int text_w = 0;
//        for(int i=0; i<8; i++){
//            string player_info(&lpSharedMemory->players[i][0]);
//            if(!player_info.empty()){
//                int w = Draw.GetTextLen(player_info.data(), 0);
//                if(w>text_w)
//                    text_w = w;
//            } else break;
//        }
//        for(int i=0; i<8; i++){
//            string player_info(&lpSharedMemory->players[i][0]);
//            if(!player_info.empty()){
//                Draw.Draw_Box(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,DARKGRAY(150));
//                Draw.Draw_Border(center_text_pos-(text_w+8)/2,/*height*0.02+*/i*(mainFontSize+14),text_w+8,mainFontSize+8,1,SKYBLUE(255));
//                Draw.String(center_text_pos-(text_w+8)/2+4,/*height*0.02+*/i*(mainFontSize+14)+4, ORANGE(255), player_info.data(), 0, DT_LEFT|DT_NOCLIP);
//            } else break;
//        }
//    }

//    if(lpSharedMemory->showAPM){
//        string str="";
//        if(lpSharedMemory->CurrentAPM>0)
//            str += "Current: "+to_string(lpSharedMemory->CurrentAPM);
//        if(lpSharedMemory->AverageAPM>0)
//            str += " Average: "+to_string(lpSharedMemory->AverageAPM);
//        if(lpSharedMemory->MaxAPM>0)
//            str += " Max: "+to_string(lpSharedMemory->MaxAPM);
//        if(!str.empty()){
//            int text_w = Draw.GetTextLen(str.data(), 0);
//            Draw.Draw_Box(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,mainFontSize+12,DARKGRAY(150));
//            Draw.Draw_Border(center_text_pos-(text_w+12)/2,0/*height*0.02*/,text_w+12,mainFontSize+12,1,SKYBLUE(255));
//            Draw.Text(str.data(), center_text_pos-(text_w+12)/2+6, /*height*0.02+*/6, 0, false, ORANGE(255), ORANGE(255));
//        }
//    }
}

Q_DECL_EXPORT void Inject(void)
{
//    LPDIRECT3D9 pD3D;
//    if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) != NULL)
//        pD3D->Release();
}
