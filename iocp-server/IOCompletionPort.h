#pragma once
// winsock2 사용을 위해 아래 코멘트 추가
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

	// 소켓 등록 및 서버 정보 설정
	bool Initialize();
	// 서버 시작
	void StartServer();
	// 작업 스레드 생성
	bool CreateWorkerThread();
	// 작업 스레드
	void WorkerThread();

private:
	stSOCKETINFO *	SocketInfo;		// 소켓 정보
	SOCKET			ListenSocket;		// 서버 리슨 소켓
	HANDLE			hIOCP;			// IOCP 객체 핸들
	bool			bAccept;			// 요청 동작 플래그
	bool			bWorkerThread;	// 작업 스레드 동작 플래그
	HANDLE *		hWorkerHandle;	// 작업 스레드 핸들	
	// location		WorldCharacterInfo[MAX_CLIENTS]; // 접속한 클라이언트의 정보를 저장
	cCharactersInfo CharactersInfo;
};
