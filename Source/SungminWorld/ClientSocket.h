// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <map>
//#include "CommonClass.h"

using namespace std;

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
#define UDP_SERVER_PORT	8001
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

class cCharacter {
public:
	cCharacter() {};
	~cCharacter() {};

	// ���� ���̵�
	int SessionId;
	// ��ġ
	float X;
	float Y;
	float Z;
	// ȸ����
	float Yaw;
	float Pitch;
	float Roll;

	friend ostream& operator<<(ostream &stream, cCharacter& info)
	{
		stream << info.SessionId << endl;
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Yaw << endl;
		stream << info.Pitch << endl;
		stream << info.Roll << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, cCharacter& info)
	{
		stream >> info.SessionId;
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.Yaw;
		stream >> info.Pitch;
		stream >> info.Roll;

		return stream;
	}
};

enum EPacketType 
{
	SEND_CHARACTER,
	LOGOUT_CHARACTER
};

class cCharactersInfo
{
public:
	cCharactersInfo() {};
	~cCharactersInfo() {};

	cCharacter WorldCharacterInfo[MAX_CLIENTS];

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

	// ���� ��� �� ����
	bool InitSocket();
	// ������ ����
	bool Connect(const char * pszIP, int nPort);
	// ĳ���� ����ȭ
	cCharactersInfo* SyncCharacters(cCharacter& info);
	// ĳ���� �α׾ƿ�
	void LogoutCharacter(int SessionId);
	char* UdpTest();

private:
	SOCKET	ServerSocket;				// ������ ������ ����
	SOCKET	UdpServerSocket;
	char 	recvBuffer[MAX_BUFFER];		// ���� ���� ��Ʈ��
	char UdpRecvBuffer[MAX_BUFFER];
	cCharactersInfo CharactersInfo;		// ĳ���� ����
	SOCKADDR_IN	UdpServerAddr;
};
