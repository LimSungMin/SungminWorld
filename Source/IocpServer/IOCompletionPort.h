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

using namespace std;

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
#define UDP_SERVER_PORT	8001
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
	
	void Send(stSOCKETINFO* pSocket);
	void Recv(stSOCKETINFO* pSocket);

	// 작업 스레드
	void WorkerThread();
	void UdpThread();
	// 캐릭터 초기 등록
	void EnrollCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// 캐릭터 위치 동기화
	void SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// 캐릭터 로그아웃 처리
	void LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// 캐릭터 피격 처리
	void HitCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// 채팅 수신 후 클라이언트들에게 송신
	void BroadcastChat(stringstream& RecvStream);

private:
	stSOCKETINFO *	SocketInfo;		// 소켓 정보
	SOCKET			ListenSocket;		// 서버 리슨 소켓
	SOCKET			UdpListenSocket;
	HANDLE			hIOCP;			// IOCP 객체 핸들
	bool			bAccept;			// 요청 동작 플래그
	bool			bWorkerThread;	// 작업 스레드 동작 플래그
	HANDLE *		hWorkerHandle;	// 작업 스레드 핸들		
	HANDLE *		hUdpHandle;
	cCharactersInfo CharactersInfo;	// 접속한 클라이언트의 정보를 저장	
	map<int, SOCKET> SessionSocket;
	float			HitPoint;		// 타격 데미지

	void WriteCharactersInfoToSocket(stSOCKETINFO* pSocket);	
};
