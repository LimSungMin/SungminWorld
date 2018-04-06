#pragma once
// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <map>
#include <iostream>
// #include "custom_struct.h"

using namespace std;

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
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
		stream << loc.Y<< endl;
		stream << loc.Z<< endl;

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

class IOCompletionPort
{
public:
	IOCompletionPort();
	~IOCompletionPort();

	// ���� ��� �� ���� ���� ����
	bool Initialize();
	// ���� ����
	void StartServer();
	// �۾� ������ ����
	bool CreateWorkerThread();
	// �۾� ������
	void WorkerThread();

private:
	stSOCKETINFO *	SocketInfo;		// ���� ����
	SOCKET			ListenSocket;		// ���� ���� ����
	HANDLE			hIOCP;			// IOCP ��ü �ڵ�
	bool			bAccept;			// ��û ���� �÷���
	bool			bWorkerThread;	// �۾� ������ ���� �÷���
	HANDLE *		hWorkerHandle;	// �۾� ������ �ڵ�	
	// location		WorldCharacterInfo[MAX_CLIENTS]; // ������ Ŭ���̾�Ʈ�� ������ ����
	cCharactersInfo CharactersInfo;
};
