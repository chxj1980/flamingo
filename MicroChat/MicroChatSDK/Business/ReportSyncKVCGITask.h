#pragma once

#include "Wrapper/CGITask.h"
#include "mars/boost/weak_ptr.hpp"
#include "fun.h"
#include "MakeHeader.h"


class ReportSyncKVCGITask : public CGITask
{
public:
	virtual bool Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select);
	virtual int Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select);

	//ÿ��ͬ������øú���֪ͨ������
	static void ReportSyncKV();

private:
	//���������ݲ���Ҫ����,��ͷ��������
	string MakeHeader(int nLenProtobuf);
};