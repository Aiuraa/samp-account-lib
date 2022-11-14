
#if defined _INC_account_lib
	#endinput
#endif

#define _INC_account_lib

#include <a_samp>
#include <a_mysql>
#include <samp_bcrypt>

#if !defined IsNull
	#define IsNull(%0) ((%0[(%0[0])=='\1'])=='\0')
#endif

#if !defined MemSet
// By Slice
// http://forum.sa-mp.com/showthread.php?p=1606781
stock MemSet(aArray[], iValue, iSize = sizeof(aArray)) {
	new iAddress;

	#emit LOAD.S.pri 12
	#emit STOR.S.pri iAddress

	iSize *= 4;

	while (iSize > 0) {
		if (iSize >= 4096) {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 4096
			iSize -= 4096;
			iAddress += 4096;
		} else if (iSize >= 1024) {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 1024
			iSize -= 1024;
			iAddress += 1024;
		} else if (iSize >= 256) {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 256
			iSize -= 256;
			iAddress += 256;
		} else if (iSize >= 64) {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 64
			iSize -= 64;
			iAddress += 64;
		} else if (iSize >= 16) {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 16
			iSize -= 16;
			iAddress += 16;
		} else {
			#emit LOAD.S.alt iAddress
			#emit LOAD.S.pri iValue
			#emit FILL 4
			iSize -= 4;
			iAddress += 4;
		}
	}
	#pragma unused aArray
}
#endif

// Core init functions
static stock
	MySQL:AccLib_g_sDbId = MYSQL_INVALID_HANDLE,
	AccLib_g_sDbTable[32],
	AccLib_g_sDbColumnId[32],
	AccLib_g_sDbColumnName[32],
	AccLib_g_sDbColumnPassword[32]
;

// We only need these for players
static stock
	AccLib_g_sAccountMySQLId[MAX_PLAYERS],
	AccLib_g_sAccountName[MAX_PLAYERS][MAX_PLAYER_NAME],
	AccLib_g_sAccountPassword[MAX_PLAYERS][BCRYPT_HASH_LENGTH],
	bool:AccLib_g_sLoggedIn[MAX_PLAYERS]
;

/**
 * <summary>
 * Core function in order for user-lib to work
 * </summary>
 * 
 * <param name="db">MySQL database handle (optional)</param>
 * <param name="table"> Database existing table (optional)</param>
 * <param name="columnName"> Database existing column name (optional)</param>
 * <param name="columnPassword"> Database existing column password (optional)</param>
 * 
 * <returns> Will return 0 if MySQL handle is invalid, otherwise will return 1 </returns>
 * 
 * <remarks>
 * Using the function without specifying paramerers will trying to create `accounts` table
 * with `id`, `name`, and `password` for default colums.
 * 
 * You need to fill paramerers if you have existing `table` structures, otherwise it will
 * result huge mess
 * </remarks>
**/
stock AccLib_Init(MySQL:db = MYSQL_DEFAULT_HANDLE, const table[] = "", const columnId[] = "", const columnName[] = "", const columnPassword[] = "")
{
	if (!__AccLib_IsValidHandle(db))
	{
		return 0;
	}

	AccLib_g_sDbId = db;

	if (IsNull(table))
	{
		mysql_pquery(db, "CREATE TABLE IF NOT EXISTS accounts");
		strcopy(AccLib_g_sDbTable, "accounts");
	}
	else
	{
		strcopy(AccLib_g_sDbTable, table);
	}

	if (IsNull(columnId))
	{
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN id INT NOT NULL AUTO_INCREMENT PRIMARY KEY");
		strcopy(AccLib_g_sDbColumnId, "id");
	}
	else 
	{
		strcopy(AccLib_g_sDbColumnId, columnId);
	}

	if (IsNull(columnName))
	{
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN name VARCHAR(24) NOT NULL");
		strcopy(AccLib_g_sDbColumnName, "name");
	}
	else
	{
		strcopy(AccLib_g_sDbColumnName, columnName);
	}

	if (IsNull(columnPassword))
	{
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN password BINARY(60) NOT NULL");
		strcopy(AccLib_g_sDbColumnPassword, "password");
	}
	else
	{
		strcopy(AccLib_g_sDbColumnPassword, columnPassword);
	}
	return 1;
}

stock bool:Account_Exists(playerid)
{
	return (IsPlayerConnected(playerid) && AccLib_g_sAccountMySQLId[playerid] != 0);
}

stock bool:Account_IsLoggedIn(playerid)
{
	return (Account_Exists(playerid) && AccLib_g_sLoggedIn[playerid]);
}

/**
 * <summary>
 * Will trying to authenticate user via input password
 * </summary>
 * 
 * <param name="playerid"> Playerid to get the name for authentication </params>
 * <param name="password"> Input password to be used for authentication </params>
**/
stock Account_Login(playerid, password[])
{
	if (!Account_Exists(playerid))
	{
		return 0;
	}

	#if !defined ACCOUNT_ALLOW_MULTI_USER
	if (AccLib_g_sLoggedIn[playerid])
	{
		return 0;
	}
	#endif

	bcrypt_verify(playerid, "__AccLib_OnVerifyPassword", password, AccLib_g_sAccountPassword[playerid]);
	MemSet(password, 0, strlen(password));
	return 1;
}

/**
 * <summary>
 * Will register user to the database
 * </summary>
 * 
 * <param name="playerid"> Playerid to get the name for registration </params>
 * <param name="password"> Input password to be used for registration </params>
**/
stock Account_Register(playerid, password[])
{
	if (Account_Exists(playerid))
	{
		return 0;
	}

	bcrypt_hash(playerid, "__AccLib_OnHashPassword", password, BCRYPT_COST);
	MemSet(password, 0, strlen(password));
	return 1;
}

/**
 * <summary>
 * Logout player from current session
 * </summary>
 * 
 * <param name="playerid"> Playerid to get the current session </params>
**/
stock Account_Logout(playerid)
{
	AccLib_g_sAccountMySQLId[playerid] = 0;
	MemSet(AccLib_g_sAccountName[playerid], 0, MAX_PLAYER_NAME);

	AccLib_g_sLoggedIn[playerid] = false;
	CallLocalFunction("OnAccountLogout", "i", playerid);

	// Refetch account for future login
	__AccLib_FetchAccount(playerid);
	return 1;
}

// Internal functions
stock __AccLib_IsValidHandle(MySQL:db)
{
	if (db == MYSQL_INVALID_HANDLE || mysql_errno(db) != 0)
	{
		return 0;
	}

	if (AccLib_g_sDbId != MYSQL_INVALID_HANDLE)
	{
		return 0;
	}
	return 1;
}

stock __AccLib_FetchAccount(playerid)
{
	new playerName[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, playerName, sizeof(playerName));

	new checkQuery[128];
	mysql_format(AccLib_g_sDbId, checkQuery, sizeof(checkQuery), "SELECT %s, %s, %s FROM %s WHERE %s = '%e'",
		AccLib_g_sDbColumnId,
		AccLib_g_sDbColumnName,
		AccLib_g_sDbColumnPassword,
		AccLib_g_sDbTable,
		AccLib_g_sDbColumnName,
		playerName
	);

	mysql_tquery(AccLib_g_sDbId, checkQuery, "__AccLib_OnFetchAccount", "i", playerid);
	return;
}

// Internal callbacks
forward __AccLib_OnFetchAccount(playerid);
public __AccLib_OnFetchAccount(playerid)
{
	if (cache_num_rows() > 0)
	{
		new playerName[MAX_PLAYER_NAME + 1];

		GetPlayerName(playerid, playerName, sizeof(playerName));
		cache_get_value_index_int(0, 0, AccLib_g_sAccountMySQLId[playerid]);
		cache_get_value_index(0, 1, AccLib_g_sAccountName[playerid], MAX_PLAYER_NAME);
		cache_get_value_index(0, 2, AccLib_g_sAccountPassword[playerid], BCRYPT_HASH_LENGTH);

		if (!strcmp(playerName, AccLib_g_sAccountName[playerid]))
		{
			return CallLocalFunction("OnAccountChecked", "ii", playerid, 1);
		}
	}
	return CallLocalFunction("OnAccountChecked", "ii", playerid, 0); 
}

forward __AccLib_OnHashPassword(playerid, hashid);
public __AccLib_OnHashPassword(playerid, hashid)
{
	new hash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(hash, sizeof(hash));

	new playerName[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, playerName, sizeof(playerName));

	new insertQuery[256];
	mysql_format(AccLib_g_sDbId, insertQuery, sizeof(insertQuery), "INSERT INTO %s ('%s', '%s') VALUES ('%e', '%e')",
		AccLib_g_sDbColumnName,
		AccLib_g_sDbColumnPassword,
		playerName,
		hash
	);

	mysql_pquery(AccLib_g_sDbId, insertQuery);
	CallLocalFunction("OnAccountRegister", "i", playerid);
	return 1;
}

forward __AccLib_OnVerifyPassword(playerid, bool:success);
public __AccLib_OnVerifyPassword(playerid, bool:success)
{
	if (success)
	{
		MemSet(AccLib_g_sAccountPassword[playerid], 0, BCRYPT_HASH_LENGTH);
		AccLib_g_sLoggedIn[playerid] = true;
	}
	return CallLocalFunction("OnAccoutVerified", "ii", playerid, _:success);
}

// Hook callbacks
public OnPlayerConnect(playerid)
{
	__AccLib_FetchAccount(playerid);
	#if defined AccLib_OnPlayerConnect
		return AccLib_OnPlayerConnect(playerid);
	#else
		return 1;
	#endif
}

public OnPlayerDisconnect(playerid, reason)
{
	AccLib_g_sAccountMySQLId[playerid] = 0;
	MemSet(AccLib_g_sAccountName[playerid], 0, MAX_PLAYER_NAME);
	MemSet(AccLib_g_sAccountPassword[playerid], 0, BCRYPT_HASH_LENGTH);
	AccLib_g_sLoggedIn[playerid] = false;

	#if defined AccLib_OnPlayerDisconnect
		return AccLib_OnPlayerDisconnect(playerid, reason);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerDisconnect
	#undef OnPlayerDisconnect
#else
	#define _ALS_OnPlayerDisconnect
#endif

#define OnPlayerDisconnect AccLib_OnPlayerDisconnect
#if defined AccLib_OnPlayerDisconnect
	forward AccLib_OnPlayerDisconnect(playerid, reason);
#endif

// ALS thing
#if defined _ALS_OnPlayerConnect
	#undef OnPlayerConnect
#else
	#define _ALS_OnPlayerConnect
#endif

#define OnPlayerConnect AccLib_OnPlayerConnect
#if defined AccLib_OnPlayerConnect
	forward AccLib_OnPlayerConnect(playerid);
#endif
