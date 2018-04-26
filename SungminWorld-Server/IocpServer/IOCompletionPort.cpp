#include "stdafx.h"
#include "IOCompletionPort.h"
#include <process.h>
#include <sstream>
#include <algorithm>
#include <string>

// static 변수 초기화
float IOCompletionPort::HitPoint = 0.1f;
map<int, SOCKET> IOCompletionPort::SessionSocket;
cCharactersInfo IOCompletionPort::CharactersInfo;
DBConnector IOCompletionPort::Conn;
CRITICAL_SECTION IOCompletionPort::csPlayers;

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
	// 멤버 변수 초기화
	bWorkerThread = true;
	bAccept = true;

	InitializeCriticalSection(&csPlayers);

	// DB 접속
	if (Conn.Connect(DB_ADDRESS, DB_ID, DB_PW, DB_SCHEMA, DB_PORT))
	{
		printf_s("[INFO] DB 접속 성공\n");
	}
	else {
		printf_s("[ERROR] DB 접속 실패\n");
	}

	// 패킷 함수 포인터에 함수 지정
	fnProcess[EPacketType::LOGIN].funcProcessPacket = Login;
	fnProcess[EPacketType::ENROLL_PLAYER].funcProcessPacket = EnrollCharacter;
	fnProcess[EPacketType::SEND_PLAYER].funcProcessPacket = SyncCharacters;
	fnProcess[EPacketType::HIT_PLAYER].funcProcessPacket = HitCharacter;
	fnProcess[EPacketType::CHAT].funcProcessPacket = BroadcastChat;
	fnProcess[EPacketType::LOGOUT_PLAYER].funcProcessPacket = LogoutCharacter;
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

	// DB 연결 종료
	Conn.Close();
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
	UdpListenSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (ListenSocket == INVALID_SOCKET || UdpListenSocket == INVALID_SOCKET)
	{
		printf_s("[ERROR] 소켓 생성 실패\n");
		return false;
	}

	// 서버 정보 설정
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	SOCKADDR_IN udpServerAddr;
	udpServerAddr.sin_family = PF_INET;
	udpServerAddr.sin_port = htons(UDP_SERVER_PORT);
	udpServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 소켓 설정
	// boost bind 와 구별짓기 위해 ::bind 사용
	nResult = ::bind(ListenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	nResult = ::bind(UdpListenSocket, (struct sockaddr*)&udpServerAddr, sizeof(SOCKADDR_IN));
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
	if (!CreateUdpThread()) return;

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
	nThreadCnt = sysInfo.dwNumberOfProcessors * 2;

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

bool IOCompletionPort::CreateUdpThread()
{
	unsigned int threadId;
	hUdpHandle = (HANDLE *)_beginthreadex(
		NULL, 0, &CallUdpThread, this, CREATE_SUSPENDED, &threadId
	);
	if (hUdpHandle == NULL)
	{
		printf_s("[ERROR] Udp Thread 생성 실패\n");
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
		printf_s("[ERROR] WSASend 실패 : ", WSAGetLastError());
	}


}

void IOCompletionPort::Recv(stSOCKETINFO * pSocket)
{
	int nResult;
	// DWORD	sendBytes;
	DWORD	dwFlags = 0;

	// stSOCKETINFO 데이터 초기화
	ZeroMemory(&(pSocket->overlapped), sizeof(OVERLAPPED));
	ZeroMemory(pSocket->messageBuffer, MAX_BUFFER);
	pSocket->dataBuf.len = MAX_BUFFER;
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->recvBytes = 0;
	pSocket->sendBytes = 0;

	dwFlags = 0;

	// 클라이언트로부터 다시 응답을 받기 위해 WSARecv 를 호출해줌
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
		printf_s("[ERROR] WSARecv 실패 : ", WSAGetLastError());
	}
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
	DWORD	dwFlags = 0;
	// 패킷 종류
	int PacketType;
	// 클라이언트 정보 역직렬화
	stringstream RecvStream;	

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

		try
		{			
			RecvStream.str(std::string());
			RecvStream << pSocketInfo->dataBuf.buf;
			RecvStream >> PacketType;

			// 패킷 처리
			if (fnProcess[PacketType].funcProcessPacket != nullptr)
			{
				fnProcess[PacketType].funcProcessPacket(RecvStream, pSocketInfo);
			}
			else
			{
				printf_s("[ERROR] 정의 되지 않은 패킷 : %d\n", PacketType);
			}
		}
		catch (const std::exception& e)
		{
			printf_s("[ERROR] 알 수 없는 예외 발생 : %s\n", e.what());
		}
		
		// 클라이언트 대기
		Recv(pSocketInfo);
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

void IOCompletionPort::Login(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	string Id;
	string Pw;

	RecvStream >> Id;
	RecvStream >> Pw;

	printf_s("[INFO] 로그인 시도 {%s}/{%s}\n", Id, Pw);

	stringstream SendStream;
	SendStream << EPacketType::LOGIN << endl;
	SendStream << Conn.SearchAccount(Id, Pw) << endl;

	CopyMemory(pSocket->messageBuffer, (CHAR*)SendStream.str().c_str(), SendStream.str().length());
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->dataBuf.len = SendStream.str().length();

	Send(pSocket);
}

void IOCompletionPort::EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	cCharacter info;
	RecvStream >> info;

	printf_s("[INFO][%d]캐릭터 등록 - X : [%f], Y : [%f], Z : [%f], Yaw : [%f], Alive : [%d], Health : [%f]\n",
		info.SessionId, info.X, info.Y, info.Z, info.Yaw, info.IsAlive, info.HealthValue);

	EnterCriticalSection(&csPlayers);

	cCharacter* pinfo = &CharactersInfo.players[info.SessionId];

	// 캐릭터의 위치를 저장						
	pinfo->SessionId = info.SessionId;
	pinfo->X = info.X;
	pinfo->Y = info.Y;
	pinfo->Z = info.Z;

	// 캐릭터의 회전값을 저장
	pinfo->Yaw = info.Yaw;
	pinfo->Pitch = info.Pitch;
	pinfo->Roll = info.Roll;

	// 캐릭터의 속도를 저장
	pinfo->VX = info.VX;
	pinfo->VY = info.VY;
	pinfo->VZ = info.VZ;

	// 캐릭터 속성
	pinfo->IsAlive = info.IsAlive;
	pinfo->HealthValue = info.HealthValue;
	pinfo->IsAttacking = info.IsAttacking;

	LeaveCriticalSection(&csPlayers);

	SessionSocket[info.SessionId] = pSocket->socket;

	printf_s("[INFO] 클라이언트 수 : %d\n", SessionSocket.size());

	//Send(pSocket);
	BroadcastNewPlayer(CharactersInfo);
}

void IOCompletionPort::SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	cCharacter info;
	RecvStream >> info;

	// 	printf_s("[INFO][%d]정보 수신 - %d\n",
	// 		info.SessionId, info.IsAttacking);	
	EnterCriticalSection(&csPlayers);

	cCharacter * pinfo = &CharactersInfo.players[info.SessionId];

	// 캐릭터의 위치를 저장						
	pinfo->SessionId = info.SessionId;
	pinfo->X = info.X;
	pinfo->Y = info.Y;
	pinfo->Z = info.Z;

	// 캐릭터의 회전값을 저장
	pinfo->Yaw = info.Yaw;
	pinfo->Pitch = info.Pitch;
	pinfo->Roll = info.Roll;

	// 캐릭터의 속도를 저장
	pinfo->VX = info.VX;
	pinfo->VY = info.VY;
	pinfo->VZ = info.VZ;

	pinfo->IsAttacking = info.IsAttacking;

	LeaveCriticalSection(&csPlayers);

	WriteCharactersInfoToSocket(pSocket);
	Send(pSocket);
}

void IOCompletionPort::LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	int SessionId;
	RecvStream >> SessionId;
	printf_s("[INFO] (%d)로그아웃 요청 수신\n", SessionId);
	EnterCriticalSection(&csPlayers);
	CharactersInfo.players[SessionId].IsAlive = false;
	LeaveCriticalSection(&csPlayers);
	SessionSocket.erase(SessionId);
	printf_s("[INFO] 클라이언트 수 : %d\n", SessionSocket.size());
	WriteCharactersInfoToSocket(pSocket);
}

void IOCompletionPort::HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	// 피격 처리된 세션 아이디
	int DamagedSessionId;
	RecvStream >> DamagedSessionId;
	printf_s("[INFO] %d 데미지 받음 \n", DamagedSessionId);
	EnterCriticalSection(&csPlayers);
	CharactersInfo.players[DamagedSessionId].HealthValue -= HitPoint;
	if (CharactersInfo.players[DamagedSessionId].HealthValue < 0)
	{
		// 캐릭터 사망처리
		CharactersInfo.players[DamagedSessionId].IsAlive = false;
	}
	LeaveCriticalSection(&csPlayers);
	WriteCharactersInfoToSocket(pSocket);
	Send(pSocket);
}

void IOCompletionPort::BroadcastChat(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	stSOCKETINFO* client = new stSOCKETINFO;

	int SessionId;
	string Temp;
	string Chat;

	RecvStream >> SessionId;
	getline(RecvStream, Temp);
	Chat += to_string(SessionId) + "_:_";
	while (RecvStream >> Temp)
	{
		Chat += Temp + "_";
	}
	Chat += '\0';

	printf_s("[CHAT] %s\n", Chat);

	stringstream SendStream;
	SendStream << EPacketType::CHAT << endl;
	SendStream << Chat;

	Broadcast(SendStream);
}

void IOCompletionPort::BroadcastNewPlayer(cCharactersInfo & player)
{
	stringstream SendStream;
	SendStream << EPacketType::ENTER_NEW_PLAYER << endl;
	SendStream << player << endl;

	Broadcast(SendStream);
}

void IOCompletionPort::Broadcast(stringstream & SendStream)
{
	stSOCKETINFO* client = new stSOCKETINFO;
	for (const auto& kvp : SessionSocket)
	{
		client->socket = kvp.second;
		CopyMemory(client->messageBuffer, (CHAR*)SendStream.str().c_str(), SendStream.str().length());
		client->dataBuf.buf = client->messageBuffer;
		client->dataBuf.len = SendStream.str().length();

		Send(client);
	}
}

void IOCompletionPort::WriteCharactersInfoToSocket(stSOCKETINFO * pSocket)
{
	stringstream SendStream;

	// 직렬화	
	SendStream << EPacketType::RECV_PLAYER << endl;
	SendStream << CharactersInfo << endl;

	// !!! 중요 !!! data.buf 에다 직접 데이터를 쓰면 쓰레기값이 전달될 수 있음
	CopyMemory(pSocket->messageBuffer, (CHAR*)SendStream.str().c_str(), SendStream.str().length());
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->dataBuf.len = SendStream.str().length();
}
