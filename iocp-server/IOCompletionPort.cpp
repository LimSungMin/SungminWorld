#include "stdafx.h"
#include "IOCompletionPort.h"
#include <process.h>

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	IOCompletionPort* pOverlappedEvent = (IOCompletionPort*)p;
	pOverlappedEvent->WorkerThread();
	return 0;
}

IOCompletionPort::IOCompletionPort()
{
	m_bWorkerThread = true;
	m_bAccept = true;
}


IOCompletionPort::~IOCompletionPort()
{
	// winsock �� ����� ������
	WSACleanup();
	// �� ����� ��ü�� ����
	if (m_pSocketInfo)
	{
		delete[] m_pSocketInfo;
		m_pSocketInfo = NULL;
	}

	if (m_pWorkerHandle)
	{
		delete[] m_pWorkerHandle;
		m_pWorkerHandle = NULL;
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
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_listenSocket == INVALID_SOCKET)
	{
		printf_s("[ERROR] ���� ���� ����\n");
		return false;
	}

	// ���� ���� ����
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// ���� ����
	nResult = bind(m_listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] bind ����\n");
		closesocket(m_listenSocket);
		WSACleanup();
		return false;
	}

	// ���� ��⿭ ����
	nResult = listen(m_listenSocket, 5);
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] listen ����\n");
		closesocket(m_listenSocket);
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
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Worker Thread ����
	if (!CreateWorkerThread()) return;

	printf_s("[INFO] ���� ����...\n");

	// Ŭ���̾�Ʈ ������ ����
	while (m_bAccept)
	{		
		clientSocket = WSAAccept(
			m_listenSocket, (struct sockaddr *)&clientAddr, &addrLen, NULL, NULL
		);

		if (clientSocket == INVALID_SOCKET)
		{
			printf_s("[ERROR] Accept ����\n");
			return;
		}

		m_pSocketInfo = new stSOCKETINFO();
		m_pSocketInfo->socket = clientSocket;
		m_pSocketInfo->recvBytes = 0;
		m_pSocketInfo->sendBytes = 0;
		m_pSocketInfo->dataBuf.len = MAX_BUFFER;
		m_pSocketInfo->dataBuf.buf = m_pSocketInfo->messageBuffer;
		flags = 0;

		m_hIOCP = CreateIoCompletionPort(
			(HANDLE)clientSocket, m_hIOCP, (DWORD)m_pSocketInfo, 0
		);

		// ��ø ������ �����ϰ� �Ϸ�� ����� �Լ��� �Ѱ���
		nResult = WSARecv(
			m_pSocketInfo->socket,
			&m_pSocketInfo->dataBuf,
			1,
			&recvBytes,
			&flags,
			&(m_pSocketInfo->overlapped),
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
	m_pWorkerHandle = new HANDLE[nThreadCnt];
	// thread ����
	for (int i = 0; i < nThreadCnt; i++)
	{		
		m_pWorkerHandle[i] = (HANDLE *)_beginthreadex(
			NULL, 0, &CallWorkerThread, this, CREATE_SUSPENDED, &threadId
		);
		if (m_pWorkerHandle[i] == NULL) 
		{
			printf_s("[ERROR] Worker Thread ���� ����\n");
			return false;
		}
		ResumeThread(m_pWorkerHandle[i]);
	}
	printf_s("[INFO] Worker Thread ����...\n");
	return true;
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

	while (m_bWorkerThread)
	{
		/**
		 * �� �Լ��� ���� ��������� WaitingThread Queue �� �����·� ���� ��
		 * �Ϸ�� Overlapped I/O �۾��� �߻��ϸ� IOCP Queue ���� �Ϸ�� �۾��� ������
		 * ��ó���� ��		 
		 */
		bResult = GetQueuedCompletionStatus(m_hIOCP,
			&recvBytes,				// ������ ���۵� ����Ʈ
			(LPDWORD)&pCompletionKey,	// completion key
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
			printf_s("[INFO] �޽��� ����- Bytes : [%d], Msg : [%s]\n",
				pSocketInfo->dataBuf.len, pSocketInfo->dataBuf.buf);

			// Ŭ���̾�Ʈ�� ������ �״�� �۽�			
			nResult = WSASend(
				pSocketInfo->socket,
				&(pSocketInfo->dataBuf),
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

			printf_s("[INFO] �޽��� �۽� - Bytes : [%d], Msg : [%s]\n",
				pSocketInfo->dataBuf.len, pSocketInfo->dataBuf.buf);

			// stSOCKETINFO ������ �ʱ�ȭ
			ZeroMemory(&(pSocketInfo->overlapped), sizeof(OVERLAPPED));
			pSocketInfo->dataBuf.len = MAX_BUFFER;
			pSocketInfo->dataBuf.buf = pSocketInfo->messageBuffer;
 			ZeroMemory(pSocketInfo->messageBuffer, MAX_BUFFER);			
 			pSocketInfo->recvBytes = 0;
 			pSocketInfo->sendBytes = 0;
 			
			dwFlags = 0;			

			// Ŭ���̾�Ʈ�κ��� �ٽ� ������ �ޱ� ���� WSARecv �� ȣ������
			nResult = WSARecv(
				pSocketInfo->socket,
				&(pSocketInfo->dataBuf),
				1,
				&recvBytes,
				&dwFlags,
				(LPWSAOVERLAPPED)&(pSocketInfo->overlapped),
				NULL
			);

			if (nResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
			{
				printf_s("[ERROR] WSARecv ���� : ", WSAGetLastError());
			}
		}
	}
}
