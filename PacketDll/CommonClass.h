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
	SEND_CHARACTER,
	LOGOUT_CHARACTER
};

class COMMONCLASS_API cCharacter {
public:
	cCharacter();
	~cCharacter();

	// 세션 아이디
	int SessionId;
	// 위치
	float X;
	float Y;
	float Z;
	// 회전값
	float Yaw;
	float Pitch;
	float Roll;

	friend ostream& operator<<(ostream &stream, cCharacter& info)
	{
		stream << info.SessionId << endl;
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Yaw << endl;
		stream << info.Pitch << endl;
		stream << info.Roll << endl;

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

