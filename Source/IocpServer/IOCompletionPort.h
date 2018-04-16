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
	
	void Send(stSOCKETINFO* pSocket);
	void Recv(stSOCKETINFO* pSocket);

	// �۾� ������
	void WorkerThread();
	void UdpThread();
	// ĳ���� �ʱ� ���
	void EnrollCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� ��ġ ����ȭ
	void SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� �α׾ƿ� ó��
	void LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ĳ���� �ǰ� ó��
	void HitCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket);
	// ä�� ���� �� Ŭ���̾�Ʈ�鿡�� �۽�
	void BroadcastChat(stringstream& RecvStream);

private:
	stSOCKETINFO *	SocketInfo;		// ���� ����
	SOCKET			ListenSocket;		// ���� ���� ����
	SOCKET			UdpListenSocket;
	HANDLE			hIOCP;			// IOCP ��ü �ڵ�
	bool			bAccept;			// ��û ���� �÷���
	bool			bWorkerThread;	// �۾� ������ ���� �÷���
	HANDLE *		hWorkerHandle;	// �۾� ������ �ڵ�		
	HANDLE *		hUdpHandle;
	cCharactersInfo CharactersInfo;	// ������ Ŭ���̾�Ʈ�� ������ ����	
	map<int, SOCKET> SessionSocket;
	float			HitPoint;		// Ÿ�� ������

	void WriteCharactersInfoToSocket(stSOCKETINFO* pSocket);	
};
