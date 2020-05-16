#include "cRender.h"
#include <vector>
#include <string>
#include <QStringList>
#include <QDebug>

using namespace std;

int ButtonMass[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct _Keys
{
    bool        bPressed;
    DWORD       dwStartTime;
}kPressingKeys[256];

QStringList racesUC = {"Random","Space Marines","Chaos","Orks","Eldar","Imperial Guard","Necrons","Tau Empire","Sisters of Battle","Dark Eldar"};
QStringList playerInfo = {"Favorite Race ", "Games Played ","Games Won ","(%)Win Ratio ","APM ","MMR ","MMR 1VS1 "};

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

void  cRender::drawBox(int x, int y, int w, int h,   D3DCOLOR Color)
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
void  cRender::drawBorder(int x, int y, int w, int h,int s, D3DCOLOR Color)
{
    drawBox(x,  y, s,  h,Color);
    drawBox(x,y+h, w,  s,Color);
    drawBox(x,  y, w,  s,Color);
    drawBox(x+w,y, s,h+s,Color);
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
int cRender::GetTextLenWChar(const char *szString, int font)
{
    if(pFont[font]==nullptr) return 0;
    RECT rect = {0,0,0,0};
    pFont[font]->DrawText(NULL, MyCharToWideChar(szString), -1, &rect, DT_CALCRECT, 0);
    return rect.right;
}
wchar_t *cRender::MyCharToWideChar(const char *data)
{
    // определим размер памяти, необходимый для хранения Unicode-строки
    int length = MultiByteToWideChar(CP_UTF8, 0, data, -1, NULL, 0);
    wchar_t *ptr = new wchar_t[length];
    // конвертируем ANSI-строку в Unicode-строку
    MultiByteToWideChar(CP_UTF8, 0, data, -1, ptr, length);
    return ptr;
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

void cRender::setGameInfo(PGameInfo _gameInfo)
{
    qDebug() << _gameInfo->lPlayers[0].name << _gameInfo->playersNumber;
    gameInfo = _gameInfo;
    qDebug() << gameInfo->lPlayers[0].name << gameInfo->playersNumber;
//    playersNumber = gameInfo->playersNumber;
//    memcpy(&Players, &gameInfo->lPlayers, sizeof(TPlayer)*50);
}

void  cRender::initPosMenu(const char* Titl, int font)
{
    int title_len = GetTextLen(Titl, font);
    int menu_margin = 0.8*titleFontSize*((double)h_screen/(double)w_screen);
    h_menu = titleFontSize+menu_margin*2;
    w_menu = title_len+menu_margin*2+h_menu;
    int x = w_screen - w_menu-1;

    int y = 0;
    pos_Menu.x = x;
    pos_Menu.y = y+h_menu+2;
    pos_Menu._y = y+h_menu+2;

    drawBox(x,y,w_menu,h_menu,DARKGRAY(150));
    drawBorder(x,y,w_menu,h_menu,1,SKYBLUE(255));
    drawBorder(x+w_menu-h_menu,y,h_menu,h_menu,1,SKYBLUE(255));

    D3DCOLOR color = ORANGE(255);

    // if cursor pos is in menu box
    if(IsInBox(x,y,w_menu,h_menu)){
        // if mouse clicked
        if (State_Key(VK_LBUTTON,200) )
            // if menu button wasnt clicked, set is clicked
            if (!ButtonMass[0])
                ButtonMass[0] = 1;
            // else if menu button was clicked, unclick all buttons of menu and menu button too
            else
                for ( int i = 0; i <= gameInfo->playersNumber; i++ )
                    ButtonMass[i] = 0;
        color = C_BUT_text_In;
    }
    else if(!IsInBox(x,y,w_menu, pos_Menu.height_fon+20)&&State_Key(VK_LBUTTON,200))
        for ( int i = 0; i <= gameInfo->playersNumber; i++ )
            ButtonMass[i] = 0;

    Text(Titl, x+(w_menu-h_menu)/2, menu_margin, 0, 0, color, BLACK, 1);

    const char *arrow = ButtonMass[0]?"▼":"▲";
    String(x+w_menu-h_menu/2+2, menu_margin, color, arrow, 0, C_Text);
    // if menu button is clicked show menu
    if (ButtonMass[0])
        showMenu();
}
void cRender::showMenu()
{
    qDebug() << gameInfo->lPlayers[0].name << gameInfo->playersNumber;
    for(int playerNum=0; playerNum<gameInfo->playersNumber; ++playerNum){
        int x = pos_Menu.x,
            y = pos_Menu.y,
            h = 22,
            w = w_menu;
        D3DCOLOR Bord_text = SKYBLUE(255);
        D3DCOLOR text_Activ = ORANGE(255);
        int ButtonNumber = playerNum + 1;
        if(IsInBox(x,y,w,h))
        {
            Bord_text = text_Activ = C_BUT_text_In;
            if (State_Key(VK_LBUTTON,200) )
                if (!ButtonMass[ButtonNumber])
                    ButtonMass[ButtonNumber]= 1;
                else
                    ButtonMass[ButtonNumber] = 0;
        }

        drawBox(x, y, w,h, DARKGRAY(150));
        drawBorder(x, y, w, h,1,Bord_text);
        String(x+(w/2),y+3,text_Activ, gameInfo->lPlayers[playerNum].name, 1, C_Text);


        // if player button clicked
        if (ButtonMass[ButtonNumber])
        {
            int x_sub = pos_Menu.x-w_sub_menu-2;
            int sub_menu_size = playerInfo.size()*fontSize;
            drawBox(x_sub, pos_Menu._y, w_sub_menu,sub_menu_size+3, DARKGRAY(150));
            drawBorder(x_sub, pos_Menu._y, w_sub_menu,sub_menu_size+3, 1, SKYBLUE(255));
            int item_index=0;
            for(int next_pos=3; next_pos<sub_menu_size; next_pos+=fontSize){
                String(x_sub+(w/2),pos_Menu._y+next_pos,C_BUT_text_In, playerInfo.at(item_index).toStdString().data(), 1, R_Text);
                if(item_index)
                    String(x_sub+(w/2)+4,pos_Menu._y+next_pos,C_BUT_text_In, to_string(gameInfo->lPlayers[playerNum].info[item_index]).data(), 1, L_Text);
                else
                    String(x_sub+(w/2)+4,pos_Menu._y+next_pos,C_BUT_text_In, racesUC.at(gameInfo->lPlayers[playerNum].info[item_index]).toStdString().data(), 1, L_Text);
                item_index++;
            }

            // change button color and unclick other player buttons
            text_Activ = C_BUT_text_On;
            for ( int i = 1; i <= gameInfo->playersNumber; i++)
                if ( i != ButtonNumber )
                    ButtonMass[i] = 0;
        }

        pos_Menu.y = y+24;
        pos_Menu.height_fon =pos_Menu.y-20;
    }
}

void cRender::setMenuParams(int fontSize, int width, int height)
{
    titleFontSize = fontSize;
    h_screen = height;
    w_screen = width;
}

bool cRender::AddFont(const char* Caption, int size, bool bold, bool italic)
{
    qDebug() << "Render AddFont" << Caption << size << FontNr;
    fontSize = size;
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
