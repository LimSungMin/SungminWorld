#include "stdafx.h"
#include "IOCompletionPort.h"
#include <process.h>
#include <sstream>

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	IOCompletionPort* pOverlappedEvent = (IOCompletionPort*)p;
	pOverlappedEvent->WorkerThread();
	return 0;
}

unsigned int WINAPI CallUdpThread(LPVOID p)
{
	IOCompletionPort* pOverlappedEvent = (IOCompletionPort*)p;
	pOverlappedEvent->UdpThread();
	return 0;
}

IOCompletionPort::IOCompletionPort()
{
	// ��� ���� �ʱ�ȭ
	bWorkerThread = true;
	bAccept = true;		

	for (int i = 0; i < MAX_CLIENTS; i++)
	{		
		CharactersInfo.WorldCharacterInfo[i].SessionId = -1;
		CharactersInfo.WorldCharacterInfo[i].X = -1;
		CharactersInfo.WorldCharacterInfo[i].Y= -1;
		CharactersInfo.WorldCharacterInfo[i].Z= -1;
		CharactersInfo.WorldCharacterInfo[i].IsAlive = false;
	}	

	HitPoint = 0.1f;
}


IOCompletionPort::~IOCompletionPort()
{
	// winsock �� ����� ������
	WSACleanup();
	// �� ����� ��ü�� ����
	if (SocketInfo)
	{
		delete[] SocketInfo;
		SocketInfo = NULL;
	}

	if (hWorkerHandle)
	{
		delete[] hWorkerHandle;
		hWorkerHandle = NULL;
	}	
}

bool IOCompletionPort::Initialize()
{
	WSADATA wsaData;
	int nResult;
	// winsock 2.2 �������� �ʱ�ȭ
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nResult != 0) 
	{
		printf_s("[ERROR] winsock �ʱ�ȭ ����\n");
		return false;
	}

	// ���� ����
	ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);	
	UdpListenSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (ListenSocket == INVALID_SOCKET || UdpListenSocket == INVALID_SOCKET)
	{
		printf_s("[ERROR] ���� ���� ����\n");
		return false;
	}

	// ���� ���� ����
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	SOCKADDR_IN udpServerAddr;
	udpServerAddr.sin_family = PF_INET;
	udpServerAddr.sin_port = htons(UDP_SERVER_PORT);
	udpServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// ���� ����
	// boost bind �� �������� ���� ::bind ���
	nResult = ::bind(ListenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	nResult = ::bind(UdpListenSocket, (struct sockaddr*)&udpServerAddr, sizeof(SOCKADDR_IN));
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] bind ����\n");
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	// ���� ��⿭ ����
	nResult = listen(ListenSocket, 5);
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] listen ����\n");
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	return true;
}

void IOCompletionPort::StartServer()
{
	int nResult;
	// Ŭ���̾�Ʈ ����
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	SOCKET clientSocket;
	DWORD recvBytes;
	DWORD flags;

	// Completion Port ��ü ����
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Worker Thread ����
	if (!CreateWorkerThread()) return;
	if (!CreateUdpThread()) return;

	printf_s("[INFO] ���� ����...\n");

	// Ŭ���̾�Ʈ ������ ����
	while (bAccept)
	{		
		clientSocket = WSAAccept(
			ListenSocket, (struct sockaddr *)&clientAddr, &addrLen, NULL, NULL
		);

		if (clientSocket == INVALID_SOCKET)
		{
			printf_s("[ERROR] Accept ����\n");
			return;
		}

		SocketInfo = new stSOCKETINFO();
		SocketInfo->socket = clientSocket;
		SocketInfo->recvBytes = 0;
		SocketInfo->sendBytes = 0;
		SocketInfo->dataBuf.len = MAX_BUFFER;
		SocketInfo->dataBuf.buf = SocketInfo->messageBuffer;
		flags = 0;

		hIOCP = CreateIoCompletionPort(
			(HANDLE)clientSocket, hIOCP, (DWORD)SocketInfo, 0
		);

		// ��ø ������ �����ϰ� �Ϸ�� ����� �Լ��� �Ѱ���
		nResult = WSARecv(
			SocketInfo->socket,
			&SocketInfo->dataBuf,
			1,
			&recvBytes,
			&flags,
			&(SocketInfo->overlapped),
			NULL
		);

		if (nResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			printf_s("[ERROR] IO Pending ���� : %d", WSAGetLastError());
			return;
		}
	}

}

bool IOCompletionPort::CreateWorkerThread()
{
	unsigned int threadId;
	// �ý��� ���� ������
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	printf_s("[INFO] CPU ���� : %d\n", sysInfo.dwNumberOfProcessors);
	// ������ �۾� �������� ������ (CPU * 2) + 1
	int nThreadCnt = sysInfo.dwNumberOfProcessors * 2;
	
	// thread handler ����
	hWorkerHandle = new HANDLE[nThreadCnt];
	// thread ����
	for (int i = 0; i < nThreadCnt; i++)
	{		
		hWorkerHandle[i] = (HANDLE *)_beginthreadex(
			NULL, 0, &CallWorkerThread, this, CREATE_SUSPENDED, &threadId
		);
		if (hWorkerHandle[i] == NULL) 
		{
			printf_s("[ERROR] Worker Thread ���� ����\n");
			return false;
		}
		ResumeThread(hWorkerHandle[i]);
	}
	printf_s("[INFO] Worker Thread ����...\n");
	return true;
}

bool IOCompletionPort::CreateUdpThread()
{
	unsigned int threadId;
	hUdpHandle = (HANDLE *)_beginthreadex(
		NULL, 0, &CallUdpThread, this, CREATE_SUSPENDED, &threadId
	);
	if (hUdpHandle == NULL)
	{
		printf_s("[ERROR] Udp Thread ���� ����\n");
		return false;
	}
	ResumeThread(hUdpHandle);
	return true;
}

void IOCompletionPort::Send(stSOCKETINFO * pSocket)
{
	int nResult;
	DWORD	sendBytes;
	DWORD	dwFlags = 0;

	nResult = WSASend(
		pSocket->socket,
		&(pSocket->dataBuf),
		1,
		&sendBytes,
		dwFlags,
		NULL,
		NULL
	);

	if (nResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		printf_s("[ERROR] WSASend ���� : ", WSAGetLastError());
	}

	// stSOCKETINFO ������ �ʱ�ȭ
	ZeroMemory(&(pSocket->overlapped), sizeof(OVERLAPPED));
	ZeroMemory(pSocket->messageBuffer, MAX_BUFFER);
	pSocket->dataBuf.len = MAX_BUFFER;
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->recvBytes = 0;
	pSocket->sendBytes = 0;

	dwFlags = 0;

	// Ŭ���̾�Ʈ�κ��� �ٽ� ������ �ޱ� ���� WSARecv �� ȣ������
	nResult = WSARecv(
		pSocket->socket,
		&(pSocket->dataBuf),
		1,
		(LPDWORD)&pSocket,
		&dwFlags,
		(LPWSAOVERLAPPED)&(pSocket->overlapped),
		NULL
	);

	if (nResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		printf_s("[ERROR] WSARecv ���� : ", WSAGetLastError());
	}
}

void IOCompletionPort::WorkerThread()
{		
	// �Լ� ȣ�� ���� ����
	BOOL	bResult;
	int		nResult;
	// Overlapped I/O �۾����� ���۵� ������ ũ��
	DWORD	recvBytes;
	DWORD	sendBytes;
	// Completion Key�� ���� ������ ����
	stSOCKETINFO *	pCompletionKey;
	// I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������	
	stSOCKETINFO *	pSocketInfo;
	// 
	DWORD	dwFlags = 0;

	while (bWorkerThread)
	{
		/**
		 * �� �Լ��� ���� ��������� WaitingThread Queue �� �����·� ���� ��
		 * �Ϸ�� Overlapped I/O �۾��� �߻��ϸ� IOCP Queue ���� �Ϸ�� �۾��� ������
		 * ��ó���� ��		 
		 */
		bResult = GetQueuedCompletionStatus(hIOCP,
			&recvBytes,				// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pCompletionKey,	// completion key
			(LPOVERLAPPED *)&pSocketInfo,			// overlapped I/O ��ü
			INFINITE				// ����� �ð�
		);

		if (!bResult && recvBytes == 0)
		{
			printf_s("[INFO] socket(%d) ���� ����\n", pSocketInfo->socket);
			closesocket(pSocketInfo->socket);
			free(pSocketInfo);
			continue;
		}

		pSocketInfo->dataBuf.len = recvBytes;

		if (recvBytes == 0)
		{
			closesocket(pSocketInfo->socket);
			free(pSocketInfo);
			continue;
		}
		else
		{			
			int PacketType;			
			// Ŭ���̾�Ʈ ���� ������ȭ
			stringstream RecvStream;			
			
			RecvStream << pSocketInfo->dataBuf.buf;
			RecvStream >> PacketType;

			switch (PacketType)
			{
			case EPacketType::ENROLL_CHARACTER:
			{
				EnrollCharacter(RecvStream, pSocketInfo);
			}
				break;
			case EPacketType::SEND_CHARACTER:
			{
				SyncCharacters(RecvStream, pSocketInfo);
			}
			break;
			case EPacketType::HIT_CHARACTER:
			{
				HitCharacter(RecvStream, pSocketInfo);
			}
			break;
			case EPacketType::LOGOUT_CHARACTER:
			{
				LogoutCharacter(RecvStream, pSocketInfo);
			}
			break;
			default:
				break;
			}									
				
			Send(pSocketInfo);
		}
	}
}

void IOCompletionPort::UdpThread()
{	
	int nResult;	
	SOCKADDR_IN clientAddr;
	int clientAddrSize = sizeof(clientAddr);	
	char RecvBuffer[MAX_BUFFER];
	char SendBuffer[MAX_BUFFER];
	int PacketType;

	while (bAccept)
	{		
		nResult = recvfrom(
			UdpListenSocket,
			RecvBuffer,
			MAX_BUFFER,
			0,
			(struct sockaddr *)&clientAddr,
			&clientAddrSize
		);
		printf_s("%s\n", RecvBuffer);

// 		stringstream RecvStream;
// 		stringstream SendStream;
// 		RecvStream << RecvBuffer;
// 		RecvStream >> PacketType;
// 
// 		switch (PacketType)
// 		{
// 		case EPacketType::SEND_CHARACTER:
// 		{
// 			SyncCharacters(RecvStream, SendStream);
// 		}
// 		break;
// 		default:		
// 			break;
// 		}		
// 		CopyMemory(SendBuffer, (char*)SendStream.str().c_str(), SendStream.str().length());
		nResult = sendto(
			UdpListenSocket,
			"world",
			6,
			0,
			(struct sockaddr *)&clientAddr,
			clientAddrSize
		);
	}
}

void IOCompletionPort::EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	cCharacter info;
	RecvStream >> info;

	printf_s("[INFO][%d]ĳ���� ��� - X : [%f], Y : [%f], Z : [%f], Yaw : [%f], Roll : [%f], Pitch : [%f]\n",
		info.SessionId, info.X, info.Y, info.Z, info.Yaw, info.Roll, info.Pitch);

	// ĳ������ ��ġ�� ����						
	CharactersInfo.WorldCharacterInfo[info.SessionId].SessionId = info.SessionId;
	CharactersInfo.WorldCharacterInfo[info.SessionId].X = info.X;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Y = info.Y;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Z = info.Z;
	// ĳ������ ȸ������ ����
	CharactersInfo.WorldCharacterInfo[info.SessionId].Yaw = info.Yaw;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Pitch = info.Pitch;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Roll = info.Roll;
	// ĳ���� �Ӽ�
	CharactersInfo.WorldCharacterInfo[info.SessionId].IsAlive = info.IsAlive;
	CharactersInfo.WorldCharacterInfo[info.SessionId].HealthValue = info.HealthValue;

}

void IOCompletionPort::SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	cCharacter info;	
	RecvStream >> info;

// 	printf_s("[INFO][%d]���� ���� - X : [%f], Y : [%f], Z : [%f], Yaw : [%f], Roll : [%f], Pitch : [%f]\n",
// 		info.SessionId, info.X, info.Y, info.Z, info.Yaw, info.Roll, info.Pitch);	
	
	// ĳ������ ��ġ�� ����						
	CharactersInfo.WorldCharacterInfo[info.SessionId].SessionId = info.SessionId;
	CharactersInfo.WorldCharacterInfo[info.SessionId].X = info.X;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Y = info.Y;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Z = info.Z;
	// ĳ������ ȸ������ ����
	CharactersInfo.WorldCharacterInfo[info.SessionId].Yaw = info.Yaw;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Pitch = info.Pitch;
	CharactersInfo.WorldCharacterInfo[info.SessionId].Roll = info.Roll;		

	WriteCharactersInfoToSocket(pSocket);
}

void IOCompletionPort::LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	int SessionId;
	RecvStream >> SessionId;
	printf_s("[INFO] (%d)�α׾ƿ� ��û ����\n", SessionId);	

	CharactersInfo.WorldCharacterInfo[SessionId].IsAlive = false;

	WriteCharactersInfoToSocket(pSocket);
}

void IOCompletionPort::HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
{	
	// �ǰ� ó���� ���� ���̵�
	int DamagedSessionId;
	RecvStream >> DamagedSessionId;

	CharactersInfo.WorldCharacterInfo[DamagedSessionId].HealthValue -= HitPoint;
	if (CharactersInfo.WorldCharacterInfo[DamagedSessionId].HealthValue < 0)
	{
		// ĳ���� ���ó��
		CharactersInfo.WorldCharacterInfo[DamagedSessionId].IsAlive = false;
	}	

	WriteCharactersInfoToSocket(pSocket);
}

void IOCompletionPort::WriteCharactersInfoToSocket(stSOCKETINFO * pSocket)
{
	stringstream SendStream;

	// ����ȭ	
	SendStream << EPacketType::RECV_CHARACTER << endl;
	SendStream << CharactersInfo << endl;

	// !!! �߿� !!! data.buf ���� ���� �����͸� ���� �����Ⱚ�� ���޵� �� ����
	CopyMemory(pSocket->messageBuffer, (CHAR*)SendStream.str().c_str(), SendStream.str().length());
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->dataBuf.len = SendStream.str().length();
}
