#pragma once


enum WM_LOGIN
{
	//��ʾ��¼���
	WM_SHOW_LOGIN_RESULT = WM_USER + 1000,

	//����ɨ����Ȩҳ��
	WM_LOGIN_SCAN_QRCODE,

	//�������������Ȩ����
	WM_MOBILE_VERIFY,

	//��¼�ɹ�
	WM_LOGIN_SUCC,

	
};

class CLoginWnd : public WindowImplBase
{
public:
	virtual void InitWindow();
	virtual void Notify(TNotifyUI& msg);

	static CLoginWnd *GetInstance();
protected:
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile() { return _T("login.xml"); };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("MicroChat_Login"); };

	//��ʾ��¼ʧ��ԭ��
	void ShowLoginResult(const char *szLog, ...);
private:
	static CLoginWnd *		m_Instance;

	//�����쳣,�����˳�
	void QuitWithMsg(CString strMsg, DWORD dwTimeout = 3 * 1000);

	//ɨ����Ȩ
	LRESULT ScanQrcode(UINT uMsg, LoginResult *res, int error_code, BOOL& bHandled);

	//������Ȩ
	LRESULT MobileVerify(UINT uMsg, MobileVerifyResult *res, int error_code, BOOL& bHandled);
};
#define pLoginWnd (CLoginWnd::GetInstance())


class CLoadingWnd : public WindowImplBase
{
public:
	virtual void InitWindow();


protected:
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile() { return _T("loading.xml"); };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("MicroChat_Loading"); };
};

class CVerifyWnd : public WindowImplBase
{
public:
	virtual void InitWindow();

protected:
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual void Notify(TNotifyUI& msg);

	virtual CDuiString GetSkinFolder() { return _T("skin"); };
	virtual CDuiString GetSkinFile() { return _T("sendMobileCode.xml"); };
	virtual LPCTSTR GetWindowClassName(void) const { return _T("MicroChat_Verify"); };
};