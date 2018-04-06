// Fill out your copyright notice in the Description page of Project Settings.

#include "ClientSocket.h"
#include <sstream>

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
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

	int nRet = connect(ServerSocket, (sockaddr*)&stServerAddr, sizeof(sockaddr));	
	if (nRet == SOCKET_ERROR) {
		// std::cout << "Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	// std::cout << "Connection success..." << std::endl;

	return true;
}

cCharactersInfo* ClientSocket::SyncCharacters(cCharacter info)
{	
	// ĳ���� ���� ����ȭ
	stringstream InputStream;
	InputStream << info;

	// ĳ���� ���� ����
	int nSendLen = send(
		ServerSocket, (CHAR*)InputStream.str().c_str(), InputStream.str().length(), 0
	);
		
	if (nSendLen == -1)
	{
		return nullptr;
	}
		
	// �������� ����
	int nRecvLen = recv(
		ServerSocket, (CHAR*)&recvBuffer, MAX_BUFFER, 0
	);

	if (nRecvLen == -1)
	{
		return nullptr;
	}
	else {		
		// ������ȭ
		stringstream OutputStream;
		OutputStream << recvBuffer;
		OutputStream >> CharactersInfo;

		return &CharactersInfo;
	}			
}
