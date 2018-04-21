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
	bool	IsAttacking;

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
		stream << info.IsAttacking << endl;

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
		stream >> info.IsAttacking;

		return stream;
	}
};

enum EPacketType
{
	LOGIN,
	ENROLL_PLAYER,
	SEND_PLAYER,
	RECV_PLAYER,
	LOGOUT_PLAYER,
	HIT_PLAYER,
	DAMAGED_PLAYER,
	CHAT,
	ENTER_NEW_PLAYER
};

class cCharactersInfo
{
public:
	cCharactersInfo() {};
	~cCharactersInfo() {};
	
	map<int, cCharacter> players;

	friend ostream& operator<<(ostream &stream, cCharactersInfo& info)
	{		
		stream << info.players.size() << endl;
		for (auto& kvp : info.players)
		{
			stream << kvp.first << endl;
			stream << kvp.second << endl;
		}

		return stream;
	}

	friend istream &operator>>(istream &stream, cCharactersInfo& info)
	{		
		int nPlayers = 0;
		int SessionId = 0;
		cCharacter Player;
		info.players.clear();

		stream >> nPlayers;
		for (int i = 0; i < nPlayers; i++)
		{
			stream >> SessionId;
			stream >> Player;
			info.players[SessionId] = Player;
		}

		return stream;
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
	void EnrollPlayer(cCharacter& info);
	// ĳ���� ����ȭ
	void SendPlayer(cCharacter& info);	
	// ĳ���� �α׾ƿ�
	void LogoutPlayer(int SessionId);	
	// ĳ���� �ǰ� ó��
	void DamagePlayer(int SessionId);
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
	
	SOCKADDR_IN	UdpServerAddr;	
	ASungminPlayerController* PlayerController;	// �÷��̾� ��Ʈ�ѷ� ����

	
	char testChat[MAX_BUFFER];

	//////////////////////////////////////////////////////////////////////////
	// ������ȭ
	//////////////////////////////////////////////////////////////////////////
	cCharactersInfo CharactersInfo;		// ĳ���� ����
	cCharactersInfo* RecvCharacterInfo(stringstream& RecvStream);

	string sChat;
	string* RecvChat(stringstream& RecvStream);

	cCharactersInfo NewPlayer;
	cCharactersInfo* RecvNewPlayer(stringstream& RecvStream);
	//////////////////////////////////////////////////////////////////////////
};


