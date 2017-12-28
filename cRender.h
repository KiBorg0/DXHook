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

typedef struct{
    char name[100];
    int race;
    int gamesCount;
    int winsCount;
    int winRate;
    int mmr;
    int apm;
} TPlayer;

typedef struct{
    bool enableDXHook;
    int version;
    char players[8][100];
    int playersNumber;
    TPlayer lobbyPlayers[50];
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
    PCHAR sidsAddr[50];
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
//	ID3DXFont* pFont;
    ID3DXFont *pFont[MAX_FONTS];
    PGameInfo lpSharedMemory;
    struct stMenu
	{
        int x,
            y, _y,
			height_fon,
			height_sub_fon;
    };
    stMenu pos_Menu;

    bool AddFont(const char* Caption, float size, bool bold, bool italic);
    bool Font();
    void OnLostDevice();
    void OnResetDevice();
    void ReleaseFonts();

	enum gr_orientation{horizontal,vertical};
//    void setFont(const char *font, int size);
//    void  Draw_Text(int x, int y, DWORD color, const char *text, DWORD ST);
    void  Text(const char *text, float x, float y, int font, bool bordered, DWORD color, DWORD bcolor, int orientation);
    void  Draw_Box( int x, int y, int w, int h,   D3DCOLOR Color);
    void  Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color);
	BOOL  IsInBox(int x,int y,int w,int h);
    BOOL  State_Key(int Key,DWORD dwTimeOut);
//    BOOL  State_Key_Combination(QVector<short> key_comb, DWORD dwTimeOut);
    void  SHOW_MENU();
    void  Init_PosMenu(const char *Titl);
    void  setGameInfo(PGameInfo gameInfo);
    void  setMenuParams(int fontSize, int width, int height);
    void  Draw_GradientBox(float x, float y, float width, float height, DWORD startCol, DWORD endCol, gr_orientation orientation );
    int GetTextLen(LPCTSTR szString, int font);
    int GetTextLen(const char *szString, int font);
    void  Draw_Menu_But(stMenu *pos_Menu, const char *text);
    void  Draw_CheckBox(stMenu *pos_Menu,bool &Var,char *Text);

    void String(int x, int y, DWORD color, const char *text, int font,  DWORD style);
//    void String(int x, int y, DWORD Color, const char *string, int font, DWORD Style);
    void  Draw_ColorBox(stMenu *pos_Menu, char *Text, int &Var, DWORD *Sel_color, int SizeArr);
    void  Draw_ScrolBox(stMenu *pos_Menu, char *Text, int &Var,int Maximal);

    static wchar_t *MyCharToWideChar(const char *data);

    void setDevice(LPDIRECT3DDEVICE9 pDev) {pDevice = pDev;}
//    void Reset();
private:
    LPDIRECT3DDEVICE9 pDevice;
    LPDIRECT3DVERTEXBUFFER9 g_pVB;    // Buffer to hold vertices
    int FontNr;
};
#endif // CRENDER_H


#ifndef _DRAW_H_
#define _DRAW_H_

enum circle_type {full, half, quarter};
enum text_alignment {lefted, centered, righted};

#define MAX_FONTS 6

struct vertex
{
    FLOAT x, y, z, rhw;
    DWORD color;
};

class CDraw
{
public:
   CDraw()
   {
      g_pVB = NULL;
      g_pIB = NULL;
      FontNr = 0;
   }

   struct sScreen
   {
      float Width;
      float Height;
      float x_center;
      float y_center;
   } Screen;

   ID3DXFont *pFont[MAX_FONTS];

   int GetTextLen(LPCTSTR szString, int font);
   int GetTextLen(const char *szString, int font);

   void Sprite(LPDIRECT3DTEXTURE9 tex, float x, float y, float resolution, float scale, float rotation);

   //=============================================================================================
   void Line(float x1, float y1, float x2, float y2, float width, bool antialias, DWORD color);
   void String(int x, int y, DWORD color, const char *text, int font,  DWORD style);
   void StringChar(int x, int y, DWORD color, const char *text, int font,  DWORD style);
   void String(int x, int y, DWORD color, LPCTSTR text, int font,  DWORD style);
//   void DrawTextBox(int x, int y, int w, int h,   D3DCOLOR Color);
   void Draw_Box(int x, int y, int w, int h,   D3DCOLOR Color);
   void Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color);
   void Box(float x, float y, float w, float h, float linewidth, DWORD color);
   void BoxFilled(float x, float y, float w, float h, DWORD color);
   void BoxBordered(float x, float y, float w, float h, float border_width, DWORD color, DWORD color_border);
   void BoxRounded(float x, float y, float w, float h,float radius, bool smoothing, DWORD color, DWORD bcolor);

   void Circle(float x, float y, float radius, int rotate, int type, bool smoothing, int resolution, DWORD color);
   void CircleFilled(float x, float y, float rad, float rotate, int type, int resolution, DWORD color);

   void Text(const char *text, float x, float y, int font, bool bordered, DWORD color, DWORD bcolor, int orientation=0);
   void Message(const char *text, LONG x, LONG y, int font, int orientation=0);
   //=============================================================================================

   //=============================================================================================
   bool Font();
   HRESULT AddFont(const char *Caption, float size, bool bold, bool italic);
   void FontReset();
   void OnLostDevice();
   void OnResetDevice();
   void ReleaseFonts();
   //=============================================================================================

   void SetDevice(LPDIRECT3DDEVICE9 pDev) {pDevice = pDev;}
   void Reset();
private:
   LPDIRECT3DDEVICE9 pDevice;
   LPDIRECT3DVERTEXBUFFER9 g_pVB;    // Buffer to hold vertices
   LPDIRECT3DINDEXBUFFER9  g_pIB;    // Buffer to hold indices

   int FontNr;
   LPD3DXSPRITE sSprite;
};

#endif /* _DRAW_H_ */
