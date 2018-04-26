#include "stdafx.h"
#include "DBConnector.h"


DBConnector::DBConnector()
{
}


DBConnector::~DBConnector()
{
}

bool DBConnector::Connect(const string & Server, const string & User, const string & Password, const string & Database, const int & Port)
{
	Conn = mysql_init(NULL);
	if (!mysql_real_connect(
		Conn, Server.c_str(), User.c_str(), Password.c_str(), Database.c_str(), Port, NULL, 0)
	)
	{
		printf_s("[DB] DB 접속 실패\n");
		return false;
	}

	return true;
}

void DBConnector::Close()
{
	mysql_close(Conn);
}

bool DBConnector::SearchAccount(const string & Id, const string & Password)
{
	bool bResult = false;
	string sql = "SELECT * FROM sungminworld.playeraccount WHERE id = '";
	sql += Id + "' and pw = '" + Password + "'";

	if (mysql_query(Conn, sql.c_str()))
	{
		printf_s("[DB] 검색 실패\n");
		return false;
	}

	Res = mysql_use_result(Conn);
	Row = mysql_fetch_row(Res);
	if (Row != NULL)
	{
		bResult = true;
	}
	else
	{
		printf_s("[ERROR] 해당 아이디 없음\n");
		bResult = false;
	}
	mysql_free_result(Res);

	return bResult;
}
