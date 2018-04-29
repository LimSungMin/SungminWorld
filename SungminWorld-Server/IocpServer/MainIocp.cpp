#include "stdafx.h"
#include "MainIocp.h"
#include <process.h>
#include <sstream>
#include <algorithm>
#include <string>

// static 변수 초기화
float				MainIocp::HitPoint = 0.1f;
map<int, SOCKET>	MainIocp::SessionSocket;
cCharactersInfo		MainIocp::CharactersInfo;
DBConnector			MainIocp::Conn;
CRITICAL_SECTION	MainIocp::csPlayers;
MonsterSet			MainIocp::MonstersInfo;

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	MainIocp* pOverlappedEvent = (MainIocp*)p;
	pOverlappedEvent->WorkerThread();
	return 0;
}

unsigned int WINAPI CallMonsterThread(LPVOID p)
{
	MainIocp* pOverlappedEvent = (MainIocp*)p;
	pOverlappedEvent->MonsterManagementThread();
	return 0;
}

MainIocp::MainIocp()
{
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
	fnProcess[EPacketType::SIGNUP].funcProcessPacket = SignUp;
	fnProcess[EPacketType::LOGIN].funcProcessPacket = Login;
	fnProcess[EPacketType::ENROLL_PLAYER].funcProcessPacket = EnrollCharacter;
	fnProcess[EPacketType::SEND_PLAYER].funcProcessPacket = SyncCharacters;
	fnProcess[EPacketType::HIT_PLAYER].funcProcessPacket = HitCharacter;
	fnProcess[EPacketType::CHAT].funcProcessPacket = BroadcastChat;
	fnProcess[EPacketType::LOGOUT_PLAYER].funcProcessPacket = LogoutCharacter;
	fnProcess[EPacketType::HIT_MONSTER].funcProcessPacket = HitMonster;
}


MainIocp::~MainIocp()
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

void MainIocp::StartServer()
{
	CreateMonsterManagementThread();
	IocpBase::StartServer();
}

bool MainIocp::CreateWorkerThread()
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

void MainIocp::Send(stSOCKETINFO * pSocket)
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

void MainIocp::CreateMonsterManagementThread()
{
	unsigned int threadId;

	MonsterHandle = (HANDLE *)_beginthreadex(
		NULL, 0, &CallMonsterThread, this, CREATE_SUSPENDED, &threadId
	);
	if (MonsterHandle == NULL)
	{
		printf_s("[ERROR] Monster Thread 생성 실패\n");
		return;
	}
	ResumeThread(MonsterHandle);

	printf_s("[INFO] Monster Thread 시작...\n");
}

void MainIocp::MonsterManagementThread()
{
	// 몬스터 초기화
	InitializeMonsterSet();
	int count = 0;	
	// 로직 시작
	while (true)
	{
		for (auto & kvp : MonstersInfo.monsters)
		{
			auto & monster = kvp.second;
			for (auto & player : CharactersInfo.players)
			{
				// 플레이어나 몬스터가 죽어있을 땐 무시
				if (!player.second.IsAlive || !monster.IsAlive())
					continue;

				if (monster.IsPlayerInHitRange(player.second) && !monster.bIsAttacking)
				{
					monster.HitPlayer(player.second);
					continue;
				}

				if (monster.IsPlayerInTraceRange(player.second) && !monster.bIsAttacking)
				{
					monster.MoveTo(player.second);
					continue;
				}
			}
		}

		count++;
		// 0.5초마다 클라이언트에게 몬스터 정보 전송
		if (count > 15)
		{			
			stringstream SendStream;
			SendStream << EPacketType::SYNC_MONSTER << endl;
			SendStream << MonstersInfo << endl;

			count = 0;
			Broadcast(SendStream);
		}
		
		Sleep(33);
	}
}

void MainIocp::InitializeMonsterSet()
{
	// 몬스터 초기화	
	Monster mFields;

	mFields.X = -5746;
	mFields.Y = 3736;
	mFields.Z = 7362;
	mFields.Health = 100.0f;
	mFields.Id = 1;
	mFields.MovePoint = 10.f;
	MonstersInfo.monsters[mFields.Id] = mFields;

	mFields.X = -5136;
	mFields.Y = 1026;
	mFields.Z = 7712;
	mFields.Id = 2;
	MonstersInfo.monsters[mFields.Id] = mFields;

	mFields.X = -3266;
	mFields.Y = 286;
	mFields.Z = 8232;
	mFields.Id = 3;
	MonstersInfo.monsters[mFields.Id] = mFields;

	mFields.X = -156;
	mFields.Y = 326;
	mFields.Z = 8352;
	mFields.Id = 4;
	MonstersInfo.monsters[mFields.Id] = mFields;
}

void MainIocp::WorkerThread()
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
			// 패킷 종류
			int PacketType;
			// 클라이언트 정보 역직렬화
			stringstream RecvStream;

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

void MainIocp::SignUp(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	string Id;
	string Pw;

	RecvStream >> Id;
	RecvStream >> Pw;

	printf_s("[INFO] 회원가입 시도 {%s}/{%s}\n", Id, Pw);

	stringstream SendStream;
	SendStream << EPacketType::SIGNUP << endl;
	SendStream << Conn.SignUpAccount(Id, Pw) << endl;

	CopyMemory(pSocket->messageBuffer, (CHAR*)SendStream.str().c_str(), SendStream.str().length());
	pSocket->dataBuf.buf = pSocket->messageBuffer;
	pSocket->dataBuf.len = SendStream.str().length();

	Send(pSocket);
}

void MainIocp::Login(stringstream & RecvStream, stSOCKETINFO * pSocket)
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

void MainIocp::EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
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
	BroadcastNewPlayer(info);
}

void MainIocp::SyncCharacters(stringstream& RecvStream, stSOCKETINFO* pSocket)
{
	cCharacter info;
	RecvStream >> info;

	// 	 	printf_s("[INFO][%d]정보 수신 - %d\n",
	// 	 		info.SessionId, info.IsAttacking);	
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

void MainIocp::LogoutCharacter(stringstream& RecvStream, stSOCKETINFO* pSocket)
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

void MainIocp::HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket)
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

void MainIocp::BroadcastChat(stringstream& RecvStream, stSOCKETINFO* pSocket)
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

void MainIocp::HitMonster(stringstream & RecvStream, stSOCKETINFO * pSocket)
{
	// 몬스터 피격 처리
	int MonsterId;
	RecvStream >> MonsterId;
	MonstersInfo.monsters[MonsterId].Damaged(30.f);

	if (!MonstersInfo.monsters[MonsterId].IsAlive())
	{
		stringstream SendStream;
		SendStream << EPacketType::DESTROY_MONSTER << endl;
		SendStream << MonstersInfo.monsters[MonsterId] << endl;

		Broadcast(SendStream);

		MonstersInfo.monsters.erase(MonsterId);
	}

	// 다른 플레이어에게 브로드캐스트
	/*stringstream SendStream;
	SendStream << EPacketType::HIT_MONSTER << endl;
	SendStream << MonstersInfo << endl;

	Broadcast(SendStream);*/
}

void MainIocp::BroadcastNewPlayer(cCharacter & player)
{
	stringstream SendStream;
	SendStream << EPacketType::ENTER_NEW_PLAYER << endl;
	SendStream << player << endl;

	Broadcast(SendStream);
}

void MainIocp::Broadcast(stringstream & SendStream)
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

void MainIocp::WriteCharactersInfoToSocket(stSOCKETINFO * pSocket)
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
