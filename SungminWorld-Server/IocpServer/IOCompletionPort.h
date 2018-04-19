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

	// ���� ��� �� ���� ���� ����
	bool Initialize();
	// ���� ����
	void StartServer();
	// �۾� ������ ����
	bool CreateWorkerThread();
	bool CreateUdpThread();
	
	// Ŭ���̾�Ʈ���� �۽�
	void Send(stSOCKETINFO* pSocket);
	// Ŭ���̾�Ʈ ���� ���
	void Recv(stSOCKETINFO* pSocket);

	// �۾� ������
	void WorkerThread();
	void UdpThread();
	// DB�� �α���
	void Login(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� �ʱ� ���
	void EnrollCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� ��ġ ����ȭ
	void SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� �α׾ƿ� ó��
	void LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� �ǰ� ó��
	void HitCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	
	// ��ε�ĳ��Ʈ �Լ�
	void Broadcast(stringstream& SendStream);
	// ä�� ���� �� Ŭ���̾�Ʈ�鿡�� �۽�
	void BroadcastChat(stringstream& RecvStream);
	// �ٸ� Ŭ���̾�Ʈ�鿡�� �� �÷��̾� ���� ���� ����
	void BroadcastNewPlayer(cCharactersInfo& player);

private:
	stSOCKETINFO *	SocketInfo;		// ���� ����
	SOCKET			ListenSocket;	// ���� ���� ����
	SOCKET			UdpListenSocket;// UDP ���� ����
	HANDLE			hIOCP;			// IOCP ��ü �ڵ�
	bool			bAccept;		// ��û ���� �÷���
	bool			bWorkerThread;	// �۾� ������ ���� �÷���
	HANDLE *		hWorkerHandle;	// �۾� ������ �ڵ�		
	HANDLE *		hUdpHandle;		// UDP �ڵ�
	cCharactersInfo CharactersInfo;	// ������ Ŭ���̾�Ʈ�� ������ ����	
	map<int, SOCKET> SessionSocket;	// ���Ǻ� ���� ����
	float			HitPoint;		// Ÿ�� ������
	DBConnector 	Conn;			// DB Ŀ����
	HANDLE			hSemaphore;
	int				nThreadCnt;

	CRITICAL_SECTION csPlayers;		// CharactersInfo �Ӱ迵��

	void WriteCharactersInfoToSocket(stSOCKETINFO* pSocket);	
};
