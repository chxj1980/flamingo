#pragma once

enum WM_MAIN
{	
	//���������б�
	WM_MAIN_ADD_FRIEND = WM_USER + 0xFFFF,
	
	//��������Ϣ
	WM_MAIN_ADD_NEW_MSG,

	//����ͨ�Ŵ���
	WM_MAIN_ERR,

	//�����˳�
	WM_MAIN_FORCE_QUIT,
};

struct ContactInfo;

//��ϵ��List�ڵ��д洢���������
struct CNodeInfo
{
	//�����¼
	list<CListContainerElementUI*>	msgRecordList;

	//��ϵ����ϸ��Ϣ
	ContactInfo contactinfo;
};

class CMainWnd : public WindowImplBase
{
public:
	virtual void InitWindow();
	virtual void Notify(TNotifyUI& msg);

	static CMainWnd* GetInstance();
protected:
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual CDuiString GetSkinFolder() { return _T("skin"); }
	virtual CDuiString GetSkinFile() { return _T("main.xml"); }
	virtual LPCTSTR GetWindowClassName(void) const { return _T("MicroChat_Main"); }

	DUI_DECLARE_MESSAGE_MAP()
	virtual void OnSelectChanged(TNotifyUI &msg);
	virtual void OnItemClick(TNotifyUI &msg);

	//��ǰ���촰�ڵĶ����Node�ڵ�
	CListContainerElementUI *m_pCurSelContactControlUI = NULL;

private:
	CListContainerElementUI* NewOneFrientListNode(ContactInfo *pInfo);
	CListContainerElementUI* NewOneChatListNodeMe();
	CListContainerElementUI* NewOneChatListNodeOther();

	//����ˢ�������¼����
	void UpdateRecord();

	
	static CMainWnd *		m_Instance;

	//�����쳣,�����˳�
	void QuitWithMsg(CString strMsg,DWORD dwTimeout = 3*1000);

};
#define pMainWnd (CMainWnd::GetInstance())