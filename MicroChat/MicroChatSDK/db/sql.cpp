#include "stdafx.h"
#include "sql.h"
#include "Business\fun.h"

#pragma warning(disable: 4996)

Sql3::Sql3()
{
	m_pDB = NULL;

	InitializeCriticalSection(&m_cs);
	InitializeCriticalSection(&m_cs_sql);

#ifndef USE_SYNC_MODE
	/** @brief ���������̶߳���ˢ�������¼ **/
	m_handle = (HANDLE)_beginthreadex(NULL, NULL, &Sql3::ExecSQLThread, this, NULL, NULL);
#endif
}

Sql3::~Sql3()
{
	Close();

	if (m_handle)
	{
		TerminateThread(m_handle, 0);
		CloseHandle(m_handle);
		m_handle = NULL;
	}
}

void Sql3::Init(LPCSTR strDbNameUtf8)
{
	/** @brief ���ȹر��Ѿ��򿪵����ݿ�  **/
	Close();

	/** @brief ��¼��ǰ�򿪵����ݿ���  **/
    m_strDbNameUtf8 = strDbNameUtf8;

	/** @brief �����ݿ�;�������򴴽��µ����ݿ�  **/
	int nRes = sqlite3_open_v2(m_strDbNameUtf8, &m_pDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if (nRes != SQLITE_OK)
	{
		LOG("[Sql3::Init]Open database fail: %s", sqlite3_errmsg(m_pDB));
		return;
	}
	else
	{
		CStringA strDbNameUtf8 = CW2A(CUTF82W(m_strDbNameUtf8));
		LOG("[Sql3::Init]Open database OK��database path:%s ", strDbNameUtf8);
	}

	/** @brief ���ݿ⽨�� **/
	CreateTable();
}

int Sql3::ExecSQL(LPCSTR strSQLUtf8, sqlite3_callback pCallBack /*= NULL*/, void *pUserArg /*= NULL*/)
{
	if (!m_pDB)		return SQLITE_FAIL;

	int res = SQLITE_OK;

	Lock(m_cs);
	
	CStringA strSqlA = CW2A(CUTF82W(strSQLUtf8));
	LOG("[Sql3::ExecSQL]����ִ��Sql���:%s", strSqlA);

	res = sqlite3_exec(m_pDB, strSQLUtf8, pCallBack, pUserArg, NULL);

	if (res != SQLITE_OK)
	{
		LOG("[Sql3::ExecSQL][Error]sql:%sִ�д���,������Ϣ:%s", strSqlA, sqlite3_errmsg(m_pDB));
	}

	UnLock(m_cs);

	return res;
}

int Sql3::ExecSQL(LPCTSTR strSql)
{
	if (!m_pDB)		return 1;

	int ret = 0;

	Lock(m_cs_sql);
#ifdef USE_SYNC_MODE
	/** @brief ͬ��ִ��Sql���  **/
	ret = ExecSQL(CW2UTF8(strSql));
#else
	m_SqlList.push_back(strSql);
#endif
	UnLock(m_cs_sql);

	return ret;
}

int Sql3::Begin()
{
	if (!m_pDB)		return SQLITE_FAIL;

	int ret = SQLITE_OK;

	Lock(m_cs);

	ret = sqlite3_exec(m_pDB, "begin;", 0, 0, NULL);

	if (ret != SQLITE_OK)
	{
		LOG("[Sql3::Begin]sql��������ʧ��,������Ϣ:%s", sqlite3_errmsg(m_pDB));
	}

	UnLock(m_cs);

	return ret;
}

int Sql3::Commit()
{
	if (!m_pDB)		return SQLITE_FAIL;

	int ret = SQLITE_OK;

	Lock(m_cs);

	ret = sqlite3_exec(m_pDB, "commit;", 0, 0, NULL);

	if (ret != SQLITE_OK)
	{
		LOG("[Sql3::Begin]sql�ύ����ʧ��,������Ϣ:%s", sqlite3_errmsg(m_pDB));
	}

	UnLock(m_cs);

	return ret;
}

unsigned int __stdcall Sql3::ExecSQLThread(void *p)
{
	if (!p)	return -1;

	Sql3 *pSql = static_cast<Sql3*>(p);

	while (1)
	{
		bool bNeedCommit = FALSE;

		pSql->Lock(pSql->m_cs_sql);

		while (pSql->m_SqlList.size())
		{
			if (!bNeedCommit)
			{
				/** @brief ʹ������������ݿ����Ч�� **/
				if (SQLITE_OK == pSql->Begin())
				{
					bNeedCommit = TRUE;
				}
			}

			/** @brief ȡ��һ��Sql��� **/
			CString strSql = pSql->m_SqlList.front();
			pSql->m_SqlList.pop_front();

			pSql->UnLock(pSql->m_cs_sql);

			/** @brief ִ��Sql��� **/
			pSql->ExecSQL(CW2UTF8(strSql));

			Sleep(0);

			pSql->Lock(pSql->m_cs_sql);
		}

		if (bNeedCommit)
		{
			/** @brief ����������д�����ݿ� **/
			pSql->Commit();
		}

		pSql->UnLock(pSql->m_cs_sql);

		Sleep(1);
	}

	return 0;
}

void Sql3::Close()
{
	if (m_pDB)
	{
		/** @brief ���ύһ��  **/
		Commit();

		sqlite3_close(m_pDB);
		m_pDB = NULL;
		m_strDbNameUtf8.Empty();

		/** @brief ���δ���sql���� **/
		Lock(m_cs_sql);
		m_SqlList.clear();
		UnLock(m_cs_sql);
	}
}

CStringA UnicodeToUtf8(CString strUnicode)
{
	CStringA strUtf8;

	int nLen = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);

	WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, strUtf8.GetBuffer(nLen + 1), nLen, NULL, NULL);
	strUtf8.ReleaseBuffer();

	return strUtf8;
}

CString Utf8ToUnicode(CStringA strUtf8)
{
	CString strUnicode;

	int nLen = MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL, 0);

	MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, strUnicode.GetBuffer(nLen + 1), nLen);
	strUnicode.ReleaseBuffer();

	return strUnicode;
}