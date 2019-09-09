#include "ReportSyncKVCGITask.h"
#include "AuthInfo.h"
#include "Business/define.h"
#include "Wrapper/NetworkService.h"
#include <mars/comm/windows/projdef.h>
#include "proto/generate/newsync.pb.h"

bool ReportSyncKVCGITask::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	string strSyncKey = pAuthInfo->GetSyncKey();
	com::tencent::mars::microchat::proto::syncMsgKey_ msgkey;
	bool bRet = msgkey.ParsePartialFromArray(strSyncKey.c_str(), strSyncKey.size());
	
	string reqProtobuf;

	if (bRet)
	{
		reqProtobuf = msgkey.msgkey().SerializePartialAsString();	
	}

	string Header = MakeHeader(reqProtobuf.size());

	string req = Header + reqProtobuf;

	_outbuffer.AllocWrite(req.size());
	_outbuffer.Write(req.c_str(), req.size());
	
	return TRUE;
}

int ReportSyncKVCGITask::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	return 0;
}

void ReportSyncKVCGITask::ReportSyncKV()
{
	ReportSyncKVCGITask* task = new ReportSyncKVCGITask();

	task->channel_select_ = ChannelType_LongConn;
	task->cmdid_ = SEND_SYNC_SUCCESS;

	//����Ϣ��֪ͨ����������Ϣ�ѱ�����,����Ҫ������Ӧ��
	pNetworkService.startTask(task,TRUE);
}


string ReportSyncKVCGITask::MakeHeader(int nLenProtobuf)
{
	//��ͷ�̶�8�ֽ�,ǰ4�ֽ�Ϊʱ���(newsync utc�뵱ǰ����utcʱ��������,��������д;���,��λus)
	//��4�ֽڴ�˰��峤��
	string strHeader;

	char szBuf[4] = { 0,0,0,0xD0 };
	strHeader = strHeader + string(szBuf, 4);

	DWORD dwLen = htonl(nLenProtobuf);
	strHeader = strHeader + string((const char *)&dwLen, 4);
	
	return strHeader;
}
