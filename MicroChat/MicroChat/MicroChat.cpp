// MicroChat.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "MicroChat.h"
#include "LoginWnd.h"
#include "MainWnd.h"
#include "interface.h"
#include "friendWnd.h"



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);

	//����Loading���� 3s���Զ�����
	/*CLoadingWnd *pLoading = new CLoadingWnd();
	pLoading->Create(NULL, _T("MicroChat"), UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
	pLoading->ShowModal();
	delete pLoading;
	pLoading = NULL;*/

	//�����¼����
	pLoginWnd->Create(NULL, _T("MicroChat"), UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
	pLoginWnd->ShowModal();
	delete pLoginWnd;

	//ж��SDK
	StopSDK();

    return 0;
}



