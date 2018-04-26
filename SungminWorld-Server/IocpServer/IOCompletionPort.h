#pragma once

// 멀티바이트 집합 사용시 define
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// winsock2 사용을 위해 아래 코멘트 추가
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <map>
#include <vector>
#include <iostream>
#include "CommonClass.h"
#include "DBConnector.h"

using namespace std;

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
#define UDP_SERVER_PORT	8001
#define MAX_CLIENTS		100
// DB 정보
#define DB_ADDRESS		"localhost"
#define	DB_PORT			3306
#define DB_ID			"root"
#define DB_PW			"anfrhrl"
#define DB_SCHEMA		"sungminworld"

// IOCP 소켓 구조체
struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

// 패킷 처리 함수 포인터
struct FuncProcess
{
	void(*funcProcessPacket)(stringstream & RecvStream, stSOCKETINFO * pSocket);
	FuncProcess()
	{
		funcProcessPacket = nullptr;
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
	bool CreateUdpThread();
	
	// 클라이언트에게 송신
	static void Send(stSOCKETINFO * pSocket);
	// 클라이언트 수신 대기
	void Recv(stSOCKETINFO * pSocket);

	// 작업 스레드
	void WorkerThread();
	void UdpThread();
	// DB에 로그인
	static void Login(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// 캐릭터 초기 등록
	static void EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// 캐릭터 위치 동기화
	static void SyncCharacters(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// 캐릭터 로그아웃 처리
	static void LogoutCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// 캐릭터 피격 처리
	static void HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	
	// 브로드캐스트 함수
	static void Broadcast(stringstream & SendStream);
	// 채팅 수신 후 클라이언트들에게 송신
	static void BroadcastChat(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// 다른 클라이언트들에게 새 플레이어 입장 정보 보냄
	static void BroadcastNewPlayer(cCharactersInfo & player);	

private:
	stSOCKETINFO *	SocketInfo;		// 소켓 정보
	SOCKET			ListenSocket;	// 서버 리슨 소켓
	SOCKET			UdpListenSocket;// UDP 리슨 소켓
	HANDLE			hIOCP;			// IOCP 객체 핸들
	bool			bAccept;		// 요청 동작 플래그
	bool			bWorkerThread;	// 작업 스레드 동작 플래그
	HANDLE *		hWorkerHandle;	// 작업 스레드 핸들		
	HANDLE *		hUdpHandle;		// UDP 핸들
	static cCharactersInfo CharactersInfo;	// 접속한 클라이언트의 정보를 저장	
	static map<int, SOCKET> SessionSocket;	// 세션별 소켓 저장
	static float			HitPoint;		// 타격 데미지
	static DBConnector 	Conn;			// DB 커넥터
	HANDLE			hSemaphore;
	int				nThreadCnt;

	static CRITICAL_SECTION csPlayers;		// CharactersInfo 임계영역
	FuncProcess fnProcess[100];

	static void WriteCharactersInfoToSocket(stSOCKETINFO * pSocket);	
};
