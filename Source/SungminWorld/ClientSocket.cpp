// Fill out your copyright notice in the Description page of Project Settings.

#include "ClientSocket.h"
#include <sstream>

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
	closesocket(m_Socket);
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
	m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_Socket == INVALID_SOCKET) {
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

	int nRet = connect(m_Socket, (sockaddr*)&stServerAddr, sizeof(sockaddr));	
	if (nRet == SOCKET_ERROR) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "Connection success..." << std::endl;

	return true;
}

int ClientSocket::SendMyLocation(const int& SessionId, const FVector& ActorLocation)
{
	// ��ġ���� ����
	cLocation loc;
	loc.SessionId = SessionId;
	loc.X = ActorLocation.X;
	loc.Y = ActorLocation.Y;
	loc.Z = ActorLocation.Z;

	// ��ġ���� ����
	int nSendLen = send(m_Socket, (CHAR*)&loc, sizeof(cLocation), 0);
		
	if (nSendLen == -1)
	{
		return -1;
	}
		
	// �������� ����
	int nRecvLen = recv(m_Socket, (CHAR*)&recvBuffer, MAX_BUFFER, 0);

	if (nRecvLen == -1)
	{
		return -1;
	}
	else {		
		// ������ȭ
		stringstream OutputStream;
		OutputStream << recvBuffer;
		OutputStream >> CharactersInfo;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			int ssId = CharactersInfo.WorldCharacterInfo[i].SessionId;
			if (ssId != -1)
			{
				return CharactersInfo.WorldCharacterInfo[i].X;
			}
		}
		return -1;
	}			
}
