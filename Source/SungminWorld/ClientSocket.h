// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// winsock2 사용을 위해 아래 코멘트 추가
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>

#define	MAX_BUFFER		1024
#define SERVER_PORT		8000
#define SERVER_IP		"127.0.0.1"

struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

struct location
{
	float x;
	float y;
	float z;
};

/**
 * 
 */
class SUNGMINWORLD_API ClientSocket
{
public:
	ClientSocket();
	~ClientSocket();

	bool InitSocket();
	bool Connect(const char * pszIP, int nPort);
	void SendMyLocation(const FVector& ActorLocation);

private:
	SOCKET m_Socket;
};
