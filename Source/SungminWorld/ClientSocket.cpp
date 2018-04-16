// Fill out your copyright notice in the Description page of Project Settings.

#include "ClientSocket.h"
#include <sstream>
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformAffinity.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include <algorithm>
#include <string>
#include "SungminPlayerController.h"

ClientSocket::ClientSocket()
	:StopTaskCounter(0)
{		
}


ClientSocket::~ClientSocket()
{
	delete Thread;
	Thread = nullptr;

	closesocket(ServerSocket);
	WSACleanup();
}

bool ClientSocket::InitSocket()
{
	WSADATA wsaData;
	// 윈속 버전을 2.2로 초기화
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;		
		return false;
	}

	// TCP 소켓 생성
	// m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	// UdpServerSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (ServerSocket == INVALID_SOCKET) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "socket initialize success." << std::endl;
	return true;
}

bool ClientSocket::Connect(const char * pszIP, int nPort)
{
	// 접속할 서버 정보를 저장할 구조체
	SOCKADDR_IN stServerAddr;

	stServerAddr.sin_family = AF_INET;
	// 접속할 서버 포트 및 IP
	stServerAddr.sin_port = htons(nPort);
	stServerAddr.sin_addr.s_addr = inet_addr(pszIP);
	
// 	UdpServerAddr.sin_family = AF_INET;
// 	// 접속할 서버 포트 및 IP
// 	UdpServerAddr.sin_port = htons(UDP_SERVER_PORT);
// 	UdpServerAddr.sin_addr.s_addr = inet_addr(pszIP);

	int nRet = connect(ServerSocket, (sockaddr*)&stServerAddr, sizeof(sockaddr));	
	if (nRet == SOCKET_ERROR) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "Connection success..." << std::endl;

	return true;
}

void ClientSocket::EnrollCharacterInfo(cCharacter & info)
{
	// 캐릭터 정보 직렬화
	stringstream SendStream;
	// 요청 종류
	SendStream << EPacketType::ENROLL_CHARACTER << endl;;
	SendStream << info;

	// 캐릭터 정보 전송
	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);

	if (nSendLen == -1)
	{
		return;
	}
}

void ClientSocket::SendCharacterInfo(cCharacter& info)
{	
	// 캐릭터 정보 직렬화
	stringstream SendStream;
	// 요청 종류
	SendStream << EPacketType::SEND_CHARACTER << endl;;
	SendStream << info;

	// 캐릭터 정보 전송
	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);
		
	if (nSendLen == -1)
	{
		return;
	}		
}

cCharactersInfo * ClientSocket::RecvCharacterInfo(stringstream & RecvStream)
{	
	// 서버에서 캐릭터 정보를 얻어 반환
	RecvStream >> CharactersInfo;
	return &CharactersInfo;		
}

string * ClientSocket::RecvChat(stringstream & RecvStream)
{	
	// 서버에서 채팅 정보를 얻어 반환
	RecvStream >> sChat;
	std::replace(sChat.begin(), sChat.end(), '_', ' ');
	return &sChat;
}

void ClientSocket::LogoutCharacter(int SessionId)
{
	// 서버에게 로그아웃시킬 캐릭터 정보 전송
	stringstream SendStream;
	SendStream << EPacketType::LOGOUT_CHARACTER << endl;
	SendStream << SessionId << endl;

	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);

	if (nSendLen == -1)
	{
		return;
	}
	
	closesocket(ServerSocket);
	WSACleanup();
}

char* ClientSocket::UdpTest()
{
	int nResult;
	char * sendBuffer = (char*)"hello";
	int sendBufferLen = strlen(sendBuffer);
	
	SOCKADDR_IN fromAddr;
	int fromAddrLen = sizeof(fromAddr);
	nResult = sendto(
		UdpServerSocket, sendBuffer, sendBufferLen, 0, (sockaddr*)&UdpServerAddr, sizeof(UdpServerAddr)
	);

	nResult = recvfrom(
		UdpServerSocket, UdpRecvBuffer, MAX_BUFFER, 0, (sockaddr*)&fromAddr, &fromAddrLen
	);

	return UdpRecvBuffer;
}

void ClientSocket::DamagingCharacter(int SessionId)
{
	// 서버에게 데미지를 준 캐릭터 정보 전송
	stringstream SendStream;
	SendStream << EPacketType::HIT_CHARACTER << endl;
	SendStream << SessionId << endl;

	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);
}

void ClientSocket::SendChat(const int& SessionId, const string & Chat)
{
	// 서버에게 채팅 전송
	stringstream SendStream;
	SendStream << EPacketType::CHAT << endl;
	SendStream << SessionId << endl;
	SendStream << Chat << endl;

	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);
}

void ClientSocket::SetPlayerController(ASungminPlayerController * pPlayerController)
{
	// 플레이어 컨트롤러 세팅
	if (pPlayerController)
	{
		PlayerController = pPlayerController;
	}
}

void ClientSocket::CloseSocket()
{	
	closesocket(ServerSocket);
	WSACleanup();
}

bool ClientSocket::Init()
{
	return true;
}

uint32 ClientSocket::Run()
{
	// 초기 init 과정을 기다림
	FPlatformProcess::Sleep(0.03);	
	// recv while loop 시작
	// StopTaskCounter 클래스 변수를 사용해 Thread Safety하게 해줌
	while (StopTaskCounter.GetValue() == 0 && PlayerController != nullptr)
	{
		stringstream RecvStream;
		int PacketType;
		int nRecvLen = recv(
			ServerSocket, (CHAR*)&recvBuffer, MAX_BUFFER, 0
		);
		if (nRecvLen > 0)
		{
			RecvStream << recvBuffer;
			RecvStream >> PacketType;

			switch (PacketType)
			{
			case EPacketType::RECV_CHARACTER:
			{
				PlayerController->RecvWorldInfo(RecvCharacterInfo(RecvStream));
			}
			break;
			case EPacketType::CHAT:
			{
				PlayerController->RecvChat(RecvChat(RecvStream));
			}
			break;
			default:
				break;
			}
		}
	}
	return 0;
}

void ClientSocket::Stop()
{	
	// thread safety 변수를 조작해 while loop 가 돌지 못하게 함
	StopTaskCounter.Increment();
}

void ClientSocket::Exit()
{
}

bool ClientSocket::StartListen()
{
	// 스레드 시작
	if (Thread != nullptr) return false;
	Thread = FRunnableThread::Create(this, TEXT("ClientSocket"), 0, TPri_BelowNormal);
	return (Thread != nullptr);
}

void ClientSocket::StopListen()
{	
	// 스레드 종료
	Stop();
	Thread->WaitForCompletion();
	Thread->Kill();	
	delete Thread;
	Thread = nullptr;	
	StopTaskCounter.Reset();
}
