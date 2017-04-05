#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include "Colors.h"
#include "Structure.h"

#define C_Text (DT_CENTER|DT_NOCLIP)
#define L_Text (DT_LEFT|DT_NOCLIP)
#define R_Text (DT_RIGHT|DT_NOCLIP)


static struct _Keys
{
	bool        bPressed;
	DWORD       dwStartTime;
}kPressingKeys[256];

class cRender
{
public:
	cRender(void);

	bool Init;
	bool Show;
	int w_menu,
		w_sub_menu;


	ID3DXFont* pFont;


	struct stMenu
	{
		int x,
			y, _y,
			height_fon,
			height_sub_fon;
	};
	stMenu pos_Menu;

	enum gr_orientation{horizontal,vertical};

    void  Draw_Text(int x, int y, DWORD color, LPCSTR text, DWORD ST);
	void  Draw_Box( int x, int y, int w, int h,   D3DCOLOR Color, LPDIRECT3DDEVICE9 m_pD3Ddev);
	void  Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color, LPDIRECT3DDEVICE9 m_pD3Ddev);
	BOOL  IsInBox(int x,int y,int w,int h);
	BOOL  State_Key(int Key,DWORD dwTimeOut);
	void  SHOW_MENU(LPDIRECT3DDEVICE9  pDevice);
    void  Init_PosMenu(int x,int y,DWORD KEY, const char* Titl,stMenu* pos_Menu,IDirect3DDevice9* m_pD3Ddev);
	void  Draw_GradientBox(float x, float y, float width, float height, DWORD startCol, DWORD endCol, gr_orientation orientation ,IDirect3DDevice9* mDevice);
	int   GetTextLen(LPCTSTR szString);
    void  Draw_Menu_But(stMenu *pos_Menu, const char* text,IDirect3DDevice9* pDevice);
    void  Draw_CheckBox(stMenu *pos_Menu,bool &Var, const char *Text,IDirect3DDevice9 *pDevice);


    void  Draw_ColorBox(stMenu *pos_Menu, const char *Text, int &Var,DWORD *Sel_color,int SizeArr, IDirect3DDevice9 * pDevice);
    void  Draw_ScrolBox(stMenu *pos_Menu, const char *Text, int &Var,int Maximal,IDirect3DDevice9 *pDevice);

};

