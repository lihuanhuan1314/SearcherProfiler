// SearcherProfiler.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include<Winsock2.h> 
#include<iostream>
#include<string>
#include<assert.h>
using namespace std;

#pragma comment(lib,"ws2_32.lib")
#define __DEBUG__
static string GetFileName(const string& path)
{
	char ch = '/';
#ifdef _WIN32
	ch = '\\';
#endif
	size_t pos = path.rfind(ch);
	if (pos == string::npos)
		return path;
	else
		return path.substr(pos + 1);
}
// ���ڵ���׷�ݵ�trace log
inline static void __trace_debug(const char* function,
	const char * filename, int line, char* format, ...)
{
#ifdef __DEBUG__
	// ������ú�������Ϣ
	fprintf(stdout, "�� %s:%d��%s", GetFileName(filename).c_str(), line, function);
	// ����û����trace��Ϣ
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
#endif
}
#define __TRACE_DEBUG(...) \
	__trace_debug(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);

//����URL  ��URL�з������������Դ
bool ParserUrl(const string&url,string& hostString,string& resourceString)
{
	// ���� http://�� �ҵ�����һ��ָ��
	const char *str = strstr(url.c_str(), "http://");
	if (str == nullptr)//û�ҵ�  www.baidu.com
	{
		str = url.c_str();
	}
	else //�ҵ��� http://www.baidu.com ָ�������ƣ�ָ��������ʼ
	{
		str = str + strlen("http://");
	}
	//������ʽ
	char hostC[MAXBYTE] = { 0 };
	char resourceC[MAXBYTE] = { 0 };
	sscanf_s(str,"%[^/]%s",hostC,MAXBYTE,resourceC,MAXBYTE);//  /ǰ������������Ž�hostC  /���������Դ���Ž�resourceC

	hostString = hostC;
	//�ж�/�����Ƿ�����Դ
	if (strlen(resourceC) != 0)
	{
		resourceString = resourceC;
	}
	else
	{
		resourceString = "/";
	}
	return true;
}
//�������Ϣ���浽�ļ���
void SaveRecvBuf(char*& recvBuf)
{
	FILE *fIn = fopen("SaveRecvBuf.txt", "wb");
	assert(fIn);
	fprintf(fIn, "%s", recvBuf);
	fclose(fIn);
}
void SaveHref(string& hrefString)
{
	FILE *fIn = fopen("SaveHref.txt", "wb");
	assert(fIn);
	fprintf(fIn, "%s", hrefString);
	fclose(fIn);
}

//ͨ��socket����һ��GET����ָ���ķ����
//1�����͸�˭: ͨ��IP �˿�   �ж�    
//2�����͵�����
void Start(string& url,string& hostString,string& resourceString)
{
	ParserUrl(url, hostString, resourceString);
	hostent* server = gethostbyname(hostString.c_str());//����ת����IP
	//����socket ����GET����
	//send(socket,��ַ�ṹ�����͵�����)
	//���͵����� ��GET ������ԴHTTP/1.1  Host:������ַ   Connection:����
	SOCKET sendSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sendaddr_in = { 0 };//��ַ�ṹ
	sendaddr_in.sin_family = AF_INET;
	sendaddr_in.sin_port = htons(80);
	memcpy(&sendaddr_in.sin_addr, server->h_addr, 4);
	//����
	connect(sendSock, (SOCKADDR*)&sendaddr_in, sizeof(sendaddr_in));
	//����
	string request = "GET " + resourceString + " HTTP/1.1\r\nHost: " + hostString + "\r\nConnection:Close\r\n\r\n";
	if (SOCKET_ERROR == send(sendSock, request.c_str(), request.length(), 0))
	{
		cout << "Sendʧ���ˣ�Error Code:" << WSAGetLastError() << endl;
	}

	//����˷���HTML�Ĵ���
	const int maxSize = 1 * 1024 * 1024;
	char *recvBuf = (char*)malloc(maxSize);
	ZeroMemory(recvBuf, maxSize);

	int recvSize = 1;
	int offset = 0;
	int count = 2;
	//��recvSize<0����=0��������ɣ�����0ѭ����ν���
	while (recvSize > 0)
	{
		recvSize = recv(sendSock, recvBuf + offset, maxSize - offset, 0);
		offset += recvSize;
		if (maxSize - offset < 100)
		{
			recvBuf = (char*)realloc(recvBuf, count*maxSize);
			count++;
		}   
	}
	SaveRecvBuf(recvBuf);
	//�����õ����������з���
	string hrefString;
	//����hrefString   ��hrefString�з������������Դ
	const char *str = strstr(recvBuf, "href=\"");
	if (str == nullptr)//û�ҵ�  
	{
		str = recvBuf;
	}
	else//�ҵ���  ָ��������
	{
		str = str + strlen("href=\"");
	}

	//������ʽ ƥ���hrefString
	char hostC[MAXBYTE] = { 0 };
	sscanf_s(str, "%[^""]%s", hostC, MAXBYTE);
	hrefString = hostC;

	while (!hrefString.empty())
	{
		if (strstr(hrefString.c_str(), "http://") != NULL)
		{
			Start(hrefString, hostString, resourceString);
		}
		else if (hrefString == "/tb/cms/content-search.xml")
		{
			cout << hrefString << endl;
			SaveHref(hrefString);
		}
		else
		{
			return;
		}
	}
	free(recvBuf);
}

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa = {0};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "��ʼ��ʧ�ܣ�" << endl;
		return 1;
	}
	string url = "http://tieba.baidu.com/f?kw=%B1%B1%BE%A9%B4%F3%D1%A7";
	string hostString;//����
	string resourceString;//��Դ
	Start(url,hostString,resourceString);
	WSACleanup();
	return 0;
}

