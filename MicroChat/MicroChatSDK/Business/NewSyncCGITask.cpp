#include "NewSyncCGITask.h"
#include <mars/comm/windows/projdef.h>
#include "proto/generate/newsync.pb.h"
#include "business/crypto/mmCryptInterface.h"
#include "AuthInfo.h"
#include "Business/define.h"
#include "Wrapper/NetworkService.h"
#include "ReportSyncKVCGITask.h"
#include "../generate/NewInit.pb.h"
#include "interface.h"


bool NewSyncCGITask::s_bSyncing		= FALSE;
bool NewSyncCGITask::s_bNeedReSync	= FALSE;
CRITICAL_SECTION NewSyncCGITask::s_cs;
bool NewSyncCGITask::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	printf("����ͬ��......\n");
	
	string reqProtobuf = MakeNewSyncReq();

	DWORD dwCompressed = 0;
	string body = nocompress_aes(pAuthInfo->m_Session, reqProtobuf);
	if (!body.size())	return FALSE;

	string header = MakeHeader(BaseHeader::s_cookie, m_nCgiType, reqProtobuf.size(), reqProtobuf.size());

	string req = header + body;

	_outbuffer.AllocWrite(req.size());
	_outbuffer.Write(req.c_str(), req.size());

	return TRUE;
}

int NewSyncCGITask::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	_error_code = CGI_ERR_UNKNOWN;

	m_res = new NewSyncResult;
	NEW_ERR(m_res);

	string body = UnPackHeader(string((const char *)_inbuffer.Ptr(), _inbuffer.Length()));

	if (!body.size())
	{
		LOG("����쳣���밴mmЭ����ȷ��������!\r\n", _inbuffer.Length());
		_error_code = CGI_CODE_UNPACK_ERR;
		return 0;
	}

	string RespProtobuf;

	if (5 == m_nDecryptType)
	{
		if (m_bCompressed)
		{
			RespProtobuf = aes_uncompress(pAuthInfo->m_Session, body, m_nLenRespProtobuf);
		}
		else
		{
			RespProtobuf = aes_nouncompress(pAuthInfo->m_Session, body);
		}

		if (RespProtobuf.size())
		{
			_error_code = CGI_CODE_OK;

			com::tencent::mars::microchat::proto::NewSyncResp resp;
			bool bRet = resp.ParsePartialFromArray(RespProtobuf.c_str(), RespProtobuf.size());

			if (bRet)
			{
				//�������µ�ͬ��key
				pAuthInfo->SetSyncKey(resp.strsynckey());

				//���������Ϣ,ͬ����UI��
				if (resp.newmsg().cnt())
				{
					((NewSyncResult *)m_res)->ppNewMsg = new NewMsg *[resp.newmsg().cnt()];
					((NewSyncResult *)m_res)->ppContanct = new ContactInfo *[resp.newmsg().cnt()];
					((NewSyncResult *)m_res)->dwSize = resp.newmsg().cnt();

					
					for (int i = 0;i<resp.newmsg().cnt();i++)
					{
						//type == 5�Ƿ����������Ϣ,type == 2�Ǻ�����Ϣ,�������Ϣ��֪��ʲô���߰����
						if (5 == resp.newmsg().msg(i).type())
						{
							com::tencent::mars::microchat::proto::NewSyncRespMsg_ msg;
							bool ret = msg.ParsePartialFromArray(resp.newmsg().msg(i).msg().c_str(), resp.newmsg().msg(i).msg().size());
							if (ret)
							{
								//��������Ϣ
								NewMsg *pMsg = new NewMsg;
								if (pMsg)
								{
									strcpy_s(pMsg->szFrom, msg.content().from().id().c_str());
									strcpy_s(pMsg->szTo, msg.content().to().id().c_str());
									strcpy_s(pMsg->szContent, Utf82CStringA(msg.content().rawcontent().content().c_str()));
									pMsg->nType = msg.content().type();
									pMsg->utc = msg.content().createtime();
									pMsg->svrid = msg.content().serverid();

									((NewSyncResult *)m_res)->ppNewMsg[((NewSyncResult *)m_res)->dwNewMsg++] = pMsg;
									
									//���±������ݿ�
									pMicroChatDb->AddMsg(pMsg);
								}								
							}
							else
							{
								_error_code = CGI_CODE_PARSE_PROTOBUF_ERR;
								return 0;
							}							
						}
						else if (2 == resp.newmsg().msg(i).type())
						{
							//ˢ�º����б�
							com::tencent::mars::microchat::proto::commonMsg common;
							bool ret = common.ParsePartialFromArray(resp.newmsg().msg(i).msg().c_str(), resp.newmsg().msg(i).msg().size());

							if (ret)
							{
								com::tencent::mars::microchat::proto::ContactInfo info;
								bool ret = info.ParsePartialFromArray(common.msg().c_str(), common.msg().size());
								if (ret)
								{
									ContactInfo *pInfo = new ContactInfo;
									strcpy_s(pInfo->wxid, info.wxid().id().c_str());
									strcpy_s(pInfo->nickName, info.nickname().name().c_str());
									strcpy_s(pInfo->headicon, info.headicon_small().c_str());
									strcpy_s(pInfo->v1_Name, info.encryptname().c_str());

									((NewSyncResult *)m_res)->ppContanct[((NewSyncResult *)m_res)->dwContanct++] = pInfo;

								}
								else
								{
									_error_code = CGI_CODE_PARSE_PROTOBUF_ERR;
									return 0;
								}
							}							
						}						
					}


					//֪ͨ������ͬ����Ϣ�ɹ�
					ReportSyncKVCGITask::ReportSyncKV();
				}
			}
			else
			{
				_error_code = CGI_CODE_PARSE_PROTOBUF_ERR;
				return 0;
			}
		}
	}
	else
	{
		_error_code = CGI_CODE_DECRYPT_ERR;
		return 0;
	}


	return 0;
}


bool NewSyncCGITask::IsSyncing()
{
	bool bRet = FALSE;

	EnterCriticalSection(&s_cs);

	if (s_bSyncing)
	{	
		//�ȴ���ǰͬ�������������ͬ��һ��
		s_bNeedReSync = TRUE;
		bRet = TRUE;
	}
	else
	{
		//�������ͬ��,����ͬ�������ֹͶ��
		s_bSyncing = TRUE;
	}

	LeaveCriticalSection(&s_cs);

	return bRet;
}

bool NewSyncCGITask::IsNeedReSync()
{
	bool bRet = FALSE;

	EnterCriticalSection(&s_cs);

	if (s_bNeedReSync)
	{
		s_bSyncing = TRUE;			//��ǵ�ǰ����ͬ��,��ֹ�ٴ�Ͷ��ͬ������
		bRet = TRUE;
	}
	else
	{
		s_bSyncing = FALSE;		//��ǰͬ���������,�����ٴ�Ͷ��ͬ����������
	}
	
	s_bNeedReSync = FALSE;		//�����Զ�ͬ����־λ

	LeaveCriticalSection(&s_cs);

	return bRet;
}

std::string NewSyncCGITask::MakeNewSyncReq()
{
	string req;

	com::tencent::mars::microchat::proto::NewSyncRequest newsyncReq;
	com::tencent::mars::microchat::proto::NewSyncRequest::continueFlag_ *pFlag = new com::tencent::mars::microchat::proto::NewSyncRequest::continueFlag_;
	com::tencent::mars::microchat::proto::syncMsgKey_ *pSyncMsgKey = new com::tencent::mars::microchat::proto::syncMsgKey_;
	com::tencent::mars::microchat::proto::syncMsgKey_::MsgKey_ *pMsgKey = new com::tencent::mars::microchat::proto::syncMsgKey_::MsgKey_;
	
	string strSyncKey;
	//����ͬ��key��ʧʱ,ʹ��ȫ0��KV��ʼ��ͬ����Ϣ
	if (!pAuthInfo->GetSyncKey().size())
	{
		//��27����Ϣ��Ҫͬ��
		const int nMsgTypeCnt = 27;
		const int nMsgType[] = { 1,2,3,4,5,7,8,9,10,11,13,14,
			101,102,103,104,105,107,109,111,112,
			201,203,204,205,1000,1001 };
		pMsgKey->set_size(nMsgTypeCnt);
		for (int i = 0; i < nMsgTypeCnt; i++)
		{
			com::tencent::mars::microchat::proto::syncMsgKey_::MsgKey_::Key_ *pKey = pMsgKey->add_key();;
			pKey->set_type(nMsgType[i]);
			pKey->set_synckey(0);
		}

		pSyncMsgKey->set_len(pMsgKey->ByteSize());
		pSyncMsgKey->set_allocated_msgkey(pMsgKey);

		strSyncKey = pSyncMsgKey->SerializePartialAsString();
	}
	else
	{
		strSyncKey = pAuthInfo->GetSyncKey();
	}
	
	
	pFlag->set_flag(0);
	newsyncReq.set_allocated_continueflag(pFlag);
	newsyncReq.set_selector(262151);		//�̶�ֵ
	newsyncReq.set_tagmsgkey(strSyncKey);
	newsyncReq.set_scene(3);
	newsyncReq.set_device(DEVICE_INFO_ANDROID_VER);
	newsyncReq.set_syncmsgdigest(1);
	
	newsyncReq.SerializeToString(&req);

	return req;
}
