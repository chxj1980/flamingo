#pragma once
#include "sqlite3.h"
#include <atlstr.h>
#include <list>
#include <windows.h>

using namespace std;

/** @brief ʹ��ͬ��ģʽ(Ч�ʽϵ�,�����Ա�֤ʱ��) **/
//#define USE_SYNC_MODE

class Sql3
{
public:
	Sql3();
	~Sql3();

	/** @brief �Ƕ�ռ�����ݿ�,���������򴴽����ݿ�ͱ� **/
	void Init(LPCSTR strDbNameUtf8);

	/** @brief �ر����ݿ�,���Sql���� **/
	void Close();

	/** @brief ���ݿ����:����ɾ���ġ��� **/
	int ExecSQL(LPCSTR strSQLUtf8, sqlite3_callback pCallBack = NULL, void *pUserArg = NULL);

	/** @brief �첽ִ��Sql��� **/
	int ExecSQL(LPCTSTR strSql);

	/** @brief ��������(��߲��������ٶ�) **/
	int Begin();

	/** @brief �ر�����(������IO����,�������쳣��֮ǰ���в���ʧ��!) **/
	int Commit();

	/** @brief ���ݿ⽨�� **/
	virtual int CreateTable() = 0;

	list<CString>					m_SqlList;		/**< Sql������  >**/
	CRITICAL_SECTION				m_cs_sql;		/**< Sql������  >**/

	/** @brief �첽ִ��Sql�߳� **/
	static unsigned int __stdcall ExecSQLThread(void *pSql);
	
	void Lock(CRITICAL_SECTION &cs)		{ EnterCriticalSection(&cs); }
	void UnLock(CRITICAL_SECTION &cs)	{ LeaveCriticalSection(&cs); }

	CRITICAL_SECTION	m_cs;			/**< ���ݿ������ >**/

private:
	CStringA			m_strDbNameUtf8;				/**< ��ǰ���������ݿ��ļ���(UTF8��ʽ) >**/
	sqlite3*			m_pDB		= NULL;				/**< ��ǰ���������ݿ����ָ�� >**/
	HANDLE				m_handle	= NULL;				/**< ���������߳� >**/
};

/*
====================================================================================
====================================================================================
*/

CStringA UnicodeToUtf8(CString strUnicode);
CString Utf8ToUnicode(CStringA strUtf8);

#define SQL3REPLACE(strSql3)	strSql3.Replace(L"\'",L"\'\'")		/**< Sql3����е�����Ҫת��Ϊ���������Ŵ��� >**/
#define CW2UTF8(strUnicode)		UnicodeToUtf8(strUnicode)
#define CUTF82W(strUtf8)		Utf8ToUnicode(strUtf8)