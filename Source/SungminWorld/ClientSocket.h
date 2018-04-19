// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// winsock2 사용을 위해 아래 코멘트 추가
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

	// 세션 아이디
	int SessionId;
	// 위치
	float X;
	float Y;
	float Z;
	// 회전값
	float Yaw;
	float Pitch;
	float Roll;
	// 속도
	float VX;
	float VY;
	float VZ;
	// 속성
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

	// 소켓 등록 및 설정
	bool InitSocket();
	// 서버와 연결
	bool Connect(const char * pszIP, int nPort);

	//////////////////////////////////////////////////////////////////////////
	// 서버와 통신
	//////////////////////////////////////////////////////////////////////////

	// 서버에 로그인
	bool Login(const FText & Id, const FText & Pw);
	// 초기 캐릭터 등록
	void EnrollCharacterInfo(cCharacter& info);
	// 캐릭터 동기화
	void SendCharacterInfo(cCharacter& info);	
	// 캐릭터 로그아웃
	void LogoutCharacter(int SessionId);	
	// 캐릭터 피격 처리
	void DamagingCharacter(int SessionId);
	// 채팅 
	void SendChat(const int& SessionId, const string& Chat);
	// UDP 테스트용 함수
	char* UdpTest();
	//////////////////////////////////////////////////////////////////////////	

	// 플레이어 컨트롤러 세팅
	void SetPlayerController(ASungminPlayerController* pPlayerController);

	void CloseSocket();

	// FRunnable Thread members	
	FRunnableThread* Thread;
	FThreadSafeCounter StopTaskCounter;

	// FRunnable override 함수
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Exit();

	// 스레드 시작 및 종료
	bool StartListen();
	void StopListen();	

	// 싱글턴 객체 가져오기
	static ClientSocket* GetSingleton()
	{
		static ClientSocket ins;
		return &ins;
	}

private:
	SOCKET	ServerSocket;				// 서버와 연결할 소켓
	SOCKET	UdpServerSocket;
	char 	recvBuffer[MAX_BUFFER];		// 수신 버퍼 스트림	
	char UdpRecvBuffer[MAX_BUFFER];
	cCharactersInfo CharactersInfo;		// 캐릭터 정보
	SOCKADDR_IN	UdpServerAddr;	
	ASungminPlayerController* PlayerController;	// 플레이어 컨트롤러 정보

	string sChat;
	char testChat[MAX_BUFFER];
	cCharactersInfo* RecvCharacterInfo(stringstream& RecvStream);
	string* RecvChat(stringstream& RecvStream);
};


