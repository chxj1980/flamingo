#pragma once

#include "Wrapper/CGITask.h"
#include "mars/boost/weak_ptr.hpp"
#include "fun.h"
#include "MakeHeader.h"


class NewSyncCGITask;


class NewSyncCGITask : public CGITask, public BaseHeader
{
public:

	virtual bool Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select);
	virtual int Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select);

	static bool IsSyncing();		//�Ƿ�����ͬ��(ͬһʱ��ֻ����һ��ͬ��������ִ��)
	static bool IsNeedReSync();			//һ��ͬ�����������,�Ƿ���Ҫ�����ٴ�ͬ��
	
	static CRITICAL_SECTION s_cs;
	static bool s_bSyncing;			//TRUE��ʾ����ͬ�����񼴽�ִ��,����Ҫ��Ͷ��ͬ������
	static bool s_bNeedReSync;		//TRUE��ʾ��ǰͬ�����������,��Ҫ��������һ��ͬ������
private:
	string MakeNewSyncReq();
	CGI_TYPE m_nCgiType = CGI_TYPE_NEWSYNC;

};