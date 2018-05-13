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
#define SERVER_IP		"127.0.0.1"
#define MAX_CLIENTS		100

// 소켓 통신 구조체
struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

// 패킷 정보
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
	ENTER_NEW_PLAYER,
	SIGNUP,
	HIT_MONSTER,
	SYNC_MONSTER,
	SPAWN_MONSTER,
	DESTROY_MONSTER
};

// 플레이어 정보
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

// 플레이어 직렬화/역직렬화 클래스
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

// 몬스터 정보
class Monster
{
public:
	float	X;				// X좌표
	float	Y;				// Y좌표
	float	Z;				// Z좌표
	float	Health;			// 체력
	int		Id;				// 고유 id
	bool	IsAttacking;		// 타격중인지

	friend ostream& operator<<(ostream &stream, Monster& info)
	{
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Health << endl;
		stream << info.Id << endl;
		stream << info.IsAttacking << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, Monster& info)
	{
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.Health;
		stream >> info.Id;
		stream >> info.IsAttacking;

		return stream;
	}
};

// 몬스터 직렬화/역직렬화 클래스
class MonsterSet
{
public:
	map<int, Monster> monsters;

	friend ostream& operator<<(ostream &stream, MonsterSet& info)
	{
		stream << info.monsters.size() << endl;
		for (auto& kvp : info.monsters)
		{
			stream << kvp.first << endl;
			stream << kvp.second << endl;
		}

		return stream;
	}

	friend istream& operator>>(istream& stream, MonsterSet& info)
	{
		int nMonsters = 0;
		int PrimaryId = 0;
		Monster monster;
		info.monsters.clear();

		stream >> nMonsters;
		for (int i = 0; i < nMonsters; i++)
		{
			stream >> PrimaryId;
			stream >> monster;
			info.monsters[PrimaryId] = monster;
		}

		return stream;
	}
};

/**
 * 서버와 접속 및 패킷 처리를 담당하는 클래스
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

	// 회원가입
	bool SignUp(const FText & Id, const FText & Pw);
	// 서버에 로그인
	bool Login(const FText & Id, const FText & Pw);
	// 초기 캐릭터 등록
	void EnrollPlayer(cCharacter& info);
	// 캐릭터 동기화
	void SendPlayer(cCharacter& info);	
	// 캐릭터 로그아웃
	void LogoutPlayer(const int& SessionId);	
	// 캐릭터 피격 처리
	void HitPlayer(const int& SessionId);
	// 몬스터 피격 처리
	void HitMonster(const int& MonsterId);
	// 채팅 
	void SendChat(const int& SessionId, const string& Chat);	
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
	char 	recvBuffer[MAX_BUFFER];		// 수신 버퍼 스트림	
	
	ASungminPlayerController* PlayerController;	// 플레이어 컨트롤러 정보	

	//////////////////////////////////////////////////////////////////////////
	// 역직렬화
	//////////////////////////////////////////////////////////////////////////
	cCharactersInfo CharactersInfo;		// 캐릭터 정보
	cCharactersInfo* RecvCharacterInfo(stringstream& RecvStream);

	string sChat;
	string* RecvChat(stringstream& RecvStream);

	cCharacter	NewPlayer;
	cCharacter* RecvNewPlayer(stringstream& RecvStream);

	MonsterSet	MonsterSetInfo;
	MonsterSet* RecvMonsterSet(stringstream& RecvStream);

	Monster		MonsterInfo;
	Monster*	RecvMonster(stringstream& RecvStream);
	//////////////////////////////////////////////////////////////////////////
};


