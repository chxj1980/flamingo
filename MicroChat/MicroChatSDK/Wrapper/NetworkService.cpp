// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
*  NetworkService.cpp
*
*  Created on: 2017-7-7
*      Author: chenzihao
*/


#include "NetworkService.h"
#include "mars/comm/windows/projdef.h"
#include "mars/boost/bind.hpp"
#include "mars/baseevent/base_logic.h"
#include "mars/stn/stn_logic.h"
#include "PublicComponentV2/stnproto_logic.h"
#include "PublicComponentV2/stn_callback.h"
#include "PublicComponentV2/app_callback.h"
#include "Business/define.h"
#include "../interface.h"
#include "mars/stn/stn.h"
#include "Business/AuthInfo.h"
#include <windows.h>
#include "Business/NewSyncCGITask.h"
using namespace std;

NetworkService& NetworkService::Instance()
{
	static NetworkService instance_;
	return instance_;
}

NetworkService::NetworkService()
{
	__Init();
}
NetworkService::~NetworkService()
{
}

void NetworkService::setClientVersion(uint32_t _client_version)
{
	mars::stn::SetClientVersion(_client_version);
}
void NetworkService::setShortLinkDebugIP(const std::string& _ip, unsigned short _port)
{
	mars::stn::SetShortlinkSvrAddr(_port, _ip);
}
void NetworkService::setShortLinkPort(unsigned short _port)
{
	mars::stn::SetShortlinkSvrAddr(_port, "");
}
void NetworkService::setLongLinkAddress(const std::string& _ip, vector<uint16_t> &ports, const std::string& _debug_ip)
{
	mars::stn::SetLonglinkSvrAddr(_ip, ports, _debug_ip);
}

void NetworkService::setCgiCallBack(CGICallBack callback)
{
	m_cgiCallBack = callback;
}

void NetworkService::DoCallBack(void *result, int nCgiType, int nTaskId, int nCode)
{
	if (m_cgiCallBack)
	{
		m_cgiCallBack(result, nCgiType,nTaskId,nCode);
	}
}

void NetworkService::start()
{
	mars::baseevent::OnForeground(true);
	mars::stn::MakesureLonglinkConnected();

	m_shortlinkHost = SHORTLINK_HOST;
}

void NetworkService::stop()
{
	__Destroy();
}

void NetworkService::__Init()
{
	InitializeCriticalSection(&m_cs);

	mars::stn::SetCallback(mars::stn::StnCallBack::Instance());
	mars::app::SetCallback(mars::app::AppCallBack::Instance());
	mars::baseevent::OnCreate();
}

void NetworkService::__Destroy()
{
	mars::baseevent::OnDestroy();
}

int NetworkService::startTask(CGITask* task, bool bSendOnly)
{
	mars::stn::Task ctask;
	ctask.cmdid = task->cmdid_;
	ctask.channel_select = task->channel_select_;
	ctask.shortlink_host_list.push_back(task->host_);
	ctask.cgi = task->cgi_;
	ctask.user_context = (void*)task;
	ctask.send_only = bSendOnly;

	EnterCriticalSection(&m_cs);
	map_task_[ctask.taskid] = task;
	LeaveCriticalSection(&m_cs);

	mars::stn::StartTask(ctask);
	
	return ctask.taskid;
}

bool NetworkService::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	bool bRet = FALSE;

	EnterCriticalSection(&m_cs);
	auto it = map_task_.find(_taskid);
	if (it != map_task_.end())
	{
		bRet = it->second->Req2Buf(_taskid, _user_context, _outbuffer, _extend, _error_code, _channel_select);
	}
	LeaveCriticalSection(&m_cs);
	
	return bRet;
}

int NetworkService::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	int bRet = mars::stn::kTaskFailHandleNoError;
	CGITask *pTask = NULL;

	EnterCriticalSection(&m_cs);
	auto it = map_task_.find(_taskid);
	if (it != map_task_.end())
	{
		pTask = it->second;
	}
	LeaveCriticalSection(&m_cs);

	if (pTask)
	{
		//���(Buf2Respһ�ɷ���kTaskFailHandleNoError,������д��_error_code�ص����ϲ㴦���쳣)
		pTask->Buf2Resp(_taskid, _user_context, _inbuffer, _extend, _error_code, _channel_select);

		//�ص�֪ͨ�ϲ�cgi����״̬�����������ص�����
		DoCallBack(pTask->m_res, pTask->cgitype_, pTask->taskid_, _error_code);
	}

	return bRet;
}

int NetworkService::OnTaskEnd(uint32_t _taskid, void* const _user_context, int _error_type, int _error_code)
{
	uint32_t cmdid = 0;

	EnterCriticalSection(&m_cs);
	auto it = map_task_.find(_taskid);
	if (it != map_task_.end())
	{
		cmdid = it->second->cmdid_;
		
		//����ֻ�ͷ�CTask�ڴ�,CTask��Աm_res�ڴ����ϲ㸺���ͷ�
		delete it->second;
		map_task_.erase(it);
	}
	LeaveCriticalSection(&m_cs);

	//report kv���������Ҫ����Ƿ���Ҫ����ͬ��
	if (SEND_SYNC_SUCCESS == cmdid && NewSyncCGITask::IsNeedReSync())
	{
		NewSyncCGITask * task = new NewSyncCGITask();

		task->channel_select_ = ChannelType_LongConn;
		task->cmdid_ = SEND_NEWSYNC_CMDID;
		task->cgi_ = CGI_NEWSYNC;
		task->cgitype_ = CGI_TYPE_NEWSYNC;
		task->host_ = SHORTLINK_HOST;
		pNetworkService.startTask(task);
	}
	
	return 0;
}

void NetworkService::OnPush(uint64_t _channel_id, uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend)
{
	printf("[OnPush]����++++++++++\n");
	
	//TODO:��ǰ<������ȷ��>û��ʵ��,������ֻ���·�����֪ͨ,�����·���������,��Ҫ��������ͬ��
	if (pAuthInfo->GetSyncKey().size())
	{
		//�õ�ͬ��key������ͬ��
		NewSync();
	}	
}

int NetworkService::GetLonglinkIdentifyCheckBuffer(AutoBuffer& _identify_buffer, AutoBuffer& _buffer_hash, int32_t& _cmdid)
{
	//TODO:��ʱ��ʹ�ó�����ȷ������
	return mars::stn::IdentifyMode::kCheckNever;
	
	_cmdid = LONGLINK_IDENTIFY_REQ;
	
	//�ȵ����Ի�ȡ����ͬ��key��uin���ٽ���ȷ��
	string strSyncKey = pAuthInfo->GetSyncKey();
	DWORD  dwUin = pAuthInfo->m_uin;
	
	if (!strSyncKey.size() || !dwUin)
	{
		//δ�ܻ�ȡ��ͬ��key,�ȴ��´��ٳ���ȷ��
		return mars::stn::IdentifyMode::kCheckNext;
	}

	//���:
	//���4�ֽ�: (uin >> 13 & 0x7FFFF | synckey.length << 19) ^ 0x5601F281
	//���4�ֽ�: 0x5601F281 ^ (synckey.length >> 13& 0x7FFFF | uin << 19)
	//ͬ��key protobuf
	//���4�ֽڰ汾��
	//8�ֽڱ�������:�̶�Ϊ"zh_CN"
	//00 00 00 02
	//���4�ֽ���������(����������̶�Ϊ:00 00 00 01)
	//00 00 00 01

	string req;
	DWORD dwTemp = ((dwUin >> 13 & 0x7FFFF) | strSyncKey.size() << 19 ) ^ 0x5601F281;
	dwTemp = htonl(dwTemp);
	req.append((const char *)&dwTemp, 4);

	dwTemp = 0x5601F281 ^ ((strSyncKey.size() >> 13 & 0x7FFFF) | dwUin << 19);
	dwTemp = htonl(dwTemp);
	req.append((const char *)&dwTemp, 4);

	req.append(strSyncKey);

	dwTemp = pAuthInfo->m_ClientVersion;
	dwTemp = htonl(dwTemp);
	req.append((const char *)&dwTemp, 4);

	char szLocale[8] = { 0 };
	strcpy(szLocale, "zh_CN");
	req.append(szLocale, 8);

	char szEnd[12] = { 0,0,0,2,0,0,0,1,0,0,0,1 };
	req.append(szEnd, 12);

	_identify_buffer.AllocWrite(req.size());
	_identify_buffer.Write(req.c_str(), req.size());
	return mars::stn::IdentifyMode::kCheckNow;
}

bool NetworkService::OnLonglinkIdentifyResponse(const AutoBuffer& _response_buffer, const AutoBuffer& _identify_buffer_hash)
{
	return TRUE;
}

void NetworkService::RequestSync()
{
	//����ͬ����Ϣ,�����Ӳ�����ʱȷ��һ��ʱ�������յ���Ϣ
	NewSync();
}
