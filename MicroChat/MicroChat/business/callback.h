#pragma once

extern char g_wxid[100];
extern DWORD g_dwUin;

//����sdk�ص�����
void SDKCallBack(void *result, int nCgiType, int nTaskId, int nCode);

//��¼���
void LoginCallBack(LoginResult *res);

//������Ȩ�ص�����
void MobileVerifyCallBack(MobileVerifyResult *res);

//�״ε�¼��ʼ��
void NewInitCallBack(NewInitResult *res);

//ͬ����Ϣ�ص�����(��ʼ������Ⱥ���б�/����������Ϣ��
void NewSyncCallBack(NewSyncResult *res);

//�������ѻص�����
void SearchContactCallBack(SearchContactResult *res);

//�Ӻ��ѻص�����
void AddNewFriendCallBack(VerifyUserResult *res);