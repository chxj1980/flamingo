#pragma once
#include <string>
using namespace std;

//��󻺳�������
#define MAX_BUF 65535

//callback������
enum CGI_ERR_CODE {
	
	CGI_CODE_LOGIN_NEED_MOBILE_MSG = -205,			//�豸�״ε�¼��Ҫ������Ȩ
	CGI_CODE_LOGIN_NEED_SCAN_QRCODE = -106,			//�豸�״ε�¼��Ҫɨ����Ȩ
	CGI_CODE_LOGIN_PWD_WRONG = -3,					//�������
	CGI_CODE_LOGIN_FAIL = -1,						//��¼ʧ��(δ֪����)
	CGI_CODE_LOGIN_SUCC = 0,						//��½�ɹ�

	CGI_CODE_OK = 0,								//cgi���óɹ�

	CGI_CODE_NETWORK_ERR = 100001,					//����ͨѶʧ��
	CGI_CODE_DECRYPT_ERR = 100002,					//�������ʧ��
	CGI_CODE_PARSE_PROTOBUF_ERR = 100003,			//protobuf����ʧ��
	CGI_CODE_UNPACK_ERR = 100004,					//�����ͷ����ʧ��
	CGI_CODE_LOGIN_ECDH_ERR = 100005,				//ECDH����ʧ��
	CGI_CODE_LOGIN_SCAN_QRCODE_ERR = 100006,		//ɨ����Ȩ��ַ��ȡʧ��


	CGI_ERR_UNKNOWN = 0x7FFFFFFF,					//δ֪����
};

enum CGI_TYPE {
	CGI_TYPE_INVALID = -1,
	CGI_TYPE_UNKNOWN = 0,

	CGI_TYPE_NEWSYNC = 138,
	CGI_TYPE_NEWINIT = 139,
	CGI_TYPE_GETPROFILE = 302,
	CGI_TYPE_MANUALAUTH = 701,
	CGI_TYPE_NEWSENDMSG = 522,
	CGI_TYPE_SEARCHCONTACT = 106,
	CGI_TYPE_GETCONTACT = 182,
	CGI_TYPE_VERIFYUSER = 30,
	CGI_TYPE_BIND = 145,

	CGI_TYPE_PUSH = 10001,

	CGI_TYPE_MAX = 0xFFFF
};

#define CGI_NEWSYNC			"/cgi-bin/micromsg-bin/newsync"					//ͬ�������������Ϣ
#define CGI_MANUALAUTH		"/cgi-bin/micromsg-bin/manualauth"				//��¼
#define CGI_NEWSENDMSG		"/cgi-bin/micromsg-bin/newsendmsg"				//����������Ϣ
#define CGI_NEWINIT			"/cgi-bin/micromsg-bin/newinit"					//�״ε�¼,��ʼ�����ݿ�
#define	CGI_GETPROFILE		"/cgi-bin/micromsg-bin/getprofile"				//��ȡ������Ϣ
#define	CGI_SEARCHCONTACT	"/cgi-bin/micromsg-bin/searchcontact"			//����������
#define	CGI_GETCONTACT		"/cgi-bin/micromsg-bin/getcontact"				//����������
#define	CGI_VERIFYUSER		"/cgi-bin/micromsg-bin/verifyuser"				//��Ӻ���
#define CGI_BIND			"/cgi-bin/micromsg-bin/bindopmobileforreg"		//�״ε�¼������Ȩ

//
//���ļ��ڽṹ�����ַ���һ��'\0'��β;UI�������SDK�ӿ�SafeFree�ͷ��ڴ�!
//

//
//��չ�������ܽӿ�:
//��д��Ӧ��protobuf�������ɵ�c++�ļ�����
//����cgi type��uri��ַ;��д�ص������������Ҫ���ݵĽṹ��
//��дCGITask��,�ɲο���¼����,��ȷ������������
//��д��ˮ����
//

//��¼���
struct LoginResult
{
	LoginResult() 
	{
		ZeroMemory(szMsg,sizeof(szMsg));
		ZeroMemory(szUrl, sizeof(szUrl));
	}

	char szMsg[4096];			//��ʾ��Ϣ
	char szUrl[4096];			//��Ҫɨ����Ȩʱ���ﴫ��ɨ����֤��ַ;��Ҫ������֤ʱ,���ﴫ��������Ѷ�����֤��ַ
	char szWxid[100] = {0};		//��ǰ��¼�˺�wxid(��¼�ɹ���ŷ���)
	DWORD dwUin = 0;			//��ǰ��¼�˺�uin(��½�ɹ���ŷ���)
};

//������֤���
struct MobileVerifyResult
{
	int  nCode = 0;						//������
	char szMsgResult[1024] = { 0 };		//������Ϣ
	int  nOption = 10;					//������,10==������֤��,11==������֤��,13==�����豸�Ƿ���Ҫ��(�������ò���,�豸�״ε�¼�ɹ�����ʾ��Ȩ��)
};

//������Ϣ
struct ContactInfo
{
	ContactInfo()
	{
		ZeroMemory(wxid, sizeof(wxid));
		ZeroMemory(nickName, sizeof(nickName));
		ZeroMemory(headicon, sizeof(headicon));
		ZeroMemory(v1_Name, sizeof(v1_Name));
		ZeroMemory(v2_Name, sizeof(v2_Name));
	}
	
	char	wxid[100];
	char	nickName[100];
	char	headicon[200];		//ͷ��url
	char	v1_Name[200];		//v1����
	char	v2_Name[200];		//v2����
};

//δ����Ϣ
struct NewMsg
{
	NewMsg()
	{
		svrid = 0;
		utc = 0;
		nType = 0;
		ZeroMemory(szFrom, sizeof(szFrom));
		ZeroMemory(szTo, sizeof(szTo));
		ZeroMemory(szContent, sizeof(szContent));
	}
	UINT64  svrid;				//ÿ����Ϣ�ڷ����Ψһ��ʶ
	char	szFrom[1024];		//����Ϣ��
	char	szTo[1024];			//����Ϣ��
	int		nType;				//��Ϣ����
	DWORD	utc;				//��Ϣ����ʱ��
	char szContent[MAX_BUF];	//��Ϣ����(�п�����xml��ʽ,����type�ж�,��ҪUI�����н���)
};

//newinit callback
struct NewInitResult
{
	NewInitResult()
	{
		dwContanct = 0;
		dwNewMsg = 0;
		dwSize = 0;
		ppNewMsg = NULL;
		ppContanct = NULL;
		
	}

	ContactInfo **ppContanct;		//������Ϣ
	DWORD		dwContanct;			//������Ϣ����
	NewMsg		**ppNewMsg;			//����Ϣ
	DWORD		dwNewMsg;			//����Ϣ����

	DWORD		dwSize;				//����ָ�������С(�ͷ��ڴ�ʱʹ��)
};

//newsync callback
struct NewSyncResult
{
	NewSyncResult()
	{
		dwNewMsg		= 0;
		dwContanct		= 0;
		dwSize = 0;
		ppNewMsg		= NULL;
		ppContanct		= NULL;
	}
	
	NewMsg **ppNewMsg;			//����Ϣ
	DWORD dwNewMsg;				//����Ϣ����
	ContactInfo **ppContanct;	//����
	DWORD dwContanct;			//��������

	DWORD		dwSize;			//����ָ�������С(�ͷ��ڴ�ʱʹ��)
};

//searchcontact callback
struct SearchContactResult
{
	int  nCode = 0;						//������
	char szMsgResult[1024] = { 0 };		//��ѯ���
	char szNickName[100] = { 0 };		//�ǳ�
	char szV1_Name[200] = { 0 };		//v1����
	char szV2_Name[200] = { 0 };		//v2����
	char szHeadIcon[200] = { 0 };		//ͷ��
};

//�Ӻ������� callback
struct VerifyUserResult
{
	int  nCode = 0;						//������
	char szMsgResult[1024] = { 0 };		//������Ϣ
};

//cgi����ص�����,���������֪ͨ�ϲ���
typedef void(*CGICallBack)(void *result,int nCgiType,int nTaskId,int nCode);

//ע��cgi�ص�����
void RegisterCgiCallBack(CGICallBack callback);

//����sdk
void StartSDK();

//ͣ��sdk
void StopSDK();

//�˺������¼,����taskid(bRandomDeivce������ʾ�Ƿ�ʹ������豸��¼:�״ε�¼��Ҫ���Ż�ɨ����֤,Ĭ��ʹ�ù̶��豸��¼)
//�״ε�¼�ɹ���,ʹ��NewInit�ӿڳ�ʼ�������б�,��ȡδ����Ϣ,ˢ�²�����ͬ��key,������ͻ���ͬ��key��������
//������¼���db�м��غ����б���ʷ��Ϣ��ͬ��key,ֱ��ʹ��newsync�ӿ�ͬ����Ϣ
//TODO:bRandomDeivce ���Ӳ����Ϣ��δʵ��
int Login(const char *user, int nLenUser, const char *pwd, int nLenPwd,bool bRandomDeivce = FALSE);

//���ն�����֤��(Ĭ��ʹ�õ�¼�˺���Ϊ�ֻ�����)
void RecvMobileVerifyCode();

//���Ͷ�����Ȩ��֤��
int SendMobileVerifyCode(const char *code, int nLenCode);


//���Դ�DB������Ϣ,���ͨ��newinit�ص����ϲ�
//���޷���db�ж�ȡͬ��key,�����newinit��ʼ��,����ȡ��synckeyִ��ͬ��
void LoadFromDB();

//�豸��һ�ε�¼��ʼ��(�����б�,��ʷ��Ϣ,syncKey)
//����ҪUI����������,LoadFromDB�л�����Զ���Ҫ����
int NewInit();

//����������Ϣ(content����Utf8��ʽ,������������)(֧��С����:��ʽΪ"[����]")
void NewSendMsg(const char *content, int nLenContent, const char *toWxid,int nLenToWxid);

//ͬ����������Ϣ(Ĭ��ʹ�ó��������ͻ�ȡ��Ϣ;��¼�����һ��ˢ��ͬ��key)
void NewSync();

//�˺Ų�ѯ:76λv1����,v2����,�ֻ���,΢�ź�����,�ݲ�֧��wxid��108λv1����
void SearchContact(const char *szName, int nLenName);

//�Ӻ���(��ʧЧ,������:����˸�����Э��,��Ҫ��v2���ݶԷ��Ż��յ�����;�Ӻ���ʱ�ᷢ���豸Ӳ����Ϣ)
void AddNewFriend(const char *szUserName, int nLenUserName, const char *szV2Name, int nLenV2Name,const char *szContent, int nLenContent);

//ȷ���豸�Ƿ�����Ȩ(�����ýӿ�,ֱ�ӵ�¼����)
//void IsNeedVerify(const char *MobileNum, int nLenMobileNum);

//��ȫ�ͷ��ڴ�(�ṹ����ָ����Ҫ�����ͷ�)
void SafeFree(void *p);
#define SAFE_DELETE(p) {SafeFree((void *)p);p=NULL;}

