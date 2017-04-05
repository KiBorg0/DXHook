#include "cRender.h"

extern int Button_Mass[20],
	       Button_Number;




 DWORD test_Color_1[] = {C_Fon_Ctrl,RED,BLUE,BLACK,WHITE,GREEN,Violet,ORANGE,YELLOW,line_Color,DarkGoldenrod };
 DWORD test_Color_2[] = {C_Fon_Ctrl,BLUE,WHITE,GREEN,Violet,ORANGE,YELLOW };

void  cRender::SHOW_MENU(LPDIRECT3DDEVICE9  pDevice)
{

	Draw_Menu_But(&pos_Menu,"Меню подцветки", pDevice);
	if(Button_Mass[0])
	{

		Draw_CheckBox(&pos_Menu,Fun._WallHack,"WallHack",pDevice);
		
		Draw_ColorBox(&pos_Menu, "Террористы", Fun._Terr, test_Color_1,10,  pDevice);
		Draw_ColorBox(&pos_Menu, "Копы",       Fun._Cop, test_Color_1,10,  pDevice);
		Draw_ColorBox(&pos_Menu, "Оружие",     Fun._Weapon, test_Color_1,6,  pDevice);
		Draw_ColorBox(&pos_Menu, "Дино",       Fun._Dino, test_Color_2,4,  pDevice);
		Draw_ColorBox(&pos_Menu, "Голова",     Fun._Head, test_Color_1,4,  pDevice);
		Draw_CheckBox(&pos_Menu,Fun._XHair,"Прицел",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._Trace,"Траектория",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._Grena,"Гранаты",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._NoSmok,"Нет дыма",pDevice);
		


	}

	Draw_Menu_But(&pos_Menu,"Меню оружия", pDevice);
	if(Button_Mass[1])
	{
		Draw_ScrolBox(&pos_Menu,"Дамаг", Fun._Damage,100,pDevice);
		Draw_CheckBox(&pos_Menu,Fun._recoil,"Отдача",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._scorostr,"Скорострельность",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._D_knife,"ДН",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._smena,"Быстрая смена",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._reload,"Быстрая перезарядка",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._anlimP,"Анлим патроны",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._remboM,"Рембо мод",pDevice);
	}

	Draw_Menu_But(&pos_Menu,"Меню Купонов", pDevice);
	if(Button_Mass[2])
	{
		Draw_CheckBox(&pos_Menu,Fun._110_xp,"110 XP",pDevice);
	}

	Draw_Menu_But(&pos_Menu,"Меню Фугаса", pDevice);
	if(Button_Mass[3])
	{
		Draw_CheckBox(&pos_Menu,Fun._miner,"Мгновенное М/Р",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._fastFgs,"Установка",pDevice);
		
	}

	Draw_Menu_But(&pos_Menu,"Меню Прыжков", pDevice);
	if(Button_Mass[4])
	{
              Draw_ScrolBox(&pos_Menu,"Высота прыжка", Fun._jumpV,10,pDevice);
			  Draw_CheckBox(&pos_Menu,Fun._NFD,"Нет урона при падении",pDevice);
			  Draw_CheckBox(&pos_Menu,Fun._jump_to,"Прыжок в воздухе",pDevice);
	}
	Draw_Menu_But(&pos_Menu,"Меню Респавна", pDevice);
	if(Button_Mass[5])
	{
		Draw_CheckBox(&pos_Menu,Fun._svetofor,"Отключить мигание",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._resp,"Быстрый респавн",pDevice);
		Draw_CheckBox(&pos_Menu,Fun._RiD,"РНС",pDevice);
	}
	Draw_Menu_But(&pos_Menu,"Меню Разного", pDevice);
	if(Button_Mass[6])
	{
		Draw_CheckBox(&pos_Menu,Fun._esp,"ESP",pDevice);
		 Draw_ScrolBox(&pos_Menu,"SpeedHack", Fun._SH,5,pDevice);
		Draw_CheckBox(&pos_Menu,Fun._tusa,"Тусовка",pDevice);

	}

	Button_Number=0;
}