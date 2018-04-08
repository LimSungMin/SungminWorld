#pragma once
// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <map>
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
	
	// �۾� ������
	void WorkerThread();
	void UdpThread();
	//
	void SyncCharacters(stringstream& RecvStream, stringstream& SendStream);

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
};
