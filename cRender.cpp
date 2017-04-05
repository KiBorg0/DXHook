#include "cRender.h"


cRender::cRender(void)
{
	Show = true;   //показ меню после старта   true - показывать, false - не показывать
	w_menu = 160;  //ширина основного меню
	w_sub_menu = 200;
}
void  cRender::Draw_Text(int x,int y,DWORD color,LPCSTR text,DWORD ST)
{
	RECT rect, rect2;
	SetRect( &rect, x, y, x, y );
	SetRect( &rect2, x - 0.1, y + 0.2, x - 0.1, y + 0.1 );
	pFont->DrawTextA(NULL,text,-1,&rect2, ST, 0xff000000 );
	pFont->DrawTextA(NULL,text,-1,&rect,  ST, color );
}
void  cRender::Draw_Box(int x, int y, int w, int h,   D3DCOLOR Color,IDirect3DDevice9* mDevice)
{
	D3DRECT rec;
	rec.x1 = x;
	rec.x2 = x + w;
	rec.y1 = y;
	rec.y2 = y + h;
	mDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1); 
	mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
	mDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);   
	mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,	D3DPT_TRIANGLESTRIP);
	mDevice->Clear( 1, &rec, D3DCLEAR_TARGET, Color, 1, 1 );
}
void  cRender::Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color,IDirect3DDevice9* mDevice)
{
	Draw_Box(x,  y, s,  h,Color,mDevice);
	Draw_Box(x,y+h, w,  s,Color,mDevice);
	Draw_Box(x,  y, w,  s,Color,mDevice);
	Draw_Box(x+w,y, s,h+s,Color,mDevice);
}
BOOL  cRender::IsInBox(int x,int y,int w,int h)
{
	POINT MousePosition; 
	GetCursorPos(&MousePosition); 
	ScreenToClient(GetForegroundWindow(),&MousePosition);
	return(MousePosition.x >= x && MousePosition.x <= x + w && MousePosition.y >= y && MousePosition.y <= y + h);
}
int   cRender::GetTextLen(LPCTSTR szString)
{
	RECT rect = {0,0,0,0};
	pFont->DrawText(NULL, szString, -1, &rect, DT_CALCRECT, 0);
	return rect.right;
} 
BOOL  cRender::State_Key(int Key,DWORD dwTimeOut)
{
	if(HIWORD(GetKeyState(Key)))
	{
		if(!kPressingKeys[Key].bPressed || (kPressingKeys[Key].dwStartTime && (kPressingKeys[Key].dwStartTime + dwTimeOut) <= GetTickCount()))
		{
			kPressingKeys[Key].bPressed = TRUE;
			if( dwTimeOut > NULL )
				kPressingKeys[Key].dwStartTime = GetTickCount();
			return TRUE;
		}
	}
	else
		kPressingKeys[Key].bPressed = FALSE;
	return FALSE;
}
void  cRender::Init_PosMenu(int x,int y,DWORD KEY, const char* Titl,stMenu* pos_Menu,IDirect3DDevice9* m_pD3Ddev)
{
	(*pos_Menu).x = x;
	(*pos_Menu).y = y+30;
	(*pos_Menu)._y = y+30;


	

	if( !pFont)
		pFont->OnLostDevice();
	else
	{

		if(State_Key(KEY,3000))Show=!Show;
		if(Show)
			{
				Draw_Box(x,y,w_menu,(*pos_Menu).height_fon,C_BOX,m_pD3Ddev);
				Draw_Border(x,y,w_menu,(*pos_Menu).height_fon,1,C_BORDER,m_pD3Ddev);
				Draw_Border(x,y,w_menu,28,1,C_BORDER,m_pD3Ddev);

				Draw_Text(x+w_menu/2,y+6,C_TITL,Titl,C_Text);
				SHOW_MENU(m_pD3Ddev);

		    }

		pFont->OnLostDevice();
		pFont->OnResetDevice();
	}
}
void  cRender::Draw_GradientBox(float x, float y, float width, float height, DWORD startCol, DWORD endCol, gr_orientation orientation ,IDirect3DDevice9* mDevice )
{
	static struct D3DVERTEX
	{
		float x, 
			y, 
			z, 
			rhw;
		DWORD color;
	} 
	vertices[4] = {
		{0,0,0,1.0f,0},
		{0,0,0,1.0f,0},
		{0,0,0,1.0f,0},
		{0,0,0,1.0f,0}};


		vertices[0].x = x;
		vertices[0].y = y;
		vertices[0].color = startCol;

		vertices[1].x = x+width;
		vertices[1].y = y;
		vertices[1].color = orientation == horizontal ? endCol : startCol;

		vertices[2].x = x;
		vertices[2].y = y+height;
		vertices[2].color = orientation == horizontal ? startCol : endCol;

		vertices[3].x = x+width;
		vertices[3].y = y+height;
		vertices[3].color = endCol;


		mDevice->SetTexture(0, NULL);
		mDevice->SetPixelShader( 0 );
		mDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
		mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
		mDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		mDevice->SetRenderState(D3DRS_ZENABLE , FALSE);
		mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		mDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(D3DVERTEX));

}

int Button_Mass[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	Button_Number = 0,
	Button_Max = 0;

void  cRender::Draw_Menu_But(stMenu *pos_Menu,const char* text,IDirect3DDevice9* pDevice)
{
	int x = (*pos_Menu).x+2,
		y = (*pos_Menu).y,
		h = 22,
		w = w_menu-4;
	D3DCOLOR Bord_text = C_BUT_BORDER;
	D3DCOLOR text_Activ = C_BUT_text_OFF;


	if(IsInBox(x,y,w,h))
	{	
		Bord_text = text_Activ =C_BUT_text_In;
		if (State_Key(VK_LBUTTON,30) )
			if (Button_Mass[Button_Number]!= 1)
				Button_Mass[Button_Number]= 1;
		
	}

	Draw_GradientBox(x, y, w, h, C_BUT_St, C_BUT_End,vertical , pDevice );
	Draw_Border(x, y, w, h,1,Bord_text,pDevice);
	if (Button_Mass[Button_Number])
	{		
		Draw_Box((*pos_Menu).x+w_menu+8, (*pos_Menu)._y, w_sub_menu,(*pos_Menu).height_sub_fon, C_sub_BOX,pDevice);
		Draw_Border((*pos_Menu).x+w_menu+8, (*pos_Menu)._y, w_sub_menu,(*pos_Menu).height_sub_fon,1, C_sub_BORDER,pDevice);
		text_Activ = C_BUT_text_On;
		for ( int i = 0; i < 20; i++ )
			if ( i != Button_Number )
				Button_Mass[i] = 0;
	}
	Draw_Text(x+(w/2),y+3,text_Activ, text,C_Text);

	Button_Number = Button_Number + 1;

	if (Button_Max < Button_Number )
		Button_Max = Button_Number;

	(*pos_Menu).y = y+24;
	(*pos_Menu).height_fon =(*pos_Menu).y-20;
}
void  cRender::Draw_CheckBox(stMenu *pos_Menu, bool &Var, const char *Text,IDirect3DDevice9 *pDevice)
{
	int x = (*pos_Menu).x+w_menu+13,
		y = (*pos_Menu)._y+3,
		w = 16,
		h = 16;


	Draw_Box( x,  y, w, h, C_Fon_Ctrl,pDevice);
	Draw_Border( x,  y, w, h,1, C_Bord_Ctrl,pDevice);


	if(IsInBox(x,y ,w,h))
	{ 
		if (State_Key(VK_LBUTTON,300) ) 
			           Var=!Var;
	}
	if(Var)
		Draw_GradientBox(x+3, y+3, 11, 11, C_Check_St, C_Check_End,vertical , pDevice );
	

	Draw_Text(x+w_sub_menu-10,y+2,C_Text_Ctrl, Text,R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}
void  cRender::Draw_ColorBox(stMenu *pos_Menu, const char *Text, int &Var,DWORD *Sel_color,int SizeArr, IDirect3DDevice9 * pDevice)
{
	int x = (*pos_Menu).x+w_menu+13,
		y = (*pos_Menu)._y+3,
		w = 25,
		h = 16;

	if(IsInBox(x,y ,w,h))
	{ 
		if (State_Key(VK_LBUTTON,300) ) 
			if(Var>=0 && Var<SizeArr)
			           Var++;


		if (State_Key(VK_RBUTTON,300) )
			if(Var!=0)
				       Var--;
	}

	Draw_Box( x,  y, w, h, Sel_color[Var],pDevice);
	Draw_Border( x,  y, w, h,1,C_Bord_Ctrl ,pDevice);
    Draw_Text(x+w_sub_menu-10,y+2,C_Text_Ctrl, Text,R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}
void  cRender::Draw_ScrolBox(stMenu *pos_Menu, const char *Text, int &Var,int Maximal,IDirect3DDevice9 *pDevice)
{
	int x = (*pos_Menu).x+w_menu+13,
		y = (*pos_Menu)._y+3,
		w = 16,
		h = 16;
	
	D3DCOLOR inActiv = C_Text_Ctrl;
	char c_Value[MAX_PATH];

	if(IsInBox(x+7,  y+2, 20, 16))
	{	
		inActiv = C_Text_Ctrl_in;

		if(State_Key(VK_LBUTTON,100))
			if(Var>=0 && Var<Maximal)Var++;

		if(State_Key(VK_RBUTTON,100))
			if(Var!=0)Var--;
	}
	
	sprintf(c_Value, "[ %d ]", Var);
    int lenText = GetTextLen((LPCTSTR) c_Value);
	Draw_Text(x+7,y+3,inActiv,c_Value,L_Text);

	Draw_Text(x+w_sub_menu-10 ,y+3,C_Text_Ctrl,Text,R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}






