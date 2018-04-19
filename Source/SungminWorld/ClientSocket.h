// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <map>
#include "Runtime/Core/Public/HAL/Runnable.h"

class ASungminWorldGameMode;
class ASungminPlayerController;

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
	// �ӵ�
	float VX;
	float VY;
	float VZ;
	// �Ӽ�
	bool	IsAlive;	
	float	HealthValue;

	friend ostream& operator<<(ostream &stream, cCharacter& info)
	{
		stream << info.SessionId << endl;
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.VX << endl;
		stream << info.VY << endl;
		stream << info.VZ << endl;
		stream << info.Yaw << endl;
		stream << info.Pitch << endl;
		stream << info.Roll << endl;
		stream << info.IsAlive << endl;		
		stream << info.HealthValue << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, cCharacter& info)
	{
		stream >> info.SessionId;
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.VX;
		stream >> info.VY;
		stream >> info.VZ;
		stream >> info.Yaw;
		stream >> info.Pitch;
		stream >> info.Roll;
		stream >> info.IsAlive;		
		stream >> info.HealthValue;

		return stream;
	}
};

enum EPacketType
{
	LOGIN,
	ENROLL_CHARACTER,
	SEND_CHARACTER,
	RECV_CHARACTER,
	LOGOUT_CHARACTER,
	HIT_CHARACTER,
	DAMAGED_CHARACTER,
	CHAT
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

class cChat 
{
public:
	cChat() {};
	~cChat() {};

	int SessionId;
	string Chat;

	friend ostream& operator<<(ostream &stream, cChat& info)
	{
// 		stream << info.SessionId << endl;
// 		stream << 
	}

	friend istream &operator>>(istream &stream, cChat& info)
	{

	}
};

/**
 * 
 */
class SUNGMINWORLD_API ClientSocket : public FRunnable
{
public:
	ClientSocket();	
	virtual ~ClientSocket();

	// ���� ��� �� ����
	bool InitSocket();
	// ������ ����
	bool Connect(const char * pszIP, int nPort);

	//////////////////////////////////////////////////////////////////////////
	// ������ ���
	//////////////////////////////////////////////////////////////////////////

	// ������ �α���
	bool Login(const FText & Id, const FText & Pw);
	// �ʱ� ĳ���� ���
	void EnrollCharacterInfo(cCharacter& info);
	// ĳ���� ����ȭ
	void SendCharacterInfo(cCharacter& info);	
	// ĳ���� �α׾ƿ�
	void LogoutCharacter(int SessionId);	
	// ĳ���� �ǰ� ó��
	void DamagingCharacter(int SessionId);
	// ä�� 
	void SendChat(const int& SessionId, const string& Chat);
	// UDP �׽�Ʈ�� �Լ�
	char* UdpTest();
	//////////////////////////////////////////////////////////////////////////	

	// �÷��̾� ��Ʈ�ѷ� ����
	void SetPlayerController(ASungminPlayerController* pPlayerController);

	void CloseSocket();

	// FRunnable Thread members	
	FRunnableThread* Thread;
	FThreadSafeCounter StopTaskCounter;

	// FRunnable override �Լ�
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit();

	// ������ ���� �� ����
	bool StartListen();
	void StopListen();	

	// �̱��� ��ü ��������
	static ClientSocket* GetSingleton()
	{
		static ClientSocket ins;
		return &ins;
	}

private:
	SOCKET	ServerSocket;				// ������ ������ ����
	SOCKET	UdpServerSocket;
	char 	recvBuffer[MAX_BUFFER];		// ���� ���� ��Ʈ��	
	char UdpRecvBuffer[MAX_BUFFER];
	cCharactersInfo CharactersInfo;		// ĳ���� ����
	SOCKADDR_IN	UdpServerAddr;	
	ASungminPlayerController* PlayerController;	// �÷��̾� ��Ʈ�ѷ� ����

	string sChat;
	char testChat[MAX_BUFFER];
	cCharactersInfo* RecvCharacterInfo(stringstream& RecvStream);
	string* RecvChat(stringstream& RecvStream);
};


