#include "stdafx.h"
#include "ManualAuthCGITask.h"
#include <mars/comm/windows/projdef.h>
#include "proto/generate/manualauth.pb.h"
#include "business/crypto/mmCryptInterface.h"
#include "AuthInfo.h"
#include "Business/define.h"
#include "Wrapper/NetworkService.h"


bool ManualAuthCGITask::Req2Buf(uint32_t _taskid, void* const _user_context, AutoBuffer& _outbuffer, AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	GenPwd();
	GenAesKey();
	GenEcdh();

	string reqAccountProtobuf = MakeAccountReq();
	string keye = Hex2bin(LOGIN_RSA_VER158_KEY_E);
	string keyn = Hex2bin(LOGIN_RSA_VER158_KEY_N);
	DWORD dwCompressedAccount = 0;
	string reqAccount = compress_rsa(keye,keyn, reqAccountProtobuf, dwCompressedAccount);
	if (!reqAccount.size())	return FALSE;


	string reqDeviceProtobuf = MakeDeviceReq("", "", "", "", "", "", "","", FALSE);
	DWORD dwCompressedDevice = 0;
	string reqDevice = compress_aes(m_strAesKey, reqDeviceProtobuf, dwCompressedDevice);
	if (!reqDevice.size())	return FALSE;

	string subHeader;
	DWORD dwLenAccountProtobuf = htonl(reqAccountProtobuf.size());
	subHeader = subHeader + string((const char *)&dwLenAccountProtobuf, 4);
	DWORD dwLenDeviceProtobuf = htonl(reqDeviceProtobuf.size());
	subHeader = subHeader + string((const char *)&dwLenDeviceProtobuf, 4);
	DWORD dwLenAccountRsa = htonl(reqAccount.size());
	subHeader = subHeader + string((const char *)&dwLenAccountRsa, 4);

	string body = subHeader + reqAccount + reqDevice;

	string header = MakeHeader(cgitype_, body.size(), body.size());

	string req = header + body;


	_outbuffer.AllocWrite(req.size());
	_outbuffer.Write(req.c_str(),req.size());

	LOG("��¼������ɹ�,����%d\r\n�ȴ����������ص�½���......\r\n", req.size());

	return TRUE;
}

int ManualAuthCGITask::Buf2Resp(uint32_t _taskid, void* const _user_context, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select)
{
	_error_code = CGI_CODE_LOGIN_FAIL;

	m_res = new LoginResult;
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
	if (m_bCompressed)
		RespProtobuf = aes_uncompress(m_strAesKey, body, m_nLenRespProtobuf);
	else
		RespProtobuf = aes_nouncompress(m_strAesKey, body);


	com::tencent::mars::sample::proto::ManualAuthResponse resp;
	bool bRet = resp.ParsePartialFromArray(RespProtobuf.c_str(), RespProtobuf.size());

	if (bRet)
	{
		LOG("ManualAuth����ɹ�,protobuf����%d\r\n", RespProtobuf.size());
		
		//���汾�ε�½code�ͽ��msg
		int nCode = resp.result().code();
		_error_code = nCode;	
		strcpy_s(((LoginResult *)m_res)->szMsg, resp.result().err_msg().msg().c_str());

		CStringA authMsg = Utf82CStringA(resp.result().err_msg().msg().c_str());
		LOG("���ε�½����������code:%d,err_msg:\r\n%s\r\n", nCode, authMsg);

		if (!nCode)
		{
			if (resp.authparam().ecdh().ecdhkey().len())
			{
				string strECServrPubKey = resp.authparam().ecdh().ecdhkey().key();

				//ECDH����
				string aesKey = DoEcdh(m_nid, strECServrPubKey, m_strLocalEcdhPriKey);

				if (resp.authparam().session().len())
				{
					string strSessionKey = resp.authparam().session().key();

					string session = aes_nouncompress(aesKey, strSessionKey);

					if (session.size())
					{
						LOG("\r\n===============��¼�ɹ�===============\r\n");
						LOG("ECDH���ֳɹ������ε�¼session:%s\r\n", session.c_str());
						
						//�����¼���,�������ͨѶʹ��
						pAuthInfo->m_Session = session.c_str();
						pAuthInfo->m_ClientVersion = s_dwVersion;
						pAuthInfo->m_uin = s_dwUin;
						pAuthInfo->m_UserName = m_strUserName.c_str();
						pAuthInfo->m_WxId = resp.accountinfo().wxid().c_str();
						pAuthInfo->m_guid = DEVICE_INFO_GUID;
						pAuthInfo->m_guid_15 = pAuthInfo->m_guid;
						pAuthInfo->m_guid_15.resize(15);
						pAuthInfo->m_androidVer = DEVICE_INFO_ANDROID_VER;
						pAuthInfo->m_launguage = DEVICE_INFO_LANGUAGE;

						//��wxid��uin���ظ��ϲ�
						strcpy_s(((LoginResult *)m_res)->szWxid, pAuthInfo->m_WxId.c_str());
						((LoginResult *)m_res)->dwUin = pAuthInfo->m_uin;
						
						//��¼�ɹ�
						_error_code = CGI_CODE_LOGIN_SUCC;
						return 0;
					}				
				}
				
				LOG("Server sessionKey ����ʧ��!!!�����mmЭ��!\r\n");
			}
			
			LOG("ECDH����ʧ��!!!�����mmЭ��!\r\n");
			_error_code = CGI_CODE_LOGIN_ECDH_ERR;
			return 0;
		}
		else
		{
			//��½ʧ��

			//����ʧ��ԭ��(����ʧ�ܾͷ��ظ��ϲ�raw����)
			string strXml = resp.result().err_msg().msg();
			if (strXml.npos != strXml.find("<Content><![CDATA["))
			{
				int nStart = strXml.find("<Content><![CDATA[") + strlen("<Content><![CDATA[");
				int nEnd = strXml.find("]></Content>", nStart);

				if (strXml.npos != nEnd)
				{
					string strReason = Utf82CStringA(strXml.substr(nStart, nEnd - nStart - 1).c_str());
					strcpy_s(((LoginResult *)m_res)->szMsg, strReason.c_str());
				}
			}

			//��Ҫɨ����Ȩ
			if (CGI_CODE_LOGIN_NEED_SCAN_QRCODE == nCode)
			{
				string strXml = resp.result().err_msg().msg();

				if (strXml.npos != strXml.find("<Url><![CDATA["))
				{
					int nStart = strXml.find("<Url><![CDATA[") + strlen("<Url><![CDATA[");
					int nEnd = strXml.find("]]></Url>",nStart);

					if (strXml.npos != nEnd)
					{
						string url = strXml.substr(nStart, nEnd-nStart);
						LOG("����һ���ڸ��豸��¼��������ת����֤ҳ��,��ɨ����Ȩ�����µ�¼!\r\n��֤ҳ���ַ:%s\r\n",url);

						_error_code = CGI_CODE_LOGIN_NEED_SCAN_QRCODE;
						strcpy_s(((LoginResult *)m_res)->szUrl, url.c_str());
						return 0;
					}
				}
				
				LOG("\r\n��ά��ҳ���ַ����ʧ��,�����mmЭ��!\r\n");
				_error_code = CGI_CODE_LOGIN_SCAN_QRCODE_ERR;
				return 0;
			}
			else if (CGI_CODE_LOGIN_NEED_MOBILE_MSG == nCode)
			{
				LOG("�������������Ȩ��֤��֤����......\r\n");

				//�����ȡ������֤��ƾ��
				pAuthInfo->m_mobilecode_authticket = resp.authparam().l();
				pAuthInfo->m_mobileNum = m_strUserName;

				_error_code = CGI_CODE_LOGIN_NEED_MOBILE_MSG;
				return 0;				
			}
			else
			{
				LOG("\r\n��¼ʧ��!\r\n");

				//����ԭ���µ�¼ʧ��
				_error_code = CGI_CODE_LOGIN_FAIL;
				return 0;
			}			
		}
	}
	else
	{		
		LOG("ManualAuth���ʧ�ܣ������mmЭ��!\r\n");	
		//�������ʧ��
		_error_code = CGI_CODE_DECRYPT_ERR;
		return 0;
	}

	return 0;
}

std::string ManualAuthCGITask::MakeHeader(int cgiType, int nLenProtobuf, int nLenCompressed)
{
	string strHeader;

	int nCur = 0;

	//��¼������Ҫ����ƶ��豸�����־
	//strHeader.push_back(0xbf);
	//nCur++;

	//��¼�����������������,����ֱ��ʹ��ѹ���㷨(���2bits��Ϊ2)
	unsigned char SecondByte = 0x2;

	//��ͷ�������д��
	strHeader.push_back(SecondByte);
	nCur++;

	//�����㷨(ǰ4bits),RSA����(7)
	unsigned char ThirdByte = 0x7 << 4;

	//cookie����(��4bits)����ǰЭ��Ĭ��15λ
	ThirdByte += 0xf;

	strHeader.push_back(ThirdByte);
	nCur++;

	DWORD dwVer = htonl(s_dwVersion);
	strHeader = strHeader + string((const char *)&dwVer, 4);
	nCur += 4;

	//��¼������Ҫuin ȫ0ռλ����
	DWORD dwUin = 0;
	strHeader = strHeader + string((const char *)&dwUin, 4);
	nCur += 4;

	//��¼������Ҫcookie ȫ0ռλ����
	char szCookie[15] = { 0 };
	strHeader = strHeader + string((const char *)szCookie, 15);
	nCur += 15;

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

	//����ͷ����д��ڶ��ֽ�ǰ6bits(��ͷ���Ȳ��ᳬ��6bits)
	SecondByte += (nCur << 2);

	//����ȷ�ĵڶ��ֽ�д���ͷ
	strHeader[0] = SecondByte;

	return strHeader;
}

std::string ManualAuthCGITask::UnPackHeader(string pack)
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

	//��¼�� ����uin
	DWORD dwUin;
	memcpy(&dwUin, &(pack[nCur]), 4);
	s_dwUin = ntohl(dwUin);
	nCur += 4;


	//ˢ��cookie(����15�ֽ�˵��Э��ͷ�Ѹ���)
	if (nCookieLen && nCookieLen <= 0xf)
	{
		s_cookie = pack.substr(nCur, nCookieLen);
		pAuthInfo->m_cookie = s_cookie;

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

void ManualAuthCGITask::GenPwd()
{
	m_strPwd = GetMd5_32(m_strPwd);
}

void ManualAuthCGITask::GenAesKey()
{
	m_strAesKey.resize(16);
	for (int i = 0; i < 16; i++)
	{
		m_strAesKey[i] = rand() % 0xff;
	}
}

void ManualAuthCGITask::GenEcdh()
{
	m_nid = ECDH_NID;
	::GenEcdh(m_nid, m_strLocalEcdhPubKey, m_strLocalEcdhPriKey);
}

std::string ManualAuthCGITask::MakeAccountReq()
{
	string req;
	com::tencent::mars::sample::proto::ManualAuthAccountRequest accountQuest;

	com::tencent::mars::sample::proto::ManualAuthAccountRequest_AesKey *aesKey = new com::tencent::mars::sample::proto::ManualAuthAccountRequest_AesKey;
	com::tencent::mars::sample::proto::ManualAuthAccountRequest_Ecdh   *ecdh = new com::tencent::mars::sample::proto::ManualAuthAccountRequest_Ecdh;
	com::tencent::mars::sample::proto::ManualAuthAccountRequest_Ecdh_EcdhKey *ecdhKey = new com::tencent::mars::sample::proto::ManualAuthAccountRequest_Ecdh_EcdhKey;

	aesKey->set_len(m_nAesKeyLen);
	aesKey->set_key(m_strAesKey);
	ecdhKey->set_len(m_strLocalEcdhPubKey.size());
	ecdhKey->set_key(m_strLocalEcdhPubKey);
	ecdh->set_allocated_ecdhkey(ecdhKey);
	ecdh->set_nid(m_nid);

	accountQuest.set_allocated_aes(aesKey);
	accountQuest.set_allocated_ecdh(ecdh);
	accountQuest.set_username(m_strUserName);
	accountQuest.set_password1(m_strPwd);
	accountQuest.set_password2(m_strPwd);

	accountQuest.SerializeToString(&req);

	accountQuest.release_aes();
	accountQuest.release_ecdh();
	ecdh->release_ecdhkey();
	delete aesKey;
	delete ecdh;
	delete ecdhKey;

	return req;
}

std::string ManualAuthCGITask::MakeDeviceReq(string guid,string androidVer,string imei,string manufacturer,string modelName,string wifiMacAddress, string apBssid, string clientSeqId,bool bRandom)
{
#if 0
	//��������������豸���ݵ�¼
	string data;
	data = Hex2bin("");
	return data;
#else
	//ʹ��ָ���豸��¼
	string req;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest deviceQuest;

	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_LoginInfo *loginInfo = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_LoginInfo;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2  *unknownInfo2 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag1 *tag1 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag1;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag2 *tag2 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag2;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag2_Tag4 *tag2_tag4 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag2_Tag4;
	tag2->set_allocated_tag4(tag2_tag4);
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag3 *tag3 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag3;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag4 *tag4 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag4;
	com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag5 *tag5 = new com::tencent::mars::sample::proto::ManualAuthDeviceRequest_UnknowInfo2_Tag5;
	unknownInfo2->set_allocated_tag1(tag1);
	unknownInfo2->set_allocated_tag2(tag2);
	unknownInfo2->set_allocated_tag3(tag3);
	unknownInfo2->set_allocated_tag4(tag4);
	unknownInfo2->set_allocated_tag5(tag5);
	unknownInfo2->set_tag6(0);

	loginInfo->set_unknown1(string(""));
	loginInfo->set_unknown2(0);
	string guid15 = string(DEVICE_INFO_GUID, 15);
	loginInfo->set_guid(string(guid15.c_str(), 16));
	loginInfo->set_clientver(s_dwVersion);
	loginInfo->set_androidver(DEVICE_INFO_ANDROID_VER);
	loginInfo->set_unknown3(1);
	
	CStringA strSoftInfo;
	strSoftInfo.Format(DEVICE_INFO_SOFTINFO, DEVICE_INFO_IMEI,DEVICE_INFO_ANDROID_ID, DEVICE_INFO_MANUFACTURER+" "+DEVICE_INFO_MODELNAME, DEVICE_INFO_MOBILE_WIFI_MAC_ADDRESS, DEVICE_INFO_CLIENT_SEQID_SIGN, DEVICE_INFO_AP_BSSID, DEVICE_INFO_MANUFACTURER,"taurus", DEVICE_INFO_MODELNAME, DEVICE_INFO_IMEI);

	CStringA strDeviceInfo;
	strDeviceInfo.Format(DEVICE_INFO_DEVICEINFO, DEVICE_INFO_MANUFACTURER, DEVICE_INFO_MODELNAME);

	deviceQuest.set_allocated_login(loginInfo);
	deviceQuest.set_allocated_unknown2(unknownInfo2);
	deviceQuest.set_imei(DEVICE_INFO_IMEI);
	deviceQuest.set_softinfoxml(strSoftInfo);
	deviceQuest.set_unknown5(0);
	deviceQuest.set_clientseqid(DEVICE_INFO_CLIENT_SEQID);
	deviceQuest.set_clientseqid_sign(DEVICE_INFO_CLIENT_SEQID_SIGN);
	deviceQuest.set_logindevicename(DEVICE_INFO_MANUFACTURER+" "+DEVICE_INFO_MODELNAME);
	deviceQuest.set_deviceinfoxml(strDeviceInfo);
	deviceQuest.set_language(DEVICE_INFO_LANGUAGE);
	deviceQuest.set_timezone("8.00");
	deviceQuest.set_unknown13(0);
	deviceQuest.set_unknown14(0);
	deviceQuest.set_devicebrand(DEVICE_INFO_MANUFACTURER);
	deviceQuest.set_devicemodel(DEVICE_INFO_MODELNAME+"armeabi-v7a");
	deviceQuest.set_ostype(DEVICE_INFO_ANDROID_VER);
	deviceQuest.set_realcountry("cn");
	deviceQuest.set_unknown22(2);

	deviceQuest.SerializeToString(&req);

	unknownInfo2->release_tag1();
	unknownInfo2->release_tag2();
	unknownInfo2->release_tag3();
	unknownInfo2->release_tag4();
	unknownInfo2->release_tag5();
	tag2->release_tag4();
	deviceQuest.release_unknown2();
	deviceQuest.release_login();

	delete tag1;
	delete tag2;
	delete tag3;
	delete tag4;
	delete tag5;
	delete tag2_tag4;
	delete unknownInfo2;
	delete loginInfo;


	return req;
#endif
}

