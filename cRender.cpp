#include "cRender.h"
#include <vector>
#include <string>
#include <QStringList>
#include <QDebug>

using namespace std;

int Button_Mass[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    Button_Number = 0,
    Button_Max = 0;

static struct _Keys
{
    bool        bPressed;
    DWORD       dwStartTime;
}kPressingKeys[256];

QStringList racesUC = QStringList() << "Random" << "Space Marines"
    <<"Chaos" <<"Orks" <<"Eldar" <<"Imperial Guard" <<"Necrons"
    <<"Tau Empire" <<"Sisters of Battle" <<"Dark Eldar";

cRender::cRender(void)
{
	Show = true;   //показ меню после старта   true - показывать, false - не показывать
    w_menu = 200;  //ширина основного меню
	w_sub_menu = 200;
    g_pVB = nullptr;
    FontNr = 0;
    for(int i = 0; i < MAX_FONTS; i++)
        pFont[i] = nullptr;
}

//void cRender::setFont(const char *font, int size)
//{
//    // создаем шрифт и получаем указатель на него
//    D3DXCreateFontA(pDevice, size, 0, FW_BLACK, 0, FALSE, DEFAULT_CHARSET,
//                   OUT_TT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
//                   font, &pFont);
//}

//void  cRender::Draw_Text(int x, int y, DWORD color, const char *text, DWORD ST)
//{
//	RECT rect, rect2;
//	SetRect( &rect, x, y, x, y );
//	SetRect( &rect2, x - 0.1, y + 0.2, x - 0.1, y + 0.1 );
//	pFont->DrawTextA(NULL,text,-1,&rect2, ST, 0xff000000 );
//	pFont->DrawTextA(NULL,text,-1,&rect,  ST, color );
//}

//void cRender::String(int x, int y, DWORD Color, const char *Format,  DWORD Style, ...)
//{
//    RECT rect;
//    SetRect(&rect, x, y, x, y);
//    char Buffer[1024] = { '\0' };
//    va_list va_alist;
//    va_start(va_alist, Format);
//    vsprintf_s(Buffer, Format, va_alist);
//    va_end(va_alist);
//    // определим размер памяти, необходимый для хранения Unicode-строки
//    int length = MultiByteToWideChar(CP_UTF8, 0, Format, -1, NULL, 0);
//    wchar_t *ptr = new wchar_t[length];
//    // конвертируем ANSI-строку в Unicode-строку
//    MultiByteToWideChar(CP_UTF8, 0, Format, -1, ptr, length);
//    pFont->DrawText(NULL, ptr, -1, &rect, Style, Color);
//    return;
//}

void cRender::String(int x, int y, DWORD color, const char *text, int font,  DWORD style)
{
    if(pFont[font]==nullptr) return;
    HRESULT hr = pDevice->TestCooperativeLevel();
    if (FAILED(hr))
    {
        qDebug() << "HookedEndScene TestCooperativeLevel" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    }
    RECT rect;
    SetRect(&rect, x, y, x, y);
    // определим размер памяти, необходимый для хранения Unicode-строки
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    // конвертируем ANSI-строку в Unicode-строку
    MultiByteToWideChar(CP_UTF8, 0, text, -1, ptr, length);
    pFont[font]->DrawText(NULL, ptr, -1, &rect, style, color);
    return;
}

wchar_t *cRender::MyCharToWideChar(const char *data)
{
    int length = MultiByteToWideChar(CP_UTF8, 0, data, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, ptr, length);
    return ptr;
}

//void cRender::String(int x, int y, DWORD Color, const char *string, int font, DWORD Style)
//{
//    if(font<MAX_FONTS&&pFont[font]){
//        RECT rect;
//        SetRect(&rect, x, y, x, y);
//        pFont[font]->DrawTextA(NULL, string, -1, &rect, Style, Color);
//    }
//    return;
//}

void  cRender::Draw_Box(int x, int y, int w, int h,   D3DCOLOR Color)
{
	D3DRECT rec;
	rec.x1 = x;
	rec.x2 = x + w;
	rec.y1 = y;
	rec.y2 = y + h;
//    DWORD FVF;
//    pDevice->GetFVF(&FVF);
//    pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
    pDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,	D3DPT_TRIANGLESTRIP);
    pDevice->Clear( 1, &rec, D3DCLEAR_TARGET, Color, 1, 1 );
//    pDevice->SetFVF(FVF);

}
void  cRender::Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color)
{
    Draw_Box(x,  y, s,  h,Color);
    Draw_Box(x,y+h, w,  s,Color);
    Draw_Box(x,  y, w,  s,Color);
    Draw_Box(x+w,y, s,h+s,Color);
}
BOOL  cRender::IsInBox(int x,int y,int w,int h)
{
	POINT MousePosition; 
	GetCursorPos(&MousePosition); 
	ScreenToClient(GetForegroundWindow(),&MousePosition);
	return(MousePosition.x >= x && MousePosition.x <= x + w && MousePosition.y >= y && MousePosition.y <= y + h);
}
int cRender::GetTextLen(LPCTSTR szString, int font)
{
    if(pFont[font]==nullptr) return 0;
	RECT rect = {0,0,0,0};
    pFont[font]->DrawText(NULL, szString, -1, &rect, DT_CALCRECT, 0);
	return rect.right;
} 

int cRender::GetTextLen(const char *szString, int font)
{
    if(pFont[font]==nullptr) return 0;
    RECT rect = {0,0,0,0};
    pFont[font]->DrawTextA(NULL, szString, -1, &rect, DT_CALCRECT, 0);
    return rect.right;
}


BOOL  cRender::State_Key(int Key, DWORD dwTimeOut)
{
    if(HIWORD(GetKeyState(Key)))
    {
        if(!kPressingKeys[Key].bPressed || (kPressingKeys[Key].dwStartTime && (kPressingKeys[Key].dwStartTime + dwTimeOut) <= GetTickCount()))
        {
            kPressingKeys[Key].bPressed = TRUE;
            if( dwTimeOut > 0 )
                kPressingKeys[Key].dwStartTime = GetTickCount();
            return TRUE;
        }
    }
    else
        kPressingKeys[Key].bPressed = FALSE;
    return FALSE;
}

//BOOL cRender::State_Key_Combination(QVector<short> key_comb, DWORD dwTimeOut)
//{
//    bool pressed = TRUE;
//    foreach(int key, key_comb){
//        if(HIWORD(GetKeyState(key)))
//        {
//            if(!kPressingKeys[key].bPressed)
//            {
//                kPressingKeys[key].bPressed = TRUE;
//            }
//        }
//        else{
//            kPressingKeys[key].bPressed = FALSE;
//            pressed = FALSE;
//        }
//    }
//    if(pressed&&(key_comb_dwStartTime && (key_comb_dwStartTime + dwTimeOut) >= GetTickCount())){
//        if( dwTimeOut > NULL )
//            key_comb_dwStartTime = GetTickCount();
//        return TRUE;
//    }

//    return FALSE;
//}
void cRender::Text(const char *text, float x, float y, int font, bool bordered, DWORD color, DWORD bcolor, int orientation)
{
   RECT rect;
   if(pFont[font]==nullptr) return;
   switch(orientation)
   {
   case lefted:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, color);
      break;
   case centered:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, color);
      break;
   case righted:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, color);
      break;
   }
}
void cRender::SHOW_MENU()
{
    for(int i=0; i<lpSharedMemory->playersNumber; ++i)
    {
        Draw_Menu_But(&pos_Menu, lpSharedMemory->lobbyPlayers[i].name);
//        if(Button_Mass[0]) { }
    }
}
void  cRender::Init_PosMenu(const char* Titl)
{
    int title_len = GetTextLen(Titl, 0);
    int menu_margin = 0.8*titleFontSize*((double)h_screen/(double)w_screen);
    h_menu = titleFontSize+menu_margin*2;
    w_menu = title_len+menu_margin*2+h_menu;
    int x = w_screen - w_menu-1;

    int y = 0;
    pos_Menu.x = x;
    pos_Menu.y = y+h_menu+2;
    pos_Menu._y = y+h_menu+2;
    Button_Number=0;

    Draw_Box(x,y,w_menu,h_menu,DARKGRAY(150));
    Draw_Border(x,y,w_menu,h_menu,1,SKYBLUE(255));
    Draw_Border(x+w_menu-h_menu,y,h_menu,h_menu,1,SKYBLUE(255));

    D3DCOLOR color = ORANGE(255);

    if(IsInBox(x,y,w_menu,h_menu)){
        if (State_Key(VK_LBUTTON,200) ){
            if (!Button_Mass[Button_Number])
                Button_Mass[Button_Number] = 1;

            else
                for ( int i = 0; i <= lpSharedMemory->playersNumber; i++ )
                    Button_Mass[i] = 0;
        }
        color = C_BUT_text_In;
    }
    else if(!IsInBox(x,y,w_menu, pos_Menu.height_fon+20)){
            if (State_Key(VK_LBUTTON,200) ){
                for ( int i = 0; i <= lpSharedMemory->playersNumber; i++ )
                    Button_Mass[i] = 0;
            }
        }

    Text(Titl, x+(w_menu-h_menu)/2, menu_margin, 0, 0, color, BLACK, 1);

    const char *arrow = Button_Mass[Button_Number]?"▼":"▲";
    String(x+w_menu-h_menu/2+2, menu_margin, color, arrow, 0, C_Text);

    if (Button_Mass[Button_Number])
    {
        Button_Number = 1;
        SHOW_MENU();
    }
}

void cRender::setGameInfo(PGameInfo gameInfo)
{
    lpSharedMemory = gameInfo;
}

void cRender::setMenuParams(int fontSize, int width, int height)
{
    titleFontSize = fontSize;
    h_screen = height;
    w_screen = width;
}
void  cRender::Draw_GradientBox(float x, float y, float width, float height, DWORD startCol, DWORD endCol, gr_orientation orientation)
{
    std::vector<vertex> vertices(4);
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0;
    vertices[0].rhw = 1;
    vertices[0].color = startCol;
    vertices[1].x = x+width;
    vertices[1].y = y;
    vertices[1].z = 0;
    vertices[1].rhw = 1;
    vertices[1].color = orientation == horizontal ? endCol : startCol;
    vertices[2].x = x;
    vertices[2].y = y+height;
    vertices[2].z = 0;
    vertices[2].rhw = 1;
    vertices[2].color = orientation == horizontal ? startCol : endCol;
    vertices[3].x = x+width;
    vertices[3].y = y+height;
    vertices[3].z = 0;
    vertices[3].rhw = 1;
    vertices[3].color = endCol;

    pDevice->CreateVertexBuffer(4 * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW|D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL );

    VOID* pVertices;
    g_pVB->Lock(0, 4 * sizeof(vertex), (void**)&pVertices, 0);
    memcpy(pVertices, &vertices[0], 4 * sizeof(vertex));
    g_pVB->Unlock();

//    DWORD FVF;
//    pDevice->GetFVF(&FVF);
    pDevice->SetTexture(0, NULL);
    pDevice->SetPixelShader(NULL);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,	D3DPT_TRIANGLESTRIP);
    pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
    pDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
//    pDevice->SetRenderState(D3DRS_ZENABLE , FALSE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
//    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(D3DVERTEX));
    pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//    pDevice->SetFVF(FVF);
    if(g_pVB != NULL) g_pVB->Release();
    g_pVB = NULL;
}
void  cRender::Draw_Menu_But(stMenu *pos_Menu, const char* text)
{
    int x = (*pos_Menu).x,
		y = (*pos_Menu).y,
		h = 22,
        w = w_menu/*-4*/;
    D3DCOLOR Bord_text = SKYBLUE(255);
    D3DCOLOR text_Activ = ORANGE(255);

	if(IsInBox(x,y,w,h))
	{	
        Bord_text = text_Activ = C_BUT_text_In;
        if (State_Key(VK_LBUTTON,200) ){
            if (!Button_Mass[Button_Number])
				Button_Mass[Button_Number]= 1;
            else
                Button_Mass[Button_Number] = 0;
        }
    }

//    Draw_GradientBox(x, y, w, h, C_BUT_St, C_BUT_End,vertical);
    Draw_Box(x, y, w,h, DARKGRAY(150));
    Draw_Border(x, y, w, h,1,Bord_text);
    // был выполнен клик по игроку в списке
    if (Button_Mass[Button_Number])
    {
        int x_sub = (*pos_Menu).x-w_sub_menu-2;
        int size = 18, next_pos;
        Draw_Box(x_sub, (*pos_Menu)._y, w_sub_menu,size*7+3, DARKGRAY(150));
        Draw_Border(x_sub, (*pos_Menu)._y, w_sub_menu,size*7+3, 1, SKYBLUE(255));
        next_pos = 3;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("Favorite Race ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, racesUC.at(lpSharedMemory->lobbyPlayers[Button_Number-1].race).toStdString().data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("Games Played ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].gamesCount).data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("Games Won ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].winsCount).data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("(%)Win Ratio ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].winRate).data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("APM ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].apm).data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("MMR ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].mmr).data(), 1, L_Text);
        next_pos += size;
        String(x_sub+(w/2),(*pos_Menu)._y+next_pos,C_BUT_text_In, string("MMR 1VS1 ").data(), 1, R_Text);
        String(x_sub+(w/2)+4,(*pos_Menu)._y+next_pos,C_BUT_text_In, to_string(lpSharedMemory->lobbyPlayers[Button_Number-1].mmr1v1).data(), 1, L_Text);

        // если эта кнопка нажата, то изменим цвет текста на ней на цвет текста активной кнопки
        // и укажем что все остальные кнопки не нажаты
        text_Activ = C_BUT_text_On;
        for ( int i = 1; i <= lpSharedMemory->playersNumber; i++ )
            if ( i != Button_Number )
                Button_Mass[i] = 0;
    }

    String(x+(w/2),y+3,text_Activ, text, 1, C_Text);

	Button_Number = Button_Number + 1;

	if (Button_Max < Button_Number )
		Button_Max = Button_Number;

	(*pos_Menu).y = y+24;
	(*pos_Menu).height_fon =(*pos_Menu).y-20;
}
void  cRender::Draw_CheckBox(stMenu *pos_Menu, bool &Var, char *Text)
{
	int x = (*pos_Menu).x+w_menu+13,
		y = (*pos_Menu)._y+3,
		w = 16,
		h = 16;


    Draw_Box( x,  y, w, h, C_Fon_Ctrl);
    Draw_Border( x,  y, w, h,1, C_Bord_Ctrl);


	if(IsInBox(x,y ,w,h))
	{ 
		if (State_Key(VK_LBUTTON,300) ) 
			           Var=!Var;
	}
	if(Var)
        Draw_GradientBox(x+3, y+3, 11, 11, C_Check_St, C_Check_End,vertical);
	

    String(x+w_sub_menu-10,y+2,C_Text_Ctrl, Text, 1, R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}
void  cRender::Draw_ColorBox(stMenu *pos_Menu, char *Text, int &Var,DWORD *Sel_color,int SizeArr)
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

    Draw_Box( x,  y, w, h, Sel_color[Var]);
    Draw_Border( x,  y, w, h,1,C_Bord_Ctrl);
    String(x+w_sub_menu-10,y+2,C_Text_Ctrl, Text, 1, R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}
void  cRender::Draw_ScrolBox(stMenu *pos_Menu, char *Text, int &Var, int Maximal)
{
	int x = (*pos_Menu).x+w_menu+13,
        y = (*pos_Menu)._y+3;
//		w = 16,
//		h = 16;
	
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
//    int lenText = GetTextLen((LPCTSTR)c_Value);
    String(x+7,y+3,inActiv,c_Value, 1, L_Text);

    String(x+w_sub_menu-10 ,y+3,C_Text_Ctrl,Text, 1, R_Text);

	(*pos_Menu)._y = y+20;
	(*pos_Menu).height_sub_fon =(*pos_Menu)._y-46;
}

bool cRender::AddFont(const char* Caption, float size, bool bold, bool italic)
{
    qDebug() << "Render AddFont" << Caption << size << FontNr;
    HRESULT hr = D3DXCreateFontA(pDevice, size, 0, (bold) ? FW_BOLD : FW_NORMAL, 1, (italic) ? 1 : 0 , DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, Caption, &pFont[FontNr]);

    if(SUCCEEDED(hr))
    {
        ++FontNr;
        return true;
    }
    else
        qDebug() << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
    return false;
}

void cRender::OnResetDevice()
{
   for(int i = 0; i < FontNr; i++)
       if(pFont[i]!=nullptr){
           HRESULT hr = pFont[i]->OnResetDevice();
//           qDebug() << "Render OnResetDevice()" << i << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
       }
}
bool cRender::Font()
{
   for(int i=0; i<FontNr; i++)
      if(pFont[i]==nullptr) return false;
   return true;
}

void cRender::OnLostDevice()
{
    for(int i = 0; i < FontNr; i++)
        if(pFont[i]!=nullptr){
            HRESULT hr = pFont[i]->OnLostDevice();
//            qDebug() << "Render OnLostDevice()" << i << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
        }
}

void cRender::ReleaseFonts()
{
    for(int i = 0; i < FontNr; i++)
        if(pFont[i]!=nullptr){
            HRESULT hr = pFont[i]->Release();

//            if(FAILED(hr))
                qDebug() << "Render ReleaseFonts()"<< i <<  DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr) << (PVOID)pFont[i];
            pFont[i] = nullptr;

        }
    FontNr = 0;
}


void CDraw::Reset()
{
   D3DVIEWPORT9 screen;
   pDevice->GetViewport(&screen);

   Screen.Width = screen.Width;
   Screen.Height = screen.Height;
   Screen.x_center = Screen.Width/2;
   Screen.y_center = Screen.Height/2;
}

void CDraw::Line(float x1, float y1, float x2, float y2, float width, bool antialias, DWORD color)
{
   ID3DXLine *m_Line;

   D3DXCreateLine(pDevice, &m_Line);
   D3DXVECTOR2 line[] = {D3DXVECTOR2(x1, y1), D3DXVECTOR2(x2, y2)};
   m_Line->SetWidth(width);
   if(antialias) m_Line->SetAntialias(1);
   m_Line->Begin();
   m_Line->Draw(line, 2, color);
   m_Line->End();
   m_Line->Release();
}

void CDraw::Circle(float x, float y, float radius, int rotate, int type, bool smoothing, int resolution, DWORD color)
{
    std::vector<vertex> circle(resolution + 2);
    float angle = rotate*D3DX_PI/180;
    float pi;

    if(type == full) pi = D3DX_PI;        // Full circle
    if(type == half) pi = D3DX_PI/2;      // 1/2 circle
    if(type == quarter) pi = D3DX_PI/4;   // 1/4 circle

    for(int i = 0; i < resolution + 2; i++)
    {
        circle[i].x = (float)(x - radius*cos(i*(2*pi/resolution)));
        circle[i].y = (float)(y - radius*sin(i*(2*pi/resolution)));
        circle[i].z = 0;
        circle[i].rhw = 1;
        circle[i].color = color;
    }

    // Rotate matrix
//    int _res = resolution + 2;
//    for(int i = 0; i < _res; i++)
//    {
//        circle[i].x = x + cos(angle)*(circle[i].x - x) - sin(angle)*(circle[i].y - y);
//        circle[i].y = y + sin(angle)*(circle[i].x - x) + cos(angle)*(circle[i].y - y);
//    }

    // new Rotate
    int _res = resolution + 2;
    for(int i = 0; i < _res; i++)
    {
        // translate point back to origin:
        circle[i].x -= x;
        circle[i].y -= y;

        // rotate point
        float xnew = circle[i].x * cos(angle) - circle[i].y * sin(angle);
        float ynew = circle[i].x * sin(angle) + circle[i].y * cos(angle);

        // translate point back:
        circle[i].x = xnew + x;
        circle[i].y = ynew + y;
    }


    pDevice->CreateVertexBuffer((resolution + 2) * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW|D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL);

    VOID* pVertices;
    g_pVB->Lock(0, (resolution + 2) * sizeof(vertex), (void**)&pVertices, 0);
    memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(vertex));
    g_pVB->Unlock();


    pDevice->SetTexture(0, NULL);
    pDevice->SetPixelShader(NULL);
    if (smoothing)
    {
        pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
        pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
    }
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
    pDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);

    pDevice->DrawPrimitive(D3DPT_LINESTRIP, 0, resolution);
    if(g_pVB != NULL) g_pVB->Release();
}

void CDraw::CircleFilled(float x, float y, float rad, float rotate, int type, int resolution, DWORD color)
{
   std::vector<vertex> circle(resolution + 2);
   float angle = rotate*D3DX_PI/180;
   float pi;

   if(type == full) pi = D3DX_PI;        // Full circle
   if(type == half) pi = D3DX_PI/2;      // 1/2 circle
   if(type == quarter) pi = D3DX_PI/4;   // 1/4 circle

   circle[0].x = x;
   circle[0].y = y;
   circle[0].z = 0;
   circle[0].rhw = 1;
   circle[0].color = color;

   for(int i = 1; i < resolution + 2; i++)
   {
      circle[i].x = (float)(x - rad*cos(pi*((i-1)/(resolution/2.0f))));
      circle[i].y = (float)(y - rad*sin(pi*((i-1)/(resolution/2.0f))));
      circle[i].z = 0;
      circle[i].rhw = 1;
      circle[i].color = color;
   }

   // Rotate matrix
//   int _res = resolution + 2;
//   for(int i = 0; i < _res; i++)
//   {
//      circle[i].x = x + cos(angle)*(circle[i].x - x) - sin(angle)*(circle[i].y - y);
//      circle[i].y = y + sin(angle)*(circle[i].x - x) + cos(angle)*(circle[i].y - y);
//   }

   // new Rotate
   int _res = resolution + 2;
   for(int i = 0; i < _res; i++)
   {
       // translate point back to origin:
       circle[i].x -= x;
       circle[i].y -= y;

       // rotate point
       float xnew = circle[i].x * cos(angle) - circle[i].y * sin(angle);
       float ynew = circle[i].x * sin(angle) + circle[i].y * cos(angle);

       // translate point back:
       circle[i].x = xnew + x;
       circle[i].y = ynew + y;
   }

   pDevice->CreateVertexBuffer((resolution + 2) * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW|D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL );

   VOID* pVertices;
   g_pVB->Lock(0, (resolution + 2) * sizeof(vertex), (void**)&pVertices, 0);
   memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(vertex));
   g_pVB->Unlock();

   pDevice->SetTexture(0, NULL);
   pDevice->SetPixelShader(NULL);
   pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
   pDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
   pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);
   if(g_pVB != NULL) g_pVB->Release();
}

void CDraw::Box(float x, float y, float w, float h, float linewidth, DWORD color)
{
   if(linewidth == 0 || linewidth == 1)
   {
      BoxFilled(x, y, w, 1, color);             // Top
      BoxFilled(x, y+h-1, w, 1, color);         // Bottom
      BoxFilled(x, y+1, 1, h-2*1, color);       // Left
      BoxFilled(x+w-1, y+1, 1, h-2*1, color);   // Right
   }
   else
   {
      BoxFilled(x, y, w, linewidth, color);                                     // Top
      BoxFilled(x, y+h-linewidth, w, linewidth, color);                         // Bottom
      BoxFilled(x, y+linewidth, linewidth, h-2*linewidth, color);               // Left
      BoxFilled(x+w-linewidth, y+linewidth, linewidth, h-2*linewidth, color);   // Right
   }
}

void CDraw::BoxBordered(float x, float y, float w, float h, float border_width, DWORD color, DWORD color_border)
{
   BoxFilled(x, y, w, h, color);
   Box(x-border_width, y-border_width, w+2*border_width, h+border_width, border_width, color_border);
}

void CDraw::BoxFilled(float x, float y, float w, float h, DWORD color)
{
   vertex V[4];

   V[0].color = V[1].color = V[2].color = V[3].color = color;

   V[0].z = V[1].z = V[2].z = V[3].z = 0;
   V[0].rhw = V[1].rhw = V[2].rhw = V[3].rhw = 0;

   V[0].x = x;
   V[0].y = y;
   V[1].x = x+w;
   V[1].y = y;
   V[2].x = x+w;
   V[2].y = y+h;
   V[3].x = x;
   V[3].y = y+h;

   unsigned short indexes[] = {0, 1, 3, 1, 2, 3};

   pDevice->CreateVertexBuffer(4*sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW|D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL);
   pDevice->CreateIndexBuffer(2*sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIB, NULL);

   VOID* pVertices;
   g_pVB->Lock(0, sizeof(V), (void**)&pVertices, 0);
   memcpy(pVertices, V, sizeof(V));
   g_pVB->Unlock();

   VOID* pIndex;
   g_pIB->Lock(0, sizeof(indexes), (void**)&pIndex, 0);
   memcpy(pIndex, indexes, sizeof(indexes));
   g_pIB->Unlock();

   pDevice->SetTexture(0, NULL);
   pDevice->SetPixelShader(NULL);
   pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
   pDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
   pDevice->SetIndices(g_pIB);

   pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);

   g_pVB->Release();
   g_pIB->Release();
}

void CDraw::BoxRounded(float x, float y, float w, float h, float radius, bool smoothing, DWORD color, DWORD bcolor)
{
   BoxFilled(x+radius, y+radius, w-2*radius-1, h-2*radius-1, color);   // Center rect.
   BoxFilled(x+radius, y+1, w-2*radius-1, radius-1, color);            // Top rect.
   BoxFilled(x+radius, y+h-radius-1, w-2*radius-1, radius, color);     // Bottom rect.
   BoxFilled(x+1, y+radius, radius-1, h-2*radius-1, color);            // Left rect.
   BoxFilled(x+w-radius-1, y+radius, radius, h-2*radius-1, color);     // Right rect.

   // Smoothing method
   if (smoothing)
   {
      CircleFilled(x+radius, y+radius, radius-1, 0, quarter, 16, color);             // Top-left corner
      CircleFilled(x+w-radius-1, y+radius, radius-1, 90, quarter, 16, color);        // Top-right corner
      CircleFilled(x+w-radius-1, y+h-radius-1, radius-1, 180, quarter, 16, color);   // Bottom-right corner
      CircleFilled(x+radius, y+h-radius-1, radius-1, 270, quarter, 16, color);       // Bottom-left corner

      Circle(x+radius+1, y+radius+1, radius, 0, quarter, true, 16, bcolor);          // Top-left corner
      Circle(x+w-radius-2, y+radius+1, radius, 90, quarter, true, 16, bcolor);       // Top-right corner
      Circle(x+w-radius-2, y+h-radius-2, radius, 180, quarter, true, 16, bcolor);    // Bottom-right corner
      Circle(x+radius+1, y+h-radius-2, radius, 270, quarter, true, 16, bcolor);      // Bottom-left corner

      Line(x+radius, y+1, x+w-radius-1, y+1, 1, false, bcolor);       // Top line
      Line(x+radius, y+h-2, x+w-radius-1, y+h-2, 1, false, bcolor);   // Bottom line
      Line(x+1, y+radius, x+1, y+h-radius-1, 1, false, bcolor);       // Left line
      Line(x+w-2, y+radius, x+w-2, y+h-radius-1, 1, false, bcolor);   // Right line
   }
   else
   {
      CircleFilled(x+radius, y+radius, radius, 0, quarter, 16, color);             // Top-left corner
      CircleFilled(x+w-radius-1, y+radius, radius, 90, quarter, 16, color);        // Top-right corner
      CircleFilled(x+w-radius-1, y+h-radius-1, radius, 180, quarter, 16, color);   // Bottom-right corner
      CircleFilled(x+radius, y+h-radius-1, radius, 270, quarter, 16, color);       // Bottom-left corner
   }
}

void CDraw::Text(const char *text, float x, float y, int font, bool bordered, DWORD color, DWORD bcolor, int orientation)
{
   RECT rect;
   if(pFont[font]==nullptr) return;
   switch(orientation)
   {
   case lefted:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, color);
      break;
   case centered:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, color);
      break;
   case righted:
      if(bordered)
      {
         SetRect(&rect, x - 1, y , x - 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x + 1, y , x + 1, y);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y - 1, x, y - 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
         SetRect(&rect, x , y + 1, x, y + 1);
         pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, bcolor);
      }
      SetRect(&rect, x, y, x, y);
      pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, color);
      break;
   }
}

void CDraw::Message(const char *text, LONG x, LONG y, int font, int orientation)
{
    if(pFont[font]==nullptr) return;
    RECT rect = {x, y, x, y};

    switch(orientation)
    {
        case lefted:
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CALCRECT|DT_LEFT, BLACK(255));
            BoxRounded(x - 5, rect.top - 5, rect.right - x + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));
            SetRect(&rect, x, y, x, y);
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_LEFT|DT_NOCLIP, ORANGE(255));
            break;
        case centered:
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CALCRECT|DT_CENTER, BLACK(255));
            BoxRounded(rect.left - 5 , rect.top - 5, rect.right - rect.left + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));
            SetRect(&rect, x, y, x, y);
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CENTER|DT_NOCLIP, ORANGE(255));
            break;
        case righted:
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_CALCRECT|DT_RIGHT, BLACK(255));
            BoxRounded(rect.left - 5, rect.top - 5, rect.right - rect.left + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));
            SetRect(&rect, x, y, x, y);
            pFont[font]->DrawTextA(NULL,text,-1,&rect, DT_RIGHT|DT_NOCLIP, ORANGE(255));
            break;
    }
}

void CDraw::Sprite(LPDIRECT3DTEXTURE9 tex, float x, float y, float resolution, float scale, float rotation)
{
   float screen_pos_x = x;
   float screen_pos_y = y;

   // Texture being used is 64x64:
   D3DXVECTOR2 spriteCentre = D3DXVECTOR2(resolution/2, resolution/2);

   // Screen position of the sprite
   D3DXVECTOR2 trans = D3DXVECTOR2(screen_pos_x, screen_pos_y);

   // Build our matrix to rotate, scale and position our sprite
   D3DXMATRIX mat;

   D3DXVECTOR2 scaling(scale, scale);

   // out, scaling centre, scaling rotation, scaling, rotation centre, rotation, translation
   D3DXMatrixTransformation2D(&mat, NULL, 0.0, &scaling, &spriteCentre, rotation, &trans);

   //pDevice->SetRenderState(D3DRS_ZENABLE, false);
   pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
   pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
   pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
   pDevice->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
   pDevice->SetPixelShader(NULL);
   sSprite->Begin(0);
   sSprite->SetTransform(&mat); // Tell the sprite about the matrix
   sSprite->Draw(tex, NULL, NULL, NULL, 0xFFFFFFFF);
   sSprite->End();
}

bool CDraw::Font()
{
   for(int i=0; i<FontNr; i++)
      if(pFont[i]==nullptr) return false;
   return true;
}

HRESULT CDraw::AddFont(const char* Caption, float size, bool bold, bool italic)
{
    qDebug() << "Draw AddFont" << Caption << size << FontNr;
    HRESULT hr = D3DXCreateFontA(pDevice, size, 0, (bold) ? FW_BOLD : FW_NORMAL, 1, (italic) ? 1 : 0 , DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, Caption, &pFont[FontNr]);
    if(SUCCEEDED(hr))
        ++FontNr;
    return hr;
}

//void CDraw::FontReset()
//{
//   for(int i = 0; i < FontNr; i++)
//       if(pFont[i]) pFont[i]->OnLostDevice();
//   for(int i = 0; i < FontNr; i++)
//       if(pFont[i]) pFont[i]->OnResetDevice();
//}

void CDraw::OnResetDevice()
{
    for(int i = 0; i < FontNr; i++)
        if(pFont[i]!=nullptr){
            HRESULT hr = pFont[i]->OnResetDevice();
            qDebug() << "Draw OnResetDevice()" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
        }
}

void CDraw::OnLostDevice()
{
    for(int i = 0; i < FontNr; i++)
        if(pFont[i]!=nullptr){
            HRESULT hr = pFont[i]->OnLostDevice();
            qDebug() << "Draw OnLostDevice()" << DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr);
        }
}

void CDraw::ReleaseFonts()
{
    for(int i = 0; i < FontNr; i++)
        if(pFont[i]!=nullptr){
            HRESULT hr = pFont[i]->Release();

//            if(FAILED(hr))
                qDebug() << "Render ReleaseFonts()"<< i <<  DXGetErrorString9A(hr) << DXGetErrorDescription9A(hr) << (PVOID)pFont[i];;
            pFont[i] = nullptr;
        }
    FontNr = 0;
}

void  CDraw::Draw_Box(int x, int y, int w, int h,   D3DCOLOR Color)
{
    D3DRECT rec;
    rec.x1 = x;
    rec.x2 = x + w;
    rec.y1 = y;
    rec.y2 = y + h;
//    DWORD FVF;
//    pDevice->GetFVF(&FVF);
//    pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
    pDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,	D3DPT_TRIANGLESTRIP);
    pDevice->Clear( 1, &rec, D3DCLEAR_TARGET, Color, 1, 1 );
//    pDevice->SetFVF(FVF);

}
void  CDraw::Draw_Border(int x, int y, int w, int h,int s, D3DCOLOR Color)
{
    Draw_Box(x,  y, s,  h,Color);
    Draw_Box(x,y+h, w,  s,Color);
    Draw_Box(x,  y, w,  s,Color);
    Draw_Box(x+w,y, s,h+s,Color);
}


//void CDraw::DrawTextBox(int x, int y, int w, int h, D3DCOLOR Color)
//{

//}

int CDraw::GetTextLen(LPCTSTR szString, int font)
{
    if(pFont[font]==nullptr) return 0;
    RECT rect = {0,0,0,0};
    pFont[font]->DrawText(NULL, szString, -1, &rect, DT_CALCRECT, 0);
    return rect.right;
}

int CDraw::GetTextLen(const char *szString, int font)
{
    if(pFont[font]==nullptr) return 0;
    RECT rect = {0,0,0,0};
//    pFont[font]->DrawTextA(NULL, szString, -1, &rect, DT_CALCRECT, 0);
    // определим размер памяти, необходимый для хранения Unicode-строки
    int length = MultiByteToWideChar(CP_UTF8, 0, szString, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    // конвертируем ANSI-строку в Unicode-строку
    MultiByteToWideChar(CP_UTF8, 0, szString, -1, ptr, length);
    pFont[font]->DrawText(NULL, ptr, -1, &rect, DT_CALCRECT, 0);

    return rect.right;
}

void CDraw::String(int x, int y, DWORD color, const char *text, int font,  DWORD style)
{
    if(pFont[font]==nullptr) return;
    RECT rect;
    SetRect(&rect, x, y, x, y);
    // определим размер памяти, необходимый для хранения Unicode-строки
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    // конвертируем ANSI-строку в Unicode-строку
    MultiByteToWideChar(CP_UTF8, 0, text, -1, ptr, length);
    pFont[font]->DrawText(NULL, ptr, -1, &rect, style, color);
    return;
}

void CDraw::StringChar(int x, int y, DWORD color, const char *text, int font,  DWORD style)
{
    if(pFont[font]==nullptr) return;
    RECT rect;
    SetRect(&rect, x, y, x, y);
    pFont[font]->DrawTextA(NULL, text, -1, &rect, style, color);
    return;
}

void CDraw::String(int x, int y, DWORD color, LPCTSTR text, int font,  DWORD Style)
{
    if(pFont[font]==nullptr) return;
    RECT rect;
    SetRect(&rect, x, y, x, y);
    pFont[font]->DrawText(NULL, text, -1, &rect, Style, color);
    return;
}

