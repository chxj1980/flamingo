#pragma once
#include <time.h>
#include "sql.h"
#include "interface.h"


/** @brief synckey���ֶ�  **/
enum DB_INDEX_SYNCKEY
{
	DB_INDEX_SYNCKEY_KEY = 0,				/**< synckey  >**/
	DB_INDEX_RECORD_CNT,					/**< synckey���ֶ�����  >**/
};

/** @brief contact���ֶ�  **/
enum DB_INDEX_CONTACT
{
	DB_INDEX_CONTACT_WXID = 0,				/**< wxid  >**/
	DB_INDEX_CONTACT_NICKNAME,				/**< nickname  >**/
	DB_INDEX_CONTACT_HEADICON_URL,			/**< ͷ��url  >**/
	DB_INDEX_CONTACT_V1_NAME,				/**< V1����  >**/

	DB_INDEX_CONTACT_CNT,					/**< contact���ֶ�����  >**/
};

/** @brief msg���ֶ�  **/
enum DB_INDEX_MSG
{
	DB_INDEX_MSG_SVRID = 0,				/**< svrid  >**/
	DB_INDEX_MSG_UTC,					/**< utc >**/
	DB_INDEX_MSG_CREATETIME,			/**< ��Ϣ�����ߴ���ʱ�� >**/	
	DB_INDEX_MSG_TYPE,					/**< ��Ϣ���� >**/
	DB_INDEX_MSG_FROM,				    /**< ����Ϣ��wxid  >**/
	DB_INDEX_MSG_TO,				    /**< ����Ϣ��wxid  >**/
	DB_INDEX_MSG_CONTENT,				/**< ��Ϣ����  >**/

	DB_INDEX_MSG_CNT,					/**< contact���ֶ�����  >**/
};

/** @brief DownloadManager���ݿ� **/
class CMicroChatDB : public Sql3
{
public:
	/** @brief ���� **/
	virtual int CreateTable();

	//������ݿ�
	void ClearDB();

	//��ȡͬ��key
	string GetSyncKey();

	//ˢ��ͬ��key
	void UpdateSyncKey(string strSyncKey);

	//������ϵ��
	void LoadContact();

	//������ʷ��Ϣ
	void LoadMsgRecord();

	//��������Ϣ
	void AddMsg(NewMsg *pMsg);

	static CMicroChatDB *GetInstance();
private:
	//���ݿ��ѯ�ص�����
	static int GetSyncKeyCallBack(void *lpUserArg, int argc, char **argv, char **lpszValue);

	//��ϵ�˲�ѯ�ص�����
	static int LoadContactCallBack(void *lpUserArg, int argc, char **argv, char **lpszValue);

	//��ѯ��Ϣ��¼�ص�����
	static int LoadMsgRecordCallBack(void *lpUserArg, int argc, char **argv, char **lpszValue);

	static CMicroChatDB *m_Instance;
};
#define pMicroChatDb	(CMicroChatDB::GetInstance())   

