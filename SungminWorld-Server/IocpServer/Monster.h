#pragma once

#include "CommonClass.h"

class Monster
{
public:
	Monster();
	virtual ~Monster();

	// 플레이어로 이동
	void MoveTo(const cCharacter& target);
	// 플레이어 타격
	void HitPlayer(cCharacter& target);
	// 피격
	void Damaged(float damage);
	// 살아있는지 여부
	bool IsAlive();
	// 공격중인지 여부
	bool IsAttacking();
	// 한번에 얼마나 이동시킬지 설정
	void SetMovePoint(float point);
	// 한번에 얼마나 때릴지 설정
	void SetHitPoint(float point);
	// 플레이어가 추격 범위에 있는지
	bool IsPlayerInTraceRange(const cCharacter& target);
	// 플레이어가 타격 범위에 있는지
	bool IsPlayerInHitRange(const cCharacter& target);
	// 추격 범위 설정
	void SetTraceRange(float range);
	// 타격 범위 설정
	void SetHitRange(float range);	
	// 위치 설정
	void SetLocation(float x, float y, float z);

	friend ostream& operator<<(ostream &stream, Monster& info)
	{
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Health << endl;
		stream << info.Id << endl;

		return stream;
	}

	friend istream& operator>>(istream& stream, Monster& info)
	{
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.Health;
		stream >> info.Id;

		return stream;
	}

	float	X;				// X좌표
	float	Y;				// Y좌표
	float	Z;				// Z좌표
	float	Health;			// 체력
	int		Id;				// 고유 id

private:	
	float	MovePoint;		// 이동 포인트
	float	HitPoint;		// 타격 포인트
	bool	bIsAttacking;	// 공격중인지
	bool	bIsTracking;	// 추격중인지
	float	TraceRange;		// 추격 범위
	float	HitRange;		// 타격 범위
};

class MonsterSet
{
public:
	map<int, Monster> monsters;

	friend ostream& operator<<(ostream &stream, MonsterSet& info)
	{
		stream << info.monsters.size() << endl;
		for (auto& kvp : info.monsters)
		{
			stream << kvp.first << endl;
			stream << kvp.second << endl;
		}

		return stream;
	}

	friend istream& operator>>(istream& stream, MonsterSet& info)
	{
		int nMonsters = 0;
		int PrimaryId = 0;
		Monster monster;
		info.monsters.clear();

		stream >> nMonsters;
		for (int i = 0; i < nMonsters; i++)
		{
			stream >> PrimaryId;
			stream >> monster;
			info.monsters[PrimaryId] = monster;
		}

		return stream;
	}
};
