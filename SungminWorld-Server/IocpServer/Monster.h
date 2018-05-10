#pragma once

#include "CommonClass.h"

class Monster
{
public:
	Monster();
	virtual ~Monster();

	// �÷��̾�� �̵�
	void MoveTo(const cCharacter& target);
	// �÷��̾� Ÿ��
	void HitPlayer(cCharacter& target);
	// �ǰ�
	void Damaged(float damage);
	// ����ִ��� ����
	bool IsAlive();
	// ���������� ����
	bool IsAttacking();
	// �÷��̾ �߰� ������ �ִ���
	bool IsPlayerInTraceRange(const cCharacter& target);
	// �÷��̾ Ÿ�� ������ �ִ���
	bool IsPlayerInHitRange(const cCharacter& target);
	// ��ġ ����
	void SetLocation(float x, float y, float z);

	friend ostream& operator<<(ostream &stream, Monster& info)
	{
		stream << info.X << endl;
		stream << info.Y << endl;
		stream << info.Z << endl;
		stream << info.Health << endl;
		stream << info.Id << endl;
		stream << info.bIsAttacking << endl;		

		return stream;
	}

	friend istream& operator>>(istream& stream, Monster& info)
	{
		stream >> info.X;
		stream >> info.Y;
		stream >> info.Z;
		stream >> info.Health;
		stream >> info.Id;
		stream >> info.bIsAttacking;

		return stream;
	}

	float	X;				// X��ǥ
	float	Y;				// Y��ǥ
	float	Z;				// Z��ǥ
	float	Health;			// ü��
	int		Id;				// ���� id
	float	TraceRange;		// �߰� ����
	float	HitRange;		// Ÿ�� ����
	float	MovePoint;		// �̵� ����Ʈ
	float	HitPoint;		// Ÿ�� ����Ʈ	
	bool	bIsAttacking;	// ����������	

private:	
	bool	bIsTracking;	// �߰�������
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
