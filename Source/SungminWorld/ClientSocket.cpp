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

void ClientSocket::SendMyLocation(const FVector& ActorLocation)
{
	location loc;
	loc.x = ActorLocation.X;
	loc.y = ActorLocation.Y;
	loc.z = ActorLocation.Z;
	
	int nSendLen = send(m_Socket, (CHAR*)&loc, sizeof(location), 0);

	if (nSendLen == -1) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		// return FVector();
	}

	// int nRecvLen = recv(socket_, (char*)&loc, 1024, 0);

	// FVector result(loc.x, loc.y, loc.z);

	// return result;
}
