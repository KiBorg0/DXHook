#pragma once

#include <d3d9.h>

namespace hooks {
    extern HRESULT (STDMETHODCALLTYPE *original_present) (LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);
    extern HRESULT (STDMETHODCALLTYPE *original_reset) (LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
    extern HRESULT (STDMETHODCALLTYPE *original_release) (LPDIRECT3DDEVICE9);
    extern HRESULT STDMETHODCALLTYPE user_present(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
    extern HRESULT STDMETHODCALLTYPE user_reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* params);
    extern HRESULT STDMETHODCALLTYPE user_release(LPDIRECT3DDEVICE9 pDevice);
};
