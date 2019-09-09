#include "stdafx.h"
#include "callback.h"
#include "interface.h"
#include "../fun.h"
#include "../MainWnd.h"
#include "../LoginWnd.h"
#include <shellapi.h>
#include "../friendWnd.h"

#define hLoginWnd	pLoginWnd->GetHWND()	//��¼���ھ��
#define hMainWnd	pMainWnd->GetHWND()		//wx�����ھ��


//��ǰ��¼�˺�wxid
char g_wxid[100] = { 0 };

//��ǰ��¼�˺�uin
DWORD g_dwUin = 0;

static void MessageBoxThread(LPCTSTR szMsg)
{
	MessageBox(hLoginWnd, szMsg, NULL, NULL);
}
static void ForceQuit(CString str)
{
	CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MessageBoxThread, str.GetBuffer(), NULL, NULL));
	Sleep(3 * 1000);
	TerminateProcess(GetCurrentProcess(), 0);
}
#define FORCE_QUIT(szMsg) ForceQuit(szMsg)

//�ص�ʧ�ܴ���
void HandleCallBackError(void *result, int nCgiType, int nTaskId, int nCode)
{
	CString strErrMsg;

	switch (nCode)
	{
	case CGI_CODE_LOGIN_NEED_SCAN_QRCODE:
		//ɨ���¼����
		PostMessage(hLoginWnd, WM_LOGIN_SCAN_QRCODE, (WPARAM)result, (LPARAM)nCode);
		break;
	case CGI_CODE_LOGIN_NEED_MOBILE_MSG:
		//���ͽ��ն�����֤������
		RecvMobileVerifyCode();
		break;
	case CGI_CODE_LOGIN_FAIL:
		//������¼����
		PostMessage(hLoginWnd, WM_SHOW_LOGIN_RESULT, (WPARAM)result, (LPARAM)nCode);
		break;
	case CGI_CODE_NETWORK_ERR:
		//�����쳣,�����˳�
		strErrMsg.Format(L"�����쳣,�����˳�......\ncgi type:%d,taskid:%d,code:%d", nCgiType, nTaskId, nCode);
		FORCE_QUIT(strErrMsg);
		break;
	case CGI_CODE_DECRYPT_ERR:
	case CGI_CODE_PARSE_PROTOBUF_ERR:
	case CGI_CODE_UNPACK_ERR:
		//ͨѶЭ�����,�����˳�
		strErrMsg.Format(L"ͨ��Э�����,�����˳�......\ncgi type:%d,taskid:%d,code:%d", nCgiType, nTaskId, nCode);
		FORCE_QUIT(strErrMsg);
		break;
	case CGI_CODE_LOGIN_ECDH_ERR:
		//ECDHЭ��ʧ��,�����˳�
		strErrMsg.Format(L"ECDH����ʧ��,�����˳�......\ncgi type:%d,taskid:%d,code:%d", nCgiType, nTaskId, nCode);
		FORCE_QUIT(strErrMsg);
		break;
	case CGI_CODE_LOGIN_SCAN_QRCODE_ERR:
		//ɨ����Ȩurl��ȡʧ��,�����˳�
		strErrMsg.Format(L"ɨ����Ȩ��ַ��ȡʧ��,�����˳�......\ncgi type:%d,taskid:%d,code:%d", nCgiType, nTaskId, nCode);
		FORCE_QUIT(strErrMsg);
		break;
	case CGI_ERR_UNKNOWN:
		//δ֪����,�����˳�
		strErrMsg.Format(L"����δ֪�쳣,�����˳�......\ncgi type:%d,taskid:%d,code:%d", nCgiType, nTaskId, nCode);
		FORCE_QUIT(strErrMsg);
		break;
	default:
		break;
	}
}

void SDKCallBack(void *result, int nCgiType, int nTaskId, int nCode)
{
	if (nCode)
	{
		//ʧ�ܴ���(result�ڴ���UI�߳����ͷ�)
		HandleCallBackError(result, nCgiType,nTaskId,nCode);
	}
	else
	{
		switch (nCgiType)
		{
		case CGI_TYPE_MANUALAUTH:
			//��½�ɹ�,��ʾ������
			LoginCallBack((LoginResult *)result);			
			break;
		case CGI_TYPE_BIND:
			//������֤
			MobileVerifyCallBack((MobileVerifyResult *)result);
			break;
		case CGI_TYPE_NEWINIT:
			NewInitCallBack((NewInitResult *)result);
			break;
		case CGI_TYPE_NEWSYNC:
			//ͬ���ɹ���ˢ��UI
			NewSyncCallBack((NewSyncResult *)result);
			break;
		case CGI_TYPE_SEARCHCONTACT:
			//������ϵ��
			SearchContactCallBack((SearchContactResult *)result);
			break;
		case CGI_TYPE_VERIFYUSER:
			//�Ӻ���
			AddNewFriendCallBack((VerifyUserResult *)result);
			break;
		default:
			break;
		}
	}	
}

void LoginCallBack(LoginResult *res)
{
	//��½�ɹ�,����wxid��uin
	g_dwUin = res->dwUin;
	strcpy_s(g_wxid,res->szWxid);
	
	if (res)
	{
		SAFE_DELETE(res);
	}

	/** @brief ��¼�ɹ� **/
	PostMessage(hLoginWnd,WM_LOGIN_SUCC, NULL, NULL);
}

void MobileVerifyCallBack(MobileVerifyResult *res)
{
	PostMessage(hLoginWnd, WM_MOBILE_VERIFY, (WPARAM)res, NULL);
}

void NewInitCallBack(NewInitResult *res)
{
	if (!res)	return;

	if (res->dwContanct)
	{
		for (int i = 0; i < res->dwContanct; i++)
		{
			PostMessage(hMainWnd, WM_MAIN_ADD_FRIEND, (WPARAM)res->ppContanct[i], NULL);
		}
		for (int i = res->dwContanct; i < res->dwSize; i++)
		{
			SAFE_DELETE(res->ppContanct[i]);
		}
	}
	if (res->dwNewMsg)
	{
		for (int i = 0; i < res->dwNewMsg; i++)
		{
			//����weixin˵�ķϻ�
			if ((res->ppNewMsg[i]->nType == 10002 || res->ppNewMsg[i]->nType == 9999) && !strcmp(res->ppNewMsg[i]->szFrom, "weixin"))
			{
				SAFE_DELETE(res->ppNewMsg[i]);
				continue;
			}
			PostMessage(hMainWnd, WM_MAIN_ADD_NEW_MSG, (WPARAM)res->ppNewMsg[i], NULL);
		}
		for (int i = res->dwNewMsg; i < res->dwSize; i++)
		{
			SAFE_DELETE(res->ppNewMsg[i]);
		}
	}
	
	//��ʼ��������Ҫ��������һ��ͬ��
	NewSync();
}

void NewSyncCallBack(NewSyncResult *res)
{
	if (!res)	return;

	//���������Ϣ
	if (res->dwContanct)
	{
		for (int i = 0; i < res->dwContanct; i++)
		{
			PostMessage(hMainWnd, WM_MAIN_ADD_FRIEND, (WPARAM)res->ppContanct[i], NULL);
		}
		for (int i = res->dwContanct; i < res->dwSize; i++)
		{
			SAFE_DELETE(res->ppContanct[i]);
		}
	}

	if (res->dwNewMsg)
	{
		for (int i = 0; i < res->dwNewMsg; i++)
		{
			//����weixin˵�ķϻ�
			if ((res->ppNewMsg[i]->nType == 10002 || res->ppNewMsg[i]->nType == 9999) && !strcmp(res->ppNewMsg[i]->szFrom,"weixin") )
			{
				SAFE_DELETE(res->ppNewMsg[i]);
				continue;
			}
			PostMessage(hMainWnd, WM_MAIN_ADD_NEW_MSG, (WPARAM)res->ppNewMsg[i], NULL);
		}
		for (int i=res->dwNewMsg;i<res->dwSize;i++)
		{
			SAFE_DELETE(res->ppNewMsg[i]);
		}
	}

}

void SearchContactCallBack(SearchContactResult *res)
{
	PostMessage(pFriendWnd->GetHWND(), WM_FRIEND_SEARCH_CONTACT, (WPARAM)res, NULL);
}

void AddNewFriendCallBack(VerifyUserResult *res)
{
	PostMessage(pFriendWnd->GetHWND(), WM_FRIEND_ADD_NEW_FRIEND, (WPARAM)res, NULL);
}

