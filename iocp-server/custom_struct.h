#pragma once

struct location
{
	float x;
	float y;
	float z;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x;
		ar & y;
		ar & z;
	}
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