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
	// 윈속 버전을 2.2로 초기화
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRet != 0) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;		
		return false;
	}

	// TCP 소켓 생성
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
	// 접속할 서버 정보를 저장할 구조체
	SOCKADDR_IN stServerAddr;

	stServerAddr.sin_family = AF_INET;
	// 접속할 서버 포트 및 IP
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
