#pragma once

#define MAX_CLIENTS 100

struct location
{
	int	sessionId;
	float x;
	float y;
	float z;
};

struct CharacterInfo
{
	int			SessionId;
	location	loc[MAX_CLIENTS];
};

/*struct CharactersInfo
{
	std::map<int, location> m;
};*/