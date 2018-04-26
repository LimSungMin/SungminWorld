#pragma once

// ��Ƽ����Ʈ ���� ���� define
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
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
// DB ����
#define DB_ADDRESS		"localhost"
#define	DB_PORT			3306
#define DB_ID			"root"
#define DB_PW			"anfrhrl"
#define DB_SCHEMA		"sungminworld"

// IOCP ���� ����ü
struct stSOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuf;
	SOCKET			socket;
	char			messageBuffer[MAX_BUFFER];
	int				recvBytes;
	int				sendBytes;
};

// ��Ŷ ó�� �Լ� ������
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

	// ���� ��� �� ���� ���� ����
	bool Initialize();
	// ���� ����
	void StartServer();
	// �۾� ������ ����
	bool CreateWorkerThread();
	bool CreateUdpThread();
	
	// Ŭ���̾�Ʈ���� �۽�
	static void Send(stSOCKETINFO * pSocket);
	// Ŭ���̾�Ʈ ���� ���
	void Recv(stSOCKETINFO * pSocket);

	// �۾� ������
	void WorkerThread();
	void UdpThread();
	// DB�� �α���
	static void Login(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �ʱ� ���
	static void EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� ��ġ ����ȭ
	static void SyncCharacters(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �α׾ƿ� ó��
	static void LogoutCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �ǰ� ó��
	static void HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	
	// ��ε�ĳ��Ʈ �Լ�
	static void Broadcast(stringstream & SendStream);
	// ä�� ���� �� Ŭ���̾�Ʈ�鿡�� �۽�
	static void BroadcastChat(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// �ٸ� Ŭ���̾�Ʈ�鿡�� �� �÷��̾� ���� ���� ����
	static void BroadcastNewPlayer(cCharactersInfo & player);	

private:
	stSOCKETINFO *	SocketInfo;		// ���� ����
	SOCKET			ListenSocket;	// ���� ���� ����
	SOCKET			UdpListenSocket;// UDP ���� ����
	HANDLE			hIOCP;			// IOCP ��ü �ڵ�
	bool			bAccept;		// ��û ���� �÷���
	bool			bWorkerThread;	// �۾� ������ ���� �÷���
	HANDLE *		hWorkerHandle;	// �۾� ������ �ڵ�		
	HANDLE *		hUdpHandle;		// UDP �ڵ�
	static cCharactersInfo CharactersInfo;	// ������ Ŭ���̾�Ʈ�� ������ ����	
	static map<int, SOCKET> SessionSocket;	// ���Ǻ� ���� ����
	static float			HitPoint;		// Ÿ�� ������
	static DBConnector 	Conn;			// DB Ŀ����
	HANDLE			hSemaphore;
	int				nThreadCnt;

	static CRITICAL_SECTION csPlayers;		// CharactersInfo �Ӱ迵��
	FuncProcess fnProcess[100];

	static void WriteCharactersInfoToSocket(stSOCKETINFO * pSocket);	
};
