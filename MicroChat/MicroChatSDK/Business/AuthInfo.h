#pragma once
#include <string>
#include "db/db.h"

class CAuthInfo
{
public:
	CAuthInfo()
	{
		InitializeCriticalSection(&m_cs_syncKey);
	}

	string	m_UserName;
	string	m_WxId;
	DWORD   m_uin = 0;
	string	m_Alias;
	string	m_Session;	
	DWORD   m_ClientVersion;
	string  m_guid_15;
	string  m_guid;
	string  m_androidVer;
	string  m_launguage;
	string  m_cookie;

	string GetSyncKey();
	void SetSyncKey(string strSyncKey);


	static CAuthInfo *GetInstance();

	//��ȡ������֤��ƾ��
	string m_mobilecode_authticket;
	//���ܶ��ź���(��ǰĬ��ʹ�õ�¼�˺�)
	string m_mobileNum;

private:
	static CAuthInfo * m_Instance;

	CRITICAL_SECTION   m_cs_syncKey;
};

#define pAuthInfo (CAuthInfo::GetInstance())