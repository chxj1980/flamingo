#include "stdafx.h"
#include "BindCGITask.h"
#include <mars/comm/windows/projdef.h>
#include "proto/generate/bindopmobileforreg.pb.h"
#include "business/crypto/mmCryptInterface.h"
#include "AuthInfo.h"
#include "Business/define.h"
#include "Wrapper/NetworkService.h"


bool BindCGITask::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	GenAesKey();

	string reqProtobuf = MakeReq();
	string keye = Hex2bin(LOGIN_RSA_VER158_KEY_E);
	string keyn = Hex2bin(LOGIN_RSA_VER158_KEY_N);
	DWORD dwCompressed = 0;
	string reqbuf = compress_rsa(keye,keyn, reqProtobuf, dwCompressed);
	if (!reqbuf.size())	return FALSE;

	string header = MakeHeader(cgitype_, reqProtobuf.size(), dwCompressed, reqbuf.size());

	string req = header + reqbuf;

	_outbuffer.AllocWrite(req.size());
	_outbuffer.Write(req.c_str(),req.size());

	return TRUE;
}

int BindCGITask::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	_error_code = CGI_CODE_LOGIN_FAIL;

	m_res = new MobileVerifyResult;
	NEW_ERR(m_res);
	
	LOG("�հ��ɹ�������%d\r\n��ʼ���......\r\n", _inbuffer.Length());

	string body = UnPackHeader(string((const char *)_inbuffer.Ptr(), _inbuffer.Length()));

	if (!body.size())
	{
		LOG("����쳣���밴mmЭ����ȷ��������!\r\n", _inbuffer.Length());

		//��ͷ���ʧ��
		_error_code = CGI_CODE_UNPACK_ERR;
		return 0;
	}

	string RespProtobuf;

	if (5 == m_nDecryptType)
	{
		if (m_bCompressed)
		{
			RespProtobuf = aes_uncompress(m_strAesKey, body, m_nLenRespProtobuf);
		}
		else
		{
			RespProtobuf = aes_nouncompress(m_strAesKey, body);
		}

		if (RespProtobuf.size())
		{
			_error_code = CGI_CODE_OK;
		}

		com::tencent::mars::microchat::proto::BindResponse resp;
		bool bRet = resp.ParsePartialFromArray(RespProtobuf.c_str(), RespProtobuf.size());
		if (bRet)
		{
			((MobileVerifyResult *)m_res)->nCode = resp.tag1().code();
			strcpy_s(((MobileVerifyResult *)m_res)->szMsgResult, Utf82CStringA(resp.tag1().result().strresult().c_str()));
			((MobileVerifyResult *)m_res)->nOption = m_nOptionType;			
		}
		else
		{
			_error_code = CGI_CODE_PARSE_PROTOBUF_ERR;
			return 0;
		}
	}
	else
	{
		_error_code = CGI_CODE_DECRYPT_ERR;
		return 0;
	}

	return 0;
}

std::string BindCGITask::MakeHeader(int cgiType, int nLenProtobuf, int nLenCompressed,int nLenRsa)
{
	string strHeader;

	int nCur = 0;

	//�ƶ��豸�����־
	strHeader.push_back(0xbf);
	nCur++;

	//protobufѹ����־λ
	unsigned char SecondByte = 0x1;

	//��ͷ�������д��
	strHeader.push_back(SecondByte);
	nCur++;

	//�����㷨(ǰ4bits)
	unsigned char ThirdByte = 0x1 << 4;

	//cookie����,д��Ҳ��д,������cookie
	ThirdByte += 0x0;

	strHeader.push_back(ThirdByte);
	nCur++;

	//�汾��
	DWORD dwVer = htonl(s_dwVersion);
	strHeader = strHeader + string((const char *)&dwVer, 4);
	nCur += 4;

	//δ��¼�ɹ�ʱ����Ҫuin ȫ0ռλ����
	DWORD dwUin = 0;
	strHeader = strHeader + string((const char *)&dwUin, 4);
	nCur += 4;

	string strCgi = Dword2String(cgiType);
	strHeader = strHeader + strCgi;
	nCur += strCgi.size();

	string strLenProtobuf = Dword2String(nLenProtobuf);
	strHeader = strHeader + strLenProtobuf;
	nCur += strLenProtobuf.size();

	string strLenCompressed = Dword2String(nLenCompressed);
	strHeader = strHeader + strLenCompressed;
	nCur += strLenCompressed.size();

	byte rsaVer = LOGIN_RSA_VER;
	strHeader = strHeader + string((const char *)&rsaVer, 1);
	nCur++;

	byte unkwnow[2] = { 0x01,0x02 };
	strHeader = strHeader + string((const char *)unkwnow, 2);
	nCur += 2;

	//���ܺ����ݳ���
	string subHeader;
	short uLenProtobuf = nLenRsa;
	strHeader = strHeader + string((const char *)&uLenProtobuf, 2);
	nCur += 2;

	//���������5���ֽ�(������д)
	char szUnknown[5] = { 0 };
	strHeader = strHeader + string((const char *)szUnknown, 5);
	nCur += 5;

	//����ͷ����д��ڶ��ֽ�ǰ6bits(��ͷ���Ȳ��ᳬ��6bits)
	SecondByte += (nCur << 2);

	//����ȷ�ĵڶ��ֽ�д���ͷ
	strHeader[1] = SecondByte;

	return strHeader;
}

std::string BindCGITask::UnPackHeader(string pack)
{
	string body;
	if (pack.size() < 0x20)	return body;

	int nCur = 0;

	//������׿��־bf(�����ͷ�ܳ���)
	if (0xbf == (unsigned char)pack[nCur])
	{
		nCur++;
	}

	//������ͷ����(ǰ6bits)
	int nHeadLen = (unsigned char)pack[nCur] >> 2;

	//�Ƿ�ʹ��ѹ��(��2bits)
	m_bCompressed = (1 == ((unsigned char)pack[nCur] & 0x3)) ? TRUE : FALSE;

	nCur++;

	//�����㷨(ǰ4 bits)(05:aes / 07:rsa)(�����ֽ׶εķ���ʹ��rsa��Կ����,����û��˽Կ�հ�һ��aes����)
	m_nDecryptType = (unsigned char)pack[nCur] >> 4;

	//cookie����(��4 bits)
	int nCookieLen = (unsigned char)pack[nCur] & 0xF;

	nCur++;

	//�������汾,����(4�ֽ�)
	nCur += 4;

	//����uin
	nCur += 4;

	//����cookie(����15�ֽ�˵��Э��ͷ�Ѹ���)
	if (nCookieLen && nCookieLen <= 0xf)
	{
		nCur += nCookieLen;
	}
	else if (nCookieLen > 0xf)
	{
		return body;
	}

	//cgi type,�䳤����,����
	DWORD dwLen = 0;
	DWORD dwCgiType = String2Dword(pack.substr(nCur, 5), dwLen);
	nCur += dwLen;

	//��ѹ��protobuf���ȣ��䳤����
	m_nLenRespProtobuf = String2Dword(pack.substr(nCur, 5), dwLen);
	nCur += dwLen;

	//ѹ����(����ǰ)��protobuf���ȣ��䳤����
	m_nLenRespCompressed = String2Dword(pack.substr(nCur, 5), dwLen);
	nCur += dwLen;

	//������������

	//������,ȡ����
	if (nHeadLen < pack.size())
	{
		body = pack.substr(nHeadLen);
	}

	return body;
}

void BindCGITask::GenAesKey()
{
	m_strAesKey.resize(16);
	for (int i = 0; i < 16; i++)
	{
		m_strAesKey[i] = rand() % 0xff;
	}
}


std::string BindCGITask::MakeReq()
{
	string req;

	com::tencent::mars::microchat::proto::BindRequest bindRequest;
	com::tencent::mars::microchat::proto::BindRequest_AesKey *aesKey = new com::tencent::mars::microchat::proto::BindRequest_AesKey;
	com::tencent::mars::microchat::proto::BindRequest_DeviceInfo *pDevice = new com::tencent::mars::microchat::proto::BindRequest_DeviceInfo;

	aesKey->set_len(m_nAesKeyLen);
	aesKey->set_key(m_strAesKey);

	pDevice->set_unknown1("");
	pDevice->set_unknown2(0);
	string guid15 = string(DEVICE_INFO_GUID, 15);
	pDevice->set_guid(string(guid15.c_str(), 16));
	pDevice->set_clientver(s_dwVersion);
	pDevice->set_androidver(DEVICE_INFO_ANDROID_VER);
	pDevice->set_unknown3(0);

	bindRequest.set_allocated_aes(aesKey);
	bindRequest.set_allocated_info(pDevice);

	bindRequest.set_mobilenum(m_strPhoneNum);
	bindRequest.set_option(m_nOptionType);

	if (11 == m_nOptionType)
	{
		bindRequest.set_verifycode(m_strVerifyCode);
		bindRequest.set_authticket(m_strAuthTicket);
	}
	else if (10 == m_nOptionType)
	{
		bindRequest.set_verifycode("");
		bindRequest.set_authticket(m_strAuthTicket);
	}
	else if (13 == m_nOptionType)
	{
		bindRequest.set_verifycode("");
		CStringA strNum = m_strPhoneNum.c_str();
		if (0 != strNum.Find("+86"))
		{
			strNum.Format("+86%s", m_strPhoneNum.c_str());
		}
		bindRequest.set_mobilenum(strNum);
	}

	bindRequest.set_tag6(0);
	bindRequest.set_tag7("");
	bindRequest.set_tag9(0);
	string strDeviceType = CStringA2Utf8("Android�豸");
	bindRequest.set_devicetype(strDeviceType);
	CStringA strDeviceName;
	strDeviceName.Format("%s%s", DEVICE_INFO_MANUFACTURER, DEVICE_INFO_MODELNAME);
	strDeviceName = CStringA2Utf8(strDeviceName);
	bindRequest.set_devicename(strDeviceName);
	bindRequest.set_launguage(DEVICE_INFO_LANGUAGE);
	bindRequest.set_tag14(0);
	bindRequest.set_tag15(0);
	bindRequest.set_clientseqid(DEVICE_INFO_CLIENT_SEQID);

	bindRequest.SerializeToString(&req);

	FILE *fp = fopen("bind.txt", "a+");
	if (fp)
	{
		string strNewMsg = Bin2Hex(req);
		fwrite(strNewMsg.c_str(), 1, strNewMsg.size(), fp);

		string strEnd = "\r\n";
		fwrite(strEnd.c_str(), 1, strEnd.size(), fp);

		fclose(fp);
	}

	bindRequest.release_aes();
	bindRequest.release_info();

	delete aesKey;
	delete pDevice;

	return req;
}
