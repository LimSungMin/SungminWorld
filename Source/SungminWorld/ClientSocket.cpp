// Fill out your copyright notice in the Description page of Project Settings.

#include "ClientSocket.h"
#include "SungminWorldGameMode.h"
#include <sstream>
#include <process.h>
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformAffinity.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"

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
	// ���� ������ 2.2�� �ʱ�ȭ
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;		
		return false;
	}

	// TCP ���� ����
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
	// ������ ���� ������ ������ ����ü
	SOCKADDR_IN stServerAddr;

	stServerAddr.sin_family = AF_INET;
	// ������ ���� ��Ʈ �� IP
	stServerAddr.sin_port = htons(nPort);
	stServerAddr.sin_addr.s_addr = inet_addr(pszIP);
	
// 	UdpServerAddr.sin_family = AF_INET;
// 	// ������ ���� ��Ʈ �� IP
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

void ClientSocket::SendCharacterInfo(cCharacter& info)
{	
	// ĳ���� ���� ����ȭ
	stringstream SendStream;
	// ��û ����
	SendStream << EPacketType::SEND_CHARACTER << endl;;
	SendStream << info;

	// ĳ���� ���� ����
	int nSendLen = send(
		ServerSocket, (CHAR*)SendStream.str().c_str(), SendStream.str().length(), 0
	);
		
	if (nSendLen == -1)
	{
		return;
	}
		
	/*// �������� ����
	int nRecvLen = recv(
		ServerSocket, (CHAR*)&recvBuffer, MAX_BUFFER, 0
	);

	if (nRecvLen == -1)
	{
		return nullptr;
	}
	else {		
		// ������ȭ
		stringstream RecvStream;
		RecvStream << recvBuffer;
		RecvStream >> CharactersInfo;

		return &CharactersInfo;
	}		*/		
}

cCharactersInfo * ClientSocket::RecvCharacterInfo(stringstream & RecvStream)
{	
	RecvStream >> CharactersInfo;
	return &CharactersInfo;		
}

void ClientSocket::LogoutCharacter(int SessionId)
{
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


void ClientSocket::SetGameMode(ASungminWorldGameMode * pGameMode)
{
	GameMode = pGameMode;
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
	// �ʱ� init ������ ��ٸ�
	FPlatformProcess::Sleep(0.03);
	// ���Ӹ�带 ������
	ASungminWorldGameMode * LocalGameMode = nullptr;
	if (GameMode != nullptr)
	{
		LocalGameMode = GameMode;
	}
	// recv while loop ����
	// StopTaskCounter Ŭ���� ������ ����� Thread Safety�ϰ� ����
	while (StopTaskCounter.GetValue() == 0 && LocalGameMode != nullptr)
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
			case EPacketType::DAMAGED_CHARACTER:
			{

			}
			break;
			case EPacketType::RECV_CHARACTER:
			{
				LocalGameMode->SyncCharactersInfo(RecvCharacterInfo(RecvStream));
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
	// thread safety ������ ������ while loop �� ���� ���ϰ� ��
	StopTaskCounter.Increment();
}

void ClientSocket::Exit()
{
}

bool ClientSocket::StartListen()
{
	// ������ ����
	if (Thread != nullptr) return false;
	Thread = FRunnableThread::Create(this, TEXT("ClientSocket"), 0, TPri_BelowNormal);
	return (Thread != nullptr);
}

void ClientSocket::StopListen()
{	
	// ������ ����
	Stop();
	Thread->WaitForCompletion();
	delete Thread;
	Thread = nullptr;	
	StopTaskCounter.Reset();
}
