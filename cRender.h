//#pragma once
#ifndef CRENDER_H
#define CRENDER_H

#include <d3d9.h>
#include <d3dx9.h>
#include <DxErr9.h>
#include <stdio.h>
#include <stdlib.h>
#include "Colors.h"
#include "Structure.h"
#include <QVector>

#define C_Text (DT_CENTER|DT_NOCLIP)
#define L_Text (DT_LEFT|DT_NOCLIP)
#define R_Text (DT_RIGHT|DT_NOCLIP)
#define MAX_FONTS 6

enum text_alignment {lefted, centered, righted};

typedef struct{
    char name[100];
    int info[7];
} TPlayer;

typedef struct{
    bool enableDXHook;
    int version;
    char players[8][100];
    int playersNumber;
    TPlayer lPlayers[50];
    char mapName[50];
    int AverageAPM;
    int CurrentAPM;
    int MaxAPM;
    int downloadProgress;
    bool fontsInited;
    bool showMenu;
    bool showRaces;
    bool showAPM;
    DWORD statsThrId;
    PVOID sidsAddr[10];
    bool sidsAddrLock;
    long total_actions;
} TGameInfo;

typedef TGameInfo *PGameInfo;

class cRender
{
public:
	cRender(void);

	bool Init;
	bool Show;
	int w_menu,
        h_menu,
		w_sub_menu;
    int titleFontSize,
        h_screen,
        w_screen;
    DWORD key_comb_dwStartTime;
    ID3DXFont *pFont[MAX_FONTS];
    int playersNumber;
    TPlayer Players[50];
    PGameInfo gameInfo;
    int fontSize;

    struct stMenu
	{
        int x,
            y, _y,
			height_fon,
			height_sub_fon;
    };
    stMenu pos_Menu;

    bool AddFont(const char* Caption, int size, bool bold, bool italic);
    bool Font();
    void OnLostDevice();
    void OnResetDevice();
    void ReleaseFonts();

	enum gr_orientation{horizontal,vertical};
    void Text(const char *text, float x, float y, int font, bool bordered, DWORD color, DWORD bcolor, int orientation);
    void drawBox( int x, int y, int w, int h,   D3DCOLOR Color);
    void drawBorder(int x, int y, int w, int h,int s, D3DCOLOR Color);
    void showMenu();
    void initPosMenu(const char *Titl, int font);
    void setGameInfo(PGameInfo gameInfo);
    void setMenuParams(int fontSize, int width, int height);
    void String(int x, int y, DWORD color, const char *text, int font,  DWORD style);
    int GetTextLen(LPCTSTR szString, int font);
    int GetTextLen(const char *szString, int font);
    int GetTextLenWChar(const char *szString, int font);
    int IsInBox(int x,int y,int w,int h);
    int State_Key(int Key,DWORD dwTimeOut);
    static wchar_t *MyCharToWideChar(const char *data);

    void setDevice(LPDIRECT3DDEVICE9 pDev) {pDevice = pDev;}
private:
    LPDIRECT3DDEVICE9 pDevice;
    LPDIRECT3DVERTEXBUFFER9 g_pVB;    // Buffer to hold vertices
    int FontNr;
};
#endif // CRENDER_H
