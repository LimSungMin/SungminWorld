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
	bWorkerThread = true;
	bAccept = true;
}


IOCompletionPort::~IOCompletionPort()
{
	// winsock 의 사용을 끝낸다
	WSACleanup();
	// 다 사용한 객체를 삭제
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
	// winsock 2.2 버전으로 초기화
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nResult != 0) 
	{
		printf_s("[ERROR] winsock 초기화 실패\n");
		return false;
	}

	// 소켓 생성
	ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf_s("[ERROR] 소켓 생성 실패\n");
		return false;
	}

	// 서버 정보 설정
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 소켓 설정
	nResult = bind(ListenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] bind 실패\n");
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	// 수신 대기열 생성
	nResult = listen(ListenSocket, 5);
	if (nResult == SOCKET_ERROR)
	{
		printf_s("[ERROR] listen 실패\n");
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	return true;
}

void IOCompletionPort::StartServer()
{
	int nResult;
	// 클라이언트 정보
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	SOCKET clientSocket;
	DWORD recvBytes;
	DWORD flags;

	// Completion Port 객체 생성
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Worker Thread 생성
	if (!CreateWorkerThread()) return;

	printf_s("[INFO] 서버 시작...\n");

	// 클라이언트 접속을 받음
	while (bAccept)
	{		
		clientSocket = WSAAccept(
			ListenSocket, (struct sockaddr *)&clientAddr, &addrLen, NULL, NULL
		);

		if (clientSocket == INVALID_SOCKET)
		{
			printf_s("[ERROR] Accept 실패\n");
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

		// 중첩 소켓을 지정하고 완료시 실행될 함수를 넘겨줌
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
			printf_s("[ERROR] IO Pending 실패 : %d", WSAGetLastError());
			return;
		}
	}

}

bool IOCompletionPort::CreateWorkerThread()
{
	unsigned int threadId;
	// 시스템 정보 가져옴
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	printf_s("[INFO] CPU 갯수 : %d\n", sysInfo.dwNumberOfProcessors);
	// 적절한 작업 스레드의 갯수는 (CPU * 2) + 1
	int nThreadCnt = sysInfo.dwNumberOfProcessors * 2;
	
	// thread handler 선언
	hWorkerHandle = new HANDLE[nThreadCnt];
	// thread 생성
	for (int i = 0; i < nThreadCnt; i++)
	{		
		hWorkerHandle[i] = (HANDLE *)_beginthreadex(
			NULL, 0, &CallWorkerThread, this, CREATE_SUSPENDED, &threadId
		);
		if (hWorkerHandle[i] == NULL) 
		{
			printf_s("[ERROR] Worker Thread 생성 실패\n");
			return false;
		}
		ResumeThread(hWorkerHandle[i]);
	}
	printf_s("[INFO] Worker Thread 시작...\n");
	return true;
}

void IOCompletionPort::WorkerThread()
{		
	// 함수 호출 성공 여부
	BOOL	bResult;
	int		nResult;
	// Overlapped I/O 작업에서 전송된 데이터 크기
	DWORD	recvBytes;
	DWORD	sendBytes;
	// Completion Key를 받을 포인터 변수
	stSOCKETINFO *	pCompletionKey;
	// I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터	
	stSOCKETINFO *	pSocketInfo;
	// 
	DWORD	dwFlags = 0;

	while (bWorkerThread)
	{
		/**
		 * 이 함수로 인해 쓰레드들은 WaitingThread Queue 에 대기상태로 들어가게 됨
		 * 완료된 Overlapped I/O 작업이 발생하면 IOCP Queue 에서 완료된 작업을 가져와
		 * 뒷처리를 함		 
		 */
		bResult = GetQueuedCompletionStatus(hIOCP,
			&recvBytes,				// 실제로 전송된 바이트
			(PULONG_PTR)&pCompletionKey,	// completion key
			(LPOVERLAPPED *)&pSocketInfo,			// overlapped I/O 객체
			INFINITE				// 대기할 시간
		);

		if (!bResult && recvBytes == 0)
		{
			printf_s("[INFO] socket(%d) 접속 끊김\n", pSocketInfo->socket);
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
			CharacterInfo* info = (CharacterInfo*)pSocketInfo->dataBuf.buf;
			
 			printf_s("[INFO] [%d]위치 수신 - X : [%f], Y : [%f], Z : [%f]\n",
 				info->SessionId, info->loc.x, info->loc.y, info->loc.z);

			// 캐릭터의 위치를 저장
			WorldCharacterInfo.m[info->SessionId] = info->loc;			
			
			pSocketInfo->dataBuf.buf = (CHAR*)&WorldCharacterInfo;

			// 다른 클라이언트의 정보를 송신
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
				printf_s("[ERROR] WSASend 실패 : ", WSAGetLastError());
			}

// 			printf_s("[INFO] 메시지 송신 - Bytes : [%d], Msg : [%s]\n",
// 				pSocketInfo->dataBuf.len, pSocketInfo->dataBuf.buf);

			// stSOCKETINFO 데이터 초기화
			ZeroMemory(&(pSocketInfo->overlapped), sizeof(OVERLAPPED));
			pSocketInfo->dataBuf.len = MAX_BUFFER;
			pSocketInfo->dataBuf.buf = pSocketInfo->messageBuffer;
 			ZeroMemory(pSocketInfo->messageBuffer, MAX_BUFFER);			
 			pSocketInfo->recvBytes = 0;
 			pSocketInfo->sendBytes = 0;
 			
			dwFlags = 0;			

			// 클라이언트로부터 다시 응답을 받기 위해 WSARecv 를 호출해줌
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
				printf_s("[ERROR] WSARecv 실패 : ", WSAGetLastError());
			}
		}
	}
}
