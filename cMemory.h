#pragma once
#include <Windows.h>

#define new_My_Thread(Function) CreateThread(0,0,(LPTHREAD_START_ROUTINE)Function,0,0,0);

BOOL  bCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
DWORD FindPattern(DWORD dwAddress,DWORD dwLen,BYTE *bMask,char * szMask);
void *Create_Hook(BYTE *src, const BYTE *dst, const int len);

