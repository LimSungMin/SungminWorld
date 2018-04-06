// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// winsock2 사용을 위해 아래 코멘트 추가
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <map>

using namespace std;

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
#define SERVER_IP		"127.0.0.1"
#define MAX_CLIENTS		100

struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

class cLocation {
public:
	cLocation() {};
	~cLocation() {};

	int SessionId;
	float X;
	float Y;
	float Z;

	friend ostream& operator<<(ostream &stream, cLocation& loc)
	{
		stream << loc.SessionId << endl;
		stream << loc.X << endl;
		stream << loc.Y << endl;
		stream << loc.Z << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, cLocation& loc)
	{
		stream >> loc.SessionId;
		stream >> loc.X;
		stream >> loc.Y;
		stream >> loc.Z;

		return stream;
	}
};

class cCharactersInfo
{
public:
	cCharactersInfo() {};
	~cCharactersInfo() {};

	cLocation WorldCharacterInfo[MAX_CLIENTS];

	friend ostream& operator<<(ostream &stream, cCharactersInfo& info)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			stream << info.WorldCharacterInfo[i] << endl;
		}
		return stream;
	}

	friend istream &operator>>(istream &stream, cCharactersInfo& info)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			stream >> info.WorldCharacterInfo[i];
		}
		return stream;
	}
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
	int SendMyLocation(const int& SessionId, const FVector& ActorLocation);

private:
	SOCKET	m_Socket;
	char 	recvBuffer[MAX_BUFFER];	
	cCharactersInfo CharactersInfo;
};
