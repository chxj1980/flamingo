#include "StdAfx.h"
#include "MainWnd.h"
#include "friendWnd.h"
#include "interface.h"
#include <shellapi.h>
#include "business\callback.h"


typedef int (WINAPI *MessageBoxTimeoutW)(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
static MessageBoxTimeoutW MessageBoxTimeout = (MessageBoxTimeoutW)GetProcAddress(GetModuleHandle(L"user32.dll"),"MessageBoxTimeoutW");


CMainWnd *CMainWnd::m_Instance = NULL;

DUI_BEGIN_MESSAGE_MAP(CMainWnd, WindowImplBase)
DUI_ON_MSGTYPE(DUI_MSGTYPE_SELECTCHANGED, OnSelectChanged)
DUI_ON_MSGTYPE(DUI_MSGTYPE_ITEMCLICK, OnItemClick)
DUI_END_MESSAGE_MAP()

void CMainWnd::QuitWithMsg(CString strMsg, DWORD dwTimeout)
{
	if (MessageBoxTimeout)
	{
		MessageBoxTimeout(GetHWND(), strMsg, L"Error", NULL, 0, dwTimeout);
	}
	else
	{
		MessageBox(GetHWND(), strMsg, L"Error", NULL);
	}
	
	TerminateProcess(GetCurrentProcess(),0);
}

#define FIND_CONTROL(x)				(m_PaintManager.FindControl(x))
#define FIND_SUBCONTROL(x,y)		(m_PaintManager.FindSubControlByName(x,y))

void FriendWndThread()
{
	pFriendWnd->Create(NULL, _T("testFriend"), UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
	pFriendWnd->ShowModal();
}

void CMainWnd::InitWindow()
{
	SetWindowText(m_hWnd,L"΢��");
	CenterWindow();

	//��ʼ��
	LoadFromDB();

	//��ʼ�����Դ���
	CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)FriendWndThread,NULL, NULL, NULL));
}

void CMainWnd::Notify(TNotifyUI& msg)
{
	if (msg.sType == DUI_MSGTYPE_CLICK)
	{
		if (_tcsicmp(msg.pSender->GetName(), _T("main_close_btn")) == 0)
		{
			Close();
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("main_mini_btn")) == 0)
		{
			SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("testFriend")) == 0)
		{
			pFriendWnd->ShowWindow(SW_SHOW);
			SetForegroundWindow(pFriendWnd->GetHWND());
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("send")) == 0)
		{
			CRichEditUI *pInput = (CRichEditUI *)FIND_CONTROL(L"inputEdit");
			if (pInput)
			{
				CDuiString strInput = pInput->GetText();

				if (!strInput.IsEmpty())
				{
					if (m_pCurSelContactControlUI)
					{
						CListContainerElementUI *pNode = NewOneChatListNodeMe();
						if (pNode)
						{
							CRichEditUI *pEdit = (CRichEditUI *)FIND_SUBCONTROL(pNode, L"content");
							if (pEdit)
							{
								pEdit->SetText(strInput);

								RECT rcPadding = pEdit->GetTextPadding();
								SIZE szMax;
								szMax.cx = 400 - rcPadding.left - rcPadding.right;
								szMax.cy = 9999;

								SIZE RealSize;
								if (pEdit->GetNaturalSize(szMax, RealSize))
								{
									SIZE szReal;
									szReal.cx = RealSize.cx + rcPadding.left + rcPadding.right;
									szReal.cy = RealSize.cy + rcPadding.top + rcPadding.bottom;

									pEdit->SetFixedWidth(szReal.cx);
									if (szReal.cy < 60)
									{
										//������Ϣ
										szReal.cy = 60;
									}
									pEdit->SetFixedHeight(szReal.cy);
									pNode->SetFixedHeight(szReal.cy + 18 + 10 + 3);
								}
							}
						}

						CControlUI *pControl = FIND_SUBCONTROL(pNode,L"wxid");
						if (pControl)
						{
							CString strShow;
							strShow.Format(L"%S %S", Utc2BeijingTime(time(NULL)), g_wxid);
							pControl->SetText(strShow);
						}						

						CNodeInfo *pNodeinfo = (CNodeInfo *)m_pCurSelContactControlUI->GetTag();
						if (pNodeinfo)
						{
							CStringA strInputUtf8 = CStringA2Utf8(strInput.GetData());
							CStringA strSendto = pNodeinfo->contactinfo.wxid;
							
							//������Ϣ
							NewSendMsg(strInputUtf8, strInputUtf8.GetLength(), strSendto, strSendto.GetLength());

							//���淢����Ϣ��¼
							pNodeinfo->msgRecordList.push_back(pNode);

							//�Ѹ���ϵ������List��һλ
							CListUI *pList = (CListUI *)m_PaintManager.FindControl(L"main_list");
							if (pList)
							{
								pList->SetItemIndex(m_pCurSelContactControlUI, 0);
							}

							//����Ϣ��ǰ8���ַ���ʾ����ϵ���б���
							CControlUI *pLabel = (CLabelUI *)FIND_SUBCONTROL(m_pCurSelContactControlUI, L"history_msg");
							if (pLabel)
							{
								pLabel->SetText(strInput);
							}

							//ˢ����Ϣʱ��
							pLabel = (CLabelUI *)FIND_SUBCONTROL(m_pCurSelContactControlUI, L"time");
							if (pLabel)
							{
								CString str;

								//��ʾʱ����
								time_t utc = time(NULL);
								struct tm *tmdate = localtime(&utc);
								str.Format(L"%02d:%02d:%02d", tmdate->tm_hour, tmdate->tm_min, tmdate->tm_sec);

								pLabel->SetText(str);
							}

							//����ˢ��UI
							UpdateRecord();
						}						
					}
					
					//������봰��
					pInput->SetText(L"");
				}
			}
		}
	}
	else if (msg.sType == DUI_MSGTYPE_RETURN)
	{
		if (msg.pSender->GetName() == L"inputEdit")
		{
			//������Ϣ
			CControlUI *pControl = m_PaintManager.FindControl(L"send");
			if (pControl)
			{
				pControl->Activate();
			}
		}
	}

	__super::Notify(msg);
}

CMainWnd* CMainWnd::GetInstance()
{
	if (!m_Instance)
	{
		m_Instance = new CMainWnd;
	}

	return m_Instance;
}

LRESULT CMainWnd::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = 0;
	bHandled = FALSE;

	switch (uMsg)
	{
	case WM_MAIN_ADD_FRIEND:
	{
		CListUI *pList = (CListUI *)m_PaintManager.FindControl(L"main_list");
		if (pList)
		{
			if (wParam)
			{
				ContactInfo *pInfo = (ContactInfo *)wParam;
				
				//������ʾ���ú��������ظ����
				CListContainerElementUI *pNode = NewOneFrientListNode(pInfo);
				if (pNode)
				{
					//�����б���
					CControlUI *pControl = new CControlUI;
					pControl->SetFixedHeight(15);
					pList->Add(pControl);					
					
					//��ʾ�ú���
					pList->Add(pNode);

					//���غ����б�ͷ��
					CDuiString strUrl = CA2W(pInfo->headicon);
					CDuiString strFileName;
					strFileName.Format(L".\\skin\\head\\%s.png", CA2W(pInfo->wxid));
					CreateDirectory(L".\\skin\\head", NULL);

					HRESULT hr = URLDownloadToFile(0, strUrl, strFileName, 0, NULL);
					if (S_OK == hr)
					{
						CLabelUI *pLabel = (CLabelUI *)FIND_SUBCONTROL(pNode, L"icon");
						if (pLabel)
						{
							CDuiString strFileName;
							strFileName.Format(L"head/%s.png", CA2W(pInfo->wxid));
							pLabel->SetBkImage(strFileName);
						}
					}
				}				
			}
		}

		break;
	}
	case WM_MAIN_ADD_NEW_MSG:
	{
		if (wParam)
		{
			NewMsg *pMsg = (NewMsg *)wParam;
			if (pMsg)
			{
				//Ѱ�Ҷ�Ӧ��ϵ��(�Ҳ����Ͳ���ʾ��,SDK�ѽ���Ϣ����DB,������ϵ��ʱ�����뱾����Ϣ��)
				CControlUI *pControl = m_PaintManager.FindControl(CA2W(pMsg->szFrom));
				if (pControl)
				{
					CNodeInfo *pNodeinfo = (CNodeInfo *)pControl->GetTag();
					if (pNodeinfo)
					{
						//����Ϣ���浽��Ӧ��ϵ��UI��
						CListContainerElementUI *pNode = NewOneChatListNodeOther();
						if (pNode)
						{
							CString strFrom = CA2W(pMsg->szFrom);
							CString strContent = CA2W(pMsg->szContent);

							//Ⱥ����ϢҪ���˷���Ϣ��wxid
							if (-1 != strFrom.Find(L"@chatroom"))
							{
								if (-1 != strContent.Find(L":"))
								{
									//ð�ź����и����з�
									strContent = strContent.Mid(strContent.Find(L":") + 1 + 1);
								}
							}
							
							CRichEditUI *pEdit = (CRichEditUI *)FIND_SUBCONTROL(pNode, L"content");
							if (pEdit)
							{
								//������Ϣ�߶��Զ�����UI�߶�
								//���۵�DUILIB:Label�ؼ������޷�ѡ��,Edit�ؼ�ֻ�ܵ�����ʾ,RichEdit�޷�����Ӧ�߶�
								pEdit->SetText(strContent);
								
								RECT rcPadding = pEdit->GetTextPadding();
								SIZE szMax;
								szMax.cx = 400 - rcPadding.left - rcPadding.right;
								szMax.cy = 9999;

								SIZE RealSize;
								if (pEdit->GetNaturalSize(szMax, RealSize))
								{
									SIZE szReal;
									szReal.cx = RealSize.cx + rcPadding.left + rcPadding.right;
									szReal.cy = RealSize.cy + rcPadding.top + rcPadding.bottom;

									pEdit->SetFixedWidth(szReal.cx);
									if (szReal.cy < 60)
									{
										//������Ϣ
										szReal.cy = 60;
									}
									pEdit->SetFixedHeight(szReal.cy);
									pNode->SetFixedHeight(szReal.cy + 18 + 10 + 3);
								}
								
								//��ʾ˵����wxid
								pControl = FIND_SUBCONTROL(pNode, L"wxid");
								if (pControl)
								{
									CString strWxid;
									if (-1 != strFrom.Find(L"@chatroom"))
									{
										//Ⱥ����Ϣ ˵����wxidҪ��content�н���
										CString strContent = CA2W(pMsg->szContent);
										if (-1 != strContent.Find(L":"))
										{
											strWxid = strContent.Left(strContent.Find(L":"));
										}
									}
									else
									{
										strWxid = strFrom;
									}
									CString strShow;
									strShow.Format(L"%S %s", Utc2BeijingTime(pMsg->utc), strWxid);
									pControl->SetText(strShow);
								}
							}

							//������Ϣ��¼
							pNodeinfo->msgRecordList.push_back(pNode);

							//�Ѹ���ϵ������List��һλ
							pControl = m_PaintManager.FindControl(CA2W(pMsg->szFrom));
							if (pControl)
							{
								CListUI *pList = (CListUI *)m_PaintManager.FindControl(L"main_list");
								if (pList)
								{
									pList->SetItemIndex(pControl,0);
								}
							}

							//����Ϣ��ǰ8���ַ���ʾ����ϵ���б���
							CControlUI *pLabel = (CLabelUI *)FIND_SUBCONTROL(pControl, L"history_msg");
							if (pLabel)
							{
								pLabel->SetText(strContent);
							}

							//ˢ����Ϣʱ��
							pLabel = (CLabelUI *)FIND_SUBCONTROL(pControl, L"time");
							if (pLabel)
							{
								CString str;
								
								//��Ϣ����ʱ��
								CStringA strTimeMsg = Utc2BeijingTime(pMsg->utc);

								//ȡ��ǰʱ��
								CStringA strTimeNow = Utc2BeijingTime(time(NULL));
								
								if (strTimeMsg.Left(10) == strTimeNow.Left(10))
								{
									//������Ϣ,��ʾʱ����
									time_t utc = pMsg->utc;
									struct tm *tmdate = localtime(&utc);
									str.Format(L"%02d:%02d:%02d", tmdate->tm_hour, tmdate->tm_min,tmdate->tm_sec);
								}
								else
								{
									//�ǵ�����Ϣ,��ʾ������
									time_t utc = pMsg->utc;
									struct tm *tmdate = localtime(&utc);
									str.Format(L"%02d/%02d/%02d", (tmdate->tm_year + 1900) % 1000, tmdate->tm_mon + 1, tmdate->tm_mday);
								}

								pLabel->SetText(str);
							}
							
							//����ˢ��UI
							if (pControl == m_pCurSelContactControlUI)
							{
								UpdateRecord();
							}							
						}
					}
				}
			}
		}

		break;
	}
	case WM_MAIN_FORCE_QUIT:
	{
		QuitWithMsg(L"�������δ֪����,�����˳�......");
		break;
	}
	case WM_MAIN_ERR:
	{
		MessageBox(m_hWnd,L"����ͨ���쳣,�����˳�!",NULL,NULL);
		Close();
		break;
	}
	default:
		break;
	}


	return 0;
}

void CMainWnd::OnSelectChanged(TNotifyUI &msg)
{

}

void CMainWnd::OnItemClick(TNotifyUI &msg)
{
	//������Ŀؼ��а���nickname�ӿؼ�˵�������������ϵ���б�ؼ�
	CLabelUI *p = (CLabelUI *)FIND_SUBCONTROL(msg.pSender, L"nickname");
	if (p)
	{
		//�л���ǰѡ��,��Ҫˢ�����촰��UI
		if (m_pCurSelContactControlUI != msg.pSender)
		{
			//�������촰�����쵱ǰѡ��
			m_pCurSelContactControlUI = (CListContainerElementUI *)msg.pSender;

			//�޸����촰�ڱ���
			CLabelUI *pLabel = (CLabelUI *)FIND_CONTROL(L"main_friendNickName");
			if (pLabel)
			{
				pLabel->SetText(p->GetText());
			}

			//ˢ�������¼
			UpdateRecord();
		}		
	}	
}

CListContainerElementUI* CMainWnd::NewOneFrientListNode(ContactInfo *pInfo)
{
	static DWORD s_dwCnt = 0;

	if (!pInfo) return NULL;

	//���ؼ��Ѵ���,��ֱ�ӷ���NULL
	CListContainerElementUI* pNode = (CListContainerElementUI*)m_PaintManager.FindControl(CA2W(pInfo->wxid));
	if (pNode)
		return NULL;

	CDialogBuilder builder;
	pNode = static_cast<CListContainerElementUI*>(builder.Create(_T("listNode.xml"), (UINT)0, NULL, &m_PaintManager));

	if (pNode)
	{
		m_PaintManager.InitControls(pNode);
	}

	CLabelUI *pLabel = (CLabelUI *)FIND_SUBCONTROL(pNode, L"nickname");
	if (pLabel)
	{
		if (pInfo)
		{
			CString strText = CA2W(Utf82CStringA(pInfo->nickName));
			if (strText.IsEmpty())
			{
				pLabel->SetText(CA2W(pInfo->wxid));
			}
			else
			{
				pLabel->SetText(strText);
			}

			//ʹ��wxid����Item,�����ѯ
			pNode->SetName(CA2W(pInfo->wxid));

			//Ϊ����ϵ�˷���һ���ṹ��,��������ϵ����Ϣ
			CNodeInfo *pNodeinfo = new CNodeInfo;
			if (pNodeinfo)
				memcpy(&pNodeinfo->contactinfo, pInfo,sizeof(ContactInfo));
			pNode->SetTag((UINT_PTR)pNodeinfo);
		}
	}

	return pNode;
}

CListContainerElementUI* CMainWnd::NewOneChatListNodeMe()
{
	CDialogBuilder builder;
	CListContainerElementUI* pNode = static_cast<CListContainerElementUI*>(builder.Create(_T("chatlistNodeMe.xml"), (UINT)0, NULL, &m_PaintManager));

	if (pNode)
	{
		m_PaintManager.InitControls(pNode);
	}

	return pNode;
}

CListContainerElementUI* CMainWnd::NewOneChatListNodeOther()
{
	CDialogBuilder builder;
	CListContainerElementUI* pNode = static_cast<CListContainerElementUI*>(builder.Create(_T("chatlistNodeOther.xml"), (UINT)0, NULL, &m_PaintManager));

	if (pNode)
	{
		m_PaintManager.InitControls(pNode);
	}

	return pNode;
}

void CMainWnd::UpdateRecord()
{
	if (!m_pCurSelContactControlUI)	return;

	//ˢ�������¼
	CListUI *pList = (CListUI *)FIND_CONTROL(L"chat_list");
	{
		if (pList)
		{
			CNodeInfo *pNodeinfo = (CNodeInfo *)m_pCurSelContactControlUI->GetTag();
			if (pNodeinfo)
			{
				//����б�ʱ��Ҫ�Զ��ͷ��ڴ�
				pList->GetList()->SetAutoDestroy(FALSE);
				//����б�
				pList->RemoveAll();

				for (list<CListContainerElementUI *>::iterator iter = pNodeinfo->msgRecordList.begin(); iter != pNodeinfo->msgRecordList.end(); iter++)
				{
					CListContainerElementUI *pNode = *iter;

					//��ʾ������Ϣ
					pList->Add(pNode);
				}
			}
		}
	}
}
