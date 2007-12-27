
//         Copyright E�in O'Callaghan 2006 - 2007.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "stdAfx.hpp"
#include "UxthemeWrapper.hpp"

template <class T, class TBase = CWindow>
class ATL_NO_VTABLE CHalTabPageImpl : public ATL::CDialogImpl< T >
{
public:
    BEGIN_MSG_MAP_EX(CHalTabPageImpl)
//		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_CTLCOLORDLG(OnCltColorDlg)
		MSG_WM_CTLCOLORBTN(OnCltColor)
//		MSG_WM_CTLCOLOREDIT(OnCltColorEdit)
		MSG_WM_CTLCOLORSTATIC(OnCltColor)

        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

protected:
	LRESULT OnEraseBkgnd(HDC dc)
	{
		return 1;
		
		CRect rect;
		GetClientRect(rect);
		
		if(hal::uxtheme().pIsAppThemed && hal::uxtheme().pIsAppThemed())
		{
			if (hal::uxtheme().pDrawThemeParentBackground)
			{
				hal::uxtheme().pDrawThemeParentBackground(*this, dc, rect);
			}
		}
		
		return 1;
	}
	

	LRESULT OnCltColorDlg(HDC hDC, HWND hWnd)
	{
		SetMsgHandled(false);

		if (hal::uxtheme().pIsAppThemed)
			if(hal::uxtheme().pIsAppThemed())
			{
				RECT rect;
				GetClientRect(&rect);
				if (hal::uxtheme().pDrawThemeParentBackground)
				{
					hal::uxtheme().pDrawThemeParentBackground(hWnd, hDC, &rect);
					SetMsgHandled(true);
				}
			}

		return (LRESULT)::GetStockObject(HOLLOW_BRUSH);
	}

	LRESULT OnCltColor(HDC hDC, HWND hWnd)
	{
		SetMsgHandled(false);

		if (hal::uxtheme().pIsAppThemed)
			if(hal::uxtheme().pIsAppThemed())
			{
				RECT rect;
				::GetClientRect(hWnd, &rect);
				::SetBkMode(hDC, TRANSPARENT);
				if (hal::uxtheme().pDrawThemeParentBackground)
				{
					hal::uxtheme().pDrawThemeParentBackground(hWnd, hDC, &rect);
					SetMsgHandled(true);
				}
			}

		return (LRESULT)::GetStockObject(HOLLOW_BRUSH);
	}
	
	LRESULT OnCltColorEdit(HDC hDC, HWND hWnd)
	{
		SetMsgHandled(true);

    SetTextColor(hDC, RGB(255,0,0));            // red
   // SetBkColor(hDC, RGB(255,255,0));            // yellow
    return (LRESULT)::GetSysColorBrush(COLOR_WINDOW);   // hilight colour

	}
};