// SearcherProfiler.cpp : 定义控制台应用程序的入口点。
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
// 用于调试追溯的trace log
inline static void __trace_debug(const char* function,
	const char * filename, int line, char* format, ...)
{
#ifdef __DEBUG__
	// 输出调用函数的信息
	fprintf(stdout, "【 %s:%d】%s", GetFileName(filename).c_str(), line, function);
	// 输出用户打的trace信息
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
#endif
}
#define __TRACE_DEBUG(...) \
	__trace_debug(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);

//解析URL  从URL中分离出域名及资源
bool ParserUrl(const string&url,string& hostString,string& resourceString)
{
	// 查找 http://， 找到返回一个指针
	const char *str = strstr(url.c_str(), "http://");
	if (str == nullptr)//没找到  www.baidu.com
	{
		str = url.c_str();
	}
	else //找到了 http://www.baidu.com 指针往后移，指向域名开始
	{
		str = str + strlen("http://");
	}
	//正则表达式
	char hostC[MAXBYTE] = { 0 };
	char resourceC[MAXBYTE] = { 0 };
	sscanf_s(str,"%[^/]%s",hostC,MAXBYTE,resourceC,MAXBYTE);//  /前面的是域名，放进hostC  /后面的是资源，放进resourceC

	hostString = hostC;
	//判断/后面是否有资源
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
//将输出信息保存到文件中
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

//通过socket发送一个GET请求到指定的服务端
//1、发送给谁: 通过IP 端口   判断    
//2、发送的内容
void Start(string& url,string& hostString,string& resourceString)
{
	ParserUrl(url, hostString, resourceString);
	hostent* server = gethostbyname(hostString.c_str());//域名转换成IP
	//创建socket 发送GET请求
	//send(socket,地址结构，发送的内容)
	//发送的内容 ：GET 请求资源HTTP/1.1  Host:请求网址   Connection:连接
	SOCKET sendSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sendaddr_in = { 0 };//地址结构
	sendaddr_in.sin_family = AF_INET;
	sendaddr_in.sin_port = htons(80);
	memcpy(&sendaddr_in.sin_addr, server->h_addr, 4);
	//连接
	connect(sendSock, (SOCKADDR*)&sendaddr_in, sizeof(sendaddr_in));
	//发送
	string request = "GET " + resourceString + " HTTP/1.1\r\nHost: " + hostString + "\r\nConnection:Close\r\n\r\n";
	if (SOCKET_ERROR == send(sendSock, request.c_str(), request.length(), 0))
	{
		cout << "Send失败了！Error Code:" << WSAGetLastError() << endl;
	}

	//服务端返回HTML的代码
	const int maxSize = 1 * 1024 * 1024;
	char *recvBuf = (char*)malloc(maxSize);
	ZeroMemory(recvBuf, maxSize);

	int recvSize = 1;
	int offset = 0;
	int count = 2;
	//当recvSize<0或者=0，接收完成，大于0循环多次接收
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
	//我们拿到代码对其进行分析
	string hrefString;
	//解析hrefString   从hrefString中分离出域名及资源
	const char *str = strstr(recvBuf, "href=\"");
	if (str == nullptr)//没找到  
	{
		str = recvBuf;
	}
	else//找到了  指针往后移
	{
		str = str + strlen("href=\"");
	}

	//正则表达式 匹配出hrefString
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
		cout << "初始化失败！" << endl;
		return 1;
	}
	string url = "http://tieba.baidu.com/f?kw=%B1%B1%BE%A9%B4%F3%D1%A7";
	string hostString;//域名
	string resourceString;//资源
	Start(url,hostString,resourceString);
	WSACleanup();
	return 0;
}

