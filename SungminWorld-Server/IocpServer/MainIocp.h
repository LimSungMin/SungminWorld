#pragma once

// ��Ƽ����Ʈ ���� ���� define
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// winsock2 ����� ���� �Ʒ� �ڸ�Ʈ �߰�
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <map>
#include <vector>
#include <iostream>
#include "CommonClass.h"
#include "DBConnector.h"
#include "IocpBase.h"

using namespace std;

// DB ����
#define DB_ADDRESS		"localhost"
#define	DB_PORT			3306
#define DB_ID			"root"
#define DB_PW			"anfrhrl"
#define DB_SCHEMA		"sungminworld"

class MainIocp : public IocpBase
{
public:
	MainIocp();
	virtual ~MainIocp();
	
	// �۾� ������ ����
	virtual bool CreateWorkerThread() override;
	// �۾� ������
	virtual void WorkerThread() override;
	
	// Ŭ���̾�Ʈ���� �۽�
	static void Send(stSOCKETINFO * pSocket);	

private:
	static cCharactersInfo CharactersInfo;	// ������ Ŭ���̾�Ʈ�� ������ ����	
	static map<int, SOCKET> SessionSocket;	// ���Ǻ� ���� ����
	static float			HitPoint;		// Ÿ�� ������
	static DBConnector 	Conn;			// DB Ŀ����
	static CRITICAL_SECTION csPlayers;		// CharactersInfo �Ӱ迵��

	FuncProcess fnProcess[100];

	// ȸ������
	static void SignUp(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// DB�� �α���
	static void Login(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �ʱ� ���
	static void EnrollCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� ��ġ ����ȭ
	static void SyncCharacters(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �α׾ƿ� ó��
	static void LogoutCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ĳ���� �ǰ� ó��
	static void HitCharacter(stringstream & RecvStream, stSOCKETINFO * pSocket);
	// ä�� ���� �� Ŭ���̾�Ʈ�鿡�� �۽�
	static void BroadcastChat(stringstream & RecvStream, stSOCKETINFO * pSocket);

	// ��ε�ĳ��Ʈ �Լ�
	static void Broadcast(stringstream & SendStream);	
	// �ٸ� Ŭ���̾�Ʈ�鿡�� �� �÷��̾� ���� ���� ����
	static void BroadcastNewPlayer(cCharactersInfo & player);
	// ĳ���� ������ ���ۿ� ���
	static void WriteCharactersInfoToSocket(stSOCKETINFO * pSocket);	
};
