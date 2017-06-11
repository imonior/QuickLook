#include "stdafx.h"

#include "Shell32.h"
#include "HelperMethods.h"
#include "DialogHook.h"

using namespace std;

Shell32::FocusedWindowType Shell32::GetFocusedWindowType()
{
	auto type = INVALID;

	auto hwndfg = GetForegroundWindow();

	if (HelperMethods::IsCursorActivated(hwndfg))
		return INVALID;

	WCHAR classBuffer[MAX_PATH] = {'\0'};
	if (SUCCEEDED(GetClassName(hwndfg, classBuffer, MAX_PATH)))
	{
		if (wcscmp(classBuffer, L"WorkerW") == 0 || wcscmp(classBuffer, L"Progman") == 0)
		{
			if (FindWindowEx(hwndfg, nullptr, L"SHELLDLL_DefView", nullptr) != nullptr)
			{
				type = DESKTOP;
			}
		}
		else if (wcscmp(classBuffer, L"ExploreWClass") == 0 || wcscmp(classBuffer, L"CabinetWClass") == 0)
		{
			type = EXPLORER;
		}
		else if (wcscmp(classBuffer, L"#32770") == 0)
		{
			if (FindWindowEx(hwndfg, nullptr, L"DUIViewWndClassName", nullptr) != nullptr)
			{
				type = DIALOG;
			}
		}
	}

	return type;
}

void Shell32::GetCurrentSelection(PWCHAR buffer)
{
	switch (GetFocusedWindowType())
	{
	case DESKTOP:
		getSelectedFromDesktop(buffer);
		break;
	case EXPLORER:
		getSelectedFromExplorer(buffer);
		break;
	case DIALOG:
		DialogHook::GetSelectedFromCommonDialog(buffer);
		break;
	default:
		break;
	}
}

void Shell32::getSelectedFromExplorer(PWCHAR buffer)
{
	CoInitialize(nullptr);

	CComPtr<IShellWindows> psw;
	if (FAILED(psw.CoCreateInstance(CLSID_ShellWindows)))
		return;

	auto hwndfg = GetForegroundWindow();

	auto count = 0L;
	psw->get_Count(&count);

	for (auto i = 0; i < count; i++)
	{
		VARIANT vi;
		V_VT(&vi) = VT_I4;
		V_I4(&vi) = i;

		CComPtr<IDispatch> pdisp;
		// ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
		if (FAILED(psw->Item(vi, &pdisp)))
			continue;

		CComQIPtr<IWebBrowserApp> pwba;
		if (FAILED(pdisp->QueryInterface(IID_IWebBrowserApp, reinterpret_cast<void**>(&pwba))))
			continue;

		HWND hwndwba;
		if (FAILED(pwba->get_HWND(reinterpret_cast<LONG_PTR*>(&hwndwba))))
			continue;

		if (hwndwba != hwndfg || HelperMethods::IsCursorActivated(hwndwba))
			continue;

		HelperMethods::GetSelectedInternal(pwba, buffer);
	}
}

void Shell32::getSelectedFromDesktop(PWCHAR buffer)
{
	CoInitialize(nullptr);

	CComPtr<IShellWindows> psw;
	CComQIPtr<IWebBrowserApp> pwba;

	if (FAILED(psw.CoCreateInstance(CLSID_ShellWindows)))
		return;

	VARIANT pvarLoc = {VT_EMPTY};
	long phwnd;
	if (FAILED(psw->FindWindowSW(&pvarLoc, &pvarLoc, SWC_DESKTOP, &phwnd, SWFO_NEEDDISPATCH, reinterpret_cast<IDispatch**>(&pwba))))
		return;

	if (HelperMethods::IsCursorActivated(reinterpret_cast<HWND>(LongToHandle(phwnd))))
		return;

	HelperMethods::GetSelectedInternal(pwba, buffer);
}
