// Fill out your copyright notice in the Description page of Project Settings.

#include "ClientSocket.h"

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
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
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
	location loc;
	loc.x = ActorLocation.X;
	loc.y = ActorLocation.Y;
	loc.z = ActorLocation.Z;

	CharacterInfo info;
	info.SessionId = SessionId;
	info.loc = loc;

	CharactersInfo* ci;
	
	int nSendLen = send(m_Socket, (CHAR*)&info, sizeof(CharacterInfo), 0);
	
	if (nSendLen == -1)
	{
		return -1;
	}
	
	int nRecvLen = recv(m_Socket, (CHAR*)&recvBuffer, 1024, 0);

	if (nRecvLen == -1)
	{
		return -1;
	}
	else {
		ci = (CharactersInfo*)&recvBuffer;
	}		
	return ci->m.size();
		
}
