#pragma once

#ifdef COMMONCLASS_EXPORTS
#define COMMONCLASS_API __declspec(dllexport)
#else
#define COMMONCLASS_API __declspec(dllimport)
#endif

#include <iostream>

using namespace std;

#define MAX_CLIENTS 100

enum COMMONCLASS_API EPacketType
{
	ENROLL_CHARACTER,
	SEND_CHARACTER,
	RECV_CHARACTER,
	LOGOUT_CHARACTER,
	HIT_CHARACTER,
	DAMAGED_CHARACTER,
	CHAT
};

class COMMONCLASS_API cCharacter {
public:
	cCharacter();
	~cCharacter();

	// 세션 아이디
	int		SessionId;
	// 위치
	float	X;
	float	Y;
	float	Z;
	// 회전값
	float	Yaw;
	float	Pitch;
	float	Roll;
	// 속성
	bool	IsAlive;
	bool	IsJumping;
	float	HealthValue;


	friend ostream& operator<<(ostream &stream, cCharacter& info)
	{
		stream << info.SessionId << endl;
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Yaw << endl;
		stream << info.Pitch << endl;
		stream << info.Roll << endl;
		stream << info.IsAlive << endl;
		stream << info.IsJumping << endl;
		stream << info.HealthValue << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, cCharacter& info)
	{
		stream >> info.SessionId;
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.Yaw;
		stream >> info.Pitch;
		stream >> info.Roll;
		stream >> info.IsAlive;
		stream >> info.IsJumping;
		stream >> info.HealthValue;

		return stream;
	}
};

class COMMONCLASS_API cCharactersInfo
{
public:
	cCharactersInfo();
	~cCharactersInfo();

	cCharacter WorldCharacterInfo[MAX_CLIENTS];

	friend ostream& operator<<(ostream &stream, cCharactersInfo& info)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			stream << info.WorldCharacterInfo[i] << endl;
		}
		return stream;
	}

	friend istream &operator>>(istream &stream, cCharactersInfo& info)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			stream >> info.WorldCharacterInfo[i];
		}
		return stream;
	}
};

class COMMONCLASS_API CommonClass
{
public:
	CommonClass();
	~CommonClass();
};

