/*
	KE Wang
	822002009
*/
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

#define ERROR_FAIL 1


int Socket::startup()
{
	WORD Version;
	WSADATA Data;
	Version = MAKEWORD(2, 2);
	int ret  = WSAStartup(Version, &Data);
	if(ret != ERROR_SUCCESS)
	{
		printf("Winsock DLL not found.\n");
		return ERROR_FAIL;
	}

	if (LOBYTE(Data.wVersion) != 2 || HIBYTE(Data.wVersion) != 2 )
	{
		/* Tell the user that we could not find a usable WinSock DLL.*/
		printf("Server: The dll do not support the Winsock version %u.%u!\n", LOBYTE(Data.wVersion), HIBYTE(Data.wVersion));
		return ERROR_FAIL;
	}
	
	return ERROR_SUCCESS;
}


bool Socket::sock_create()
{
	if((sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("Fail to create socket with error = %d.\n",WSAGetLastError());
		return false;//return ERROR_FAIL;
	}

	bool option = true;
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(bool)) == SOCKET_ERROR)
	{
        printf("Setsockopt for SO_REUSEADDR failed with error: %u\n", WSAGetLastError());
		return false;
	}

	//LINGER linger;
	//linger.l_onoff = 0;
	//if(setsockopt(sock,SOL_SOCKET,SO_LINGER,(char*)&linger,sizeof(LINGER)) == SOCKET_ERROR)
	//{
	//	printf("Setsockopt for SO_LINGER failed with error: %u\n", WSAGetLastError());
	//	return ERROR_FAIL;
	//}

	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;

	if(bind(sock,(const sockaddr*)&server,sizeof(sockaddr))
		== SOCKET_ERROR)
	{
		printf("Fail to connect to server with error = %d.\n",WSAGetLastError());
		return false;
	}

	return true;
}



bool Socket::sock_send(char* send_buf,int count,char* address,char* port)
{	
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_port = htons(atoi(port));
	
	if(sendto(sock,send_buf,count,0, (SOCKADDR*)&server,sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("Fail to send to server with error = %d.\n",WSAGetLastError());
		return false;
	}
	return true;
}


bool Socket::sock_recv()
{
	SOCKADDR_IN server;	
	recv_buf = new char[1024];
	memset(recv_buf,0,recv_buf_size);
	int recv_count = 0;
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	int ret;
	timeval timeout;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	int count = 0;

	ret = select(0, &fd, NULL,NULL, &timeout);
	if(ret > 0)  
	{
		int size = sizeof(SOCKADDR);
		recvfrom(sock,recv_buf,recv_buf_size - total_recv_count,0,(SOCKADDR*)&server, &size);
		return true;
	}
	else if(ret == SOCKET_ERROR)
	{
		printf("Fail to select = %d.\n",WSAGetLastError());
		return false;
	}

	return false;
}

