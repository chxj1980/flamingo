#include "stdafx.h"
#include "friendWnd.h"
#include "interface.h"
#include "Business/callback.h"
#include <shellapi.h>


LRESULT CFriendWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (0 == wParam)
	{
		KillTimer(m_hWnd,0);
		ShowWindow(FALSE);
	}
	
	bHandled = FALSE;
	return 0;
}

LRESULT CFriendWnd::OnSearchContact(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SearchContactResult *res = (SearchContactResult *)wParam;
	if (res)
	{
		if (!res->nCode)
		{
			CStringA strText;
			strText.Format("��ѯ�ɹ�!\n�ǳ�:%s\nv1����:%s\nv2����:%s",res->szNickName,res->szV1_Name,res->szV2_Name);
			CString str = CA2W(strText);
			ShowText(str, TRUE);

			//����ͷ��
			CDuiString strUrl = CA2W(res->szHeadIcon);
			CDuiString strFileName;
			strFileName.Format(L".\\skin\\head\\%s.png", CA2W(res->szV1_Name));
			CreateDirectory(L".\\skin\\head", NULL);

			HRESULT hr = URLDownloadToFile(0, strUrl, strFileName, 0, NULL);
			if (S_OK == hr)
			{
				CLabelUI *pLabel = (CLabelUI *)m_PaintManager.FindControl(L"headicon");
				if (pLabel)
				{
					CDuiString strFileName;
					strFileName.Format(L"head/%s.png", CA2W(res->szV1_Name));
					pLabel->SetBkImage(strFileName);
				}
			}
		}
		else
		{
			CStringA strText;
			strText.Format("������:%d\n������Ϣ:%s", res->nCode, res->szMsgResult);
			CString str = CA2W(strText);
			ShowText(str, TRUE);

			CLabelUI *pLabel = (CLabelUI *)m_PaintManager.FindControl(L"headicon");
			if (pLabel)
			{
				CDuiString strFileName;
				strFileName.Format(L"friend/icon.png");
				pLabel->SetBkImage(strFileName);
			}
		}

		SAFE_DELETE(res);
	}
	else
	{
		ShowText(L"δ֪����,����´���!", TRUE);
	}	
	
	bHandled = TRUE;
	return 0;
}

LRESULT CFriendWnd::OnAddNewFriend(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	VerifyUserResult *res = (VerifyUserResult *)wParam;
	if (res)
	{
		if (!res->nCode)
		{
			CStringA strText;
			strText.Format("��Ӻ��������ͳɹ�!\n");
			CString str = CA2W(strText);
			ShowText(str, TRUE);
		}
		else if (-44 == res->nCode)
		{
			CStringA strText;
			strText.Format("�Է���������Ӻ���ʱ��Ҫ��֤,����������ӶԷ���������......\n");
			CString str = CA2W(strText);
			ShowText(str, TRUE);
		}

		SAFE_DELETE(res);
	}
	else
	{
		CStringA strText;
		strText.Format("δ֪����,����´���!\n");
		CString str = CA2W(strText);
		ShowText(str, TRUE);
	}

	bHandled = TRUE;
	return 0;
}

CFriendWnd *CFriendWnd::m_Instance = NULL;

void CFriendWnd::ShowText(CString str, bool bClear)
{
	CControlUI *pControl = m_PaintManager.FindControl(L"log");
	if (pControl)
	{
		if (bClear)
		{
			pControl->SetText(str);
		}
		else
		{
			CString strText = pControl->GetText();
			strText += str;
			pControl->SetText(strText);
		}		
	}
}

void CFriendWnd::InitWindow()
{
	CenterWindow();
	SetTimer(m_hWnd,0,100,NULL);

	CControlUI *pControl = m_PaintManager.FindControl(L"tips");
	if (pControl)
	{
		pControl->SetText(L"**********��Ӻ�����ʹ��v1+v2����**********\n��ʹ���ֻ��š�΢�ź����Ȳ�ѯ��ȡv1/wxid/v2���ݺ������");
	}
}

CFriendWnd * CFriendWnd::GetInstance()
{
	if (!m_Instance)
	{
		m_Instance = new CFriendWnd();
	}

	return m_Instance;
}

void CFriendWnd::Notify(TNotifyUI& msg)
{
	CString strName = msg.pSender->GetName();
	if (msg.sType == DUI_MSGTYPE_CLICK)
	{
		if (strName == L"closebutton")
		{
			ShowWindow(FALSE);
		}
		else if (strName == L"search")
		{
			//������ϵ�˲���
			CControlUI *pControl = m_PaintManager.FindControl(L"username");
			if (pControl)
			{
				CStringA strSearchName = CW2A(pControl->GetText());
				string searchName = CStringA2Utf8(strSearchName);
				SearchContact(searchName.c_str(), searchName.size());
			}
		}
		else if (strName == L"verify")
		{
			//�Ӻ��Ѳ���
			CControlUI *pControl = m_PaintManager.FindControl(L"username");
			if (pControl)
			{
				CControlUI *pControlV2 = m_PaintManager.FindControl(L"v2Name");
				if (pControlV2)
				{
					CStringA strVerifyName = CW2A(pControl->GetText());
					string VerifyName = CStringA2Utf8(strVerifyName);
					CStringA strVerifyV2Name = CW2A(pControlV2->GetText());
					string VerifyV2Name = CStringA2Utf8(strVerifyV2Name);
					string content = CStringA2Utf8("�����~");
					pControl = m_PaintManager.FindControl(L"sayHello");
					if (pControl)
					{
						CStringA strContent = CW2A(pControl->GetText());
						content = CStringA2Utf8(strContent);
					}
					AddNewFriend(VerifyName.c_str(), VerifyName.size(), VerifyV2Name.c_str(), VerifyV2Name.size(), content.c_str(), content.size());
				}				
			}
		}
	}	
}

LRESULT CFriendWnd::MessageHandler(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, bool& /*bHandled*/)
{
	LRESULT lRes = 0;
	return lRes;
}

LRESULT CFriendWnd::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = 0;
	bHandled = FALSE;

	switch (uMsg)
	{
	case WM_TIMER:
		lRes = OnTimer(uMsg, wParam, lParam, bHandled);
		break;
	case WM_FRIEND_SEARCH_CONTACT:
		OnSearchContact(uMsg, wParam, lParam, bHandled);
		break;
	case WM_FRIEND_ADD_NEW_FRIEND:
		OnAddNewFriend(uMsg, wParam, lParam, bHandled);
		break;
	default:
		break;
	}

	return 0;
}