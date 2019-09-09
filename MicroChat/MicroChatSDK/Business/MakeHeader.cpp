#include "stdafx.h"
#include "MakeHeader.h"
#include "fun.h"
#include "AuthInfo.h"

DWORD	BaseHeader::s_dwVersion = CLIENT_VERSION;
int		BaseHeader::s_dwUin = 0;
string  BaseHeader::s_cookie = "";

string BaseHeader::MakeHeader(string cookie, int cgiType, int nLenProtobuf, int nLenCompressed)
{
	string strHeader;

	int nCur = 0;

	//����ƶ��豸�����־
	strHeader.push_back(0xbf);
	nCur++;
	
	//�Ƿ�ʹ��ѹ���㷨(���2bits)(1��ʾʹ��zlibѹ��)(ѹ���󳤶ȿ��ܱ䳤,��һ��ʹ��ѹ���㷨)
	unsigned char SecondByte = (nLenProtobuf == nLenCompressed) ? 0x2 : 0x1;

	//��ͷ�������д��
	strHeader.push_back(SecondByte);
	nCur++;

	//�����㷨(ǰ4bits),Ĭ��ʹ��aes����(5),��Ҫrsa���ܵ�CGI���ش��麯��
	unsigned char ThirdByte = 0x5 << 4;

	//cookie����(��4bits)����ǰЭ��Ĭ��15λ
	ThirdByte += 0xf;

	strHeader.push_back(ThirdByte);
	nCur++;

	//д��汾��(���4�ֽ�����)
	DWORD dwVer = htonl(s_dwVersion);
	strHeader = strHeader + string((const char *)&dwVer,4);
	nCur += 4;

	//д��uin(���4�ֽ�����)
	DWORD dwUin = htonl(pAuthInfo->m_uin);
	strHeader = strHeader + string((const char *)&dwUin, 4);
	nCur += 4;

	//д��cookie
	strHeader = strHeader + pAuthInfo->m_cookie;
	nCur += 0xf;

	//cgi type(�䳤����)
	string strCgi = Dword2String(cgiType);
	strHeader = strHeader + strCgi;
	nCur += strCgi.size();

	//protobuf����(�䳤����)
	string strProtobuf = Dword2String(nLenProtobuf);
	strHeader = strHeader + strProtobuf;
	nCur += strProtobuf.size();

	//protobufѹ���󳤶�(�䳤����)
	string strCompressed = Dword2String(nLenCompressed);
	strHeader = strHeader + strCompressed;
	nCur += strCompressed.size();

	//ecdhУ��ֵ��mmtlsЭ����õ��Ĳ���(15 byte)(��0 ��λ)
	char szBuf[15] = { 0 };
	strHeader = strHeader + string(szBuf, 15);
	nCur += 15;

	//����ͷ����д��ڶ��ֽ�ǰ6bits(��ͷ���Ȳ��ᳬ��6bits)
	SecondByte += (nCur << 2);


	//����ȷ�ĵڶ��ֽ�д���ͷ
	strHeader[1] = SecondByte;

	return strHeader;
}

std::string BaseHeader::UnPackHeader(string pack)
{
	string body;

	int nCur = 0;

	if (pack.size() < 0x20)	return body;

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

	//uin �ǵ�¼��,����(4�ֽ�)
	nCur += 4;

	//ˢ��cookie(����15�ֽ�˵��Э��ͷ�Ѹ���)
	if (nCookieLen && nCookieLen <= 0xf)
	{
		s_cookie = pack.substr(nCur, nCookieLen);
		pAuthInfo->m_cookie = s_cookie;

		nCur += 0xf;
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

