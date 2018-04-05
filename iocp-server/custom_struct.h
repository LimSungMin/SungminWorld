#pragma once

struct location
{
	float x;
	float y;
	float z;
};

struct CharacterInfo
{
	int			SessionId;
	location	loc;
};

struct CharactersInfo
{
	std::map<int, location> m;
};