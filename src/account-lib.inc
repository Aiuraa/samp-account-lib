/*
	SA:MP Account Lib
	Version: 0.1.1
	Author: Ebi/Aiura

	Copyright (C) 2022  Ebi/Aiura

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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

#if defined ACCLIB_DEBUG_MODE
	#define accLib_LogDebug(%0 printf("[AccLib Debug]: "%0
#else
	#define accLib_LogDebug(%0)
#endif

#define string: 

#define accLib_LogWarning(%0 printf("[AccLib Warning]: "%0
#define accLib_LogError(%0 printf("[AccLib Error]: "%0

forward OnAccountFetched(playerid, bool:success);
forward OnAccountLogin(playerid, bool:success);
forward OnAccountRegister(playerid);
forward OnAccountLogout(playerid);

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
	string:AccLib_g_sDbTable[32],
	string:AccLib_g_sDbColumnId[32],
	string:AccLib_g_sDbColumnName[32],
	string:AccLib_g_sDbColumnPassword[32]
;

// We only need these for players
static stock
	ORM:AccLib_g_sAccountInstance[MAX_PLAYERS],
	AccLib_g_sAccountMySQLId[MAX_PLAYERS],
	string:AccLib_g_sAccountName[MAX_PLAYERS][MAX_PLAYER_NAME],
	string:AccLib_g_sAccountPassword[MAX_PLAYERS][BCRYPT_HASH_LENGTH],
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
stock AccLib_Init(MySQL:db = MYSQL_DEFAULT_HANDLE, const string:table[] = "", const string:columnId[] = "", const string:columnName[] = "", const string:columnPassword[] = "")
{
	if (!accLib_IsValidHandle__(db))
	{
		accLib_LogError("Cannot established connection MySQL handle ID: %d\nNote: make sure to connect MySQL first before using AccLib_Init.", _:db);
		return 0;
	}

	AccLib_g_sDbId = db;
	accLib_LogDebug("Linked connection to MySQL handle ID: %d", _:db);

	if (IsNull(table))
	{
		accLib_LogWarning("Creating `accounts` tables since table arguments is empty.");
		mysql_pquery(db, "CREATE TABLE IF NOT EXISTS accounts");
		strcopy(AccLib_g_sDbTable, "accounts");
	}
	else
	{
		strcopy(AccLib_g_sDbTable, table);
	}

	if (IsNull(columnId))
	{
		accLib_LogWarning("Creating `id` column in `%s` tables since columnId arguments is empty.", AccLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN id INT NOT NULL AUTO_INCREMENT PRIMARY KEY");
		strcopy(AccLib_g_sDbColumnId, "id");
	}
	else 
	{
		strcopy(AccLib_g_sDbColumnId, columnId);
	}

	if (IsNull(columnName))
	{
		accLib_LogWarning("Creating `name` column in `%s` tables since columnName arguments is empty.", AccLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN name VARCHAR(24) NOT NULL");
		strcopy(AccLib_g_sDbColumnName, "name");
	}
	else
	{
		strcopy(AccLib_g_sDbColumnName, columnName);
	}

	if (IsNull(columnPassword))
	{
		accLib_LogWarning("Creating `name` column in `%s` tables since columnPassword arguments is empty.", AccLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN password BINARY(60) NOT NULL");
		strcopy(AccLib_g_sDbColumnPassword, "password");
	}
	else
	{
		strcopy(AccLib_g_sDbColumnPassword, columnPassword);
	}

	accLib_LogDebug("Linking table `%s`", table);
	accLib_LogDebug("Linking column %s for table `%s`", columnId, table);
	accLib_LogDebug("Linking column %s for table `%s`", columnName, table);
	accLib_LogDebug("Linking column %s for table `%s`", columnPassword, table);
	return 1;
}

stock AccLib_FetchAccount(playerid)
{
	accLib_LogDebug("Getting player %d name...", playerid);
	GetPlayerName(playerid, AccLib_g_sAccountName[playerid], MAX_PLAYER_NAME + 1);
	accLib_LogDebug("Success, player %d name is \"%s\"", playerid, AccLib_g_sAccountName[playerid]);

	accLib_LogDebug("Creating new ORM instances..");
	new ORM:instance = AccLib_g_sAccountInstance[playerid] = orm_create(AccLib_g_sDbTable, AccLib_g_sDbId);

	accLib_LogDebug("Linking table and rows to the ORM instances...");
	orm_addvar_int(instance, AccLib_g_sAccountMySQLId[playerid], AccLib_g_sDbColumnId);
	orm_addvar_string(instance, AccLib_g_sAccountName[playerid], MAX_PLAYER_NAME + 1, AccLib_g_sDbColumnName);
	orm_addvar_string(instance, AccLib_g_sAccountPassword[playerid], BCRYPT_HASH_LENGTH, AccLib_g_sDbColumnPassword);

	accLib_LogDebug("Setting key to `%s` for fetching data", AccLib_g_sDbColumnName);
	orm_setkey(instance, AccLib_g_sDbColumnName);

	accLib_LogDebug("Fetching accounts now!");
	orm_load(instance, "accLib_AccountFetched__", "i", playerid);
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

stock AccLib_LoginPlayer(playerid, string:password[])
{
	if (!Account_Exists(playerid))
	{
		return 0;
	}

	#if !defined ACCLIB_ALLOW_MULTI_USER
	if (AccLib_g_sLoggedIn[playerid])
	{
		return 0;
	}
	#endif

	bcrypt_verify(playerid, "accLib_VerifyPassword__", password, AccLib_g_sAccountPassword[playerid]);
	MemSet(password, 0, strlen(password));
	return 1;
}

stock AccLib_RegisterPlayer(playerid, string:password[])
{
	if (Account_Exists(playerid))
	{
		return 0;
	}

	bcrypt_hash(playerid, "accLib_HashPassword__", password, BCRYPT_COST);
	MemSet(password, 0, strlen(password));
	return 1;
}

stock AccLib_LogoutPlayer(playerid)
{
	accLib_ClearData__(playerid);
	CallLocalFunction("OnAccountLogout", "i", playerid);

	#if defined ACCLIB_AUTO_FETCH_ACCOUNT
		// Refetch account for future login
		AccLib_FetchAccount(playerid);
	#endif
	return 1;
}

// Internal callback, shouldn't be used.
forward accLib_AccountFetched__(playerid);
forward accLib_VerifyPassword__(playerid, bool:success);
forward accLib_HashPassword__(playerid, hashid);
forward accLib_FetchAfterInsert__(playerid);

public accLib_AccountFetched__(playerid)
{
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));

	if (!strcmp(name, AccLib_g_sAccountName[playerid], .length = MAX_PLAYER_NAME + 1))
	{
		new
			ORM:instance = AccLib_g_sAccountInstance[playerid],
			success = (orm_errno(instance) == ERROR_OK) ? 1 : 0;

		// set the key to id for faster query search (when needed)
		orm_setkey(instance, AccLib_g_sDbColumnId);

		// call the real callback
		CallLocalFunction(#OnAccountFetched, "ii", playerid, success);
	}
	else
	{
		#if defined ACCLIB_AUTO_KICK_ON_ERROR
			Kick(playerid);
		#endif

		accLib_LogError("Player %s(%d) race check invalid (%s != %s)", name, playerid, name, AccLib_g_sAccountName[playerid]);
	}
	return 0;
}

public accLib_VerifyPassword__(playerid, bool:success)
{
	if (success)
	{
		// remove everything related to password.
		orm_delvar(AccLib_g_sAccountName[playerid], "password");
		MemSet(AccLib_g_sAccountPassword[playerid], 0, BCRYPT_HASH_LENGTH);

		// set logged in to true.
		AccLib_g_sLoggedIn[playerid] = true;
	}
	return CallLocalFunction("OnAccountLogin", "ii", playerid, _:success);
}

public accLib_HashPassword__(playerid, hashid)
{
	new hash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(hash, sizeof(hash));

	new playerName[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, playerName, sizeof(playerName));

	orm_save(AccLib_g_sAccountInstance[playerid], "accLib_FetchAfterInsert__", "i", playerid);
	return 1;
}

public accLib_FetchAfterInsert__(playerid)
{
	accLib_ClearData__(playerid);
	AccLib_FetchAccount(playerid);
	return 1;
}

static accLib_IsValidHandle__(MySQL:db)
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

static accLib_ClearData__(playerid)
{
	new ORM:instance = AccLib_g_sAccountInstance[playerid];

	orm_delvar(instance, "id");
	orm_delvar(instance, "name");
	orm_delvar(instance, "password");

	AccLib_g_sAccountMySQLId[playerid] = 0;
	MemSet(AccLib_g_sAccountName[playerid], EOS, MAX_PLAYER_NAME + 1);
	MemSet(AccLib_g_sAccountPassword[playerid], EOS, BCRYPT_HASH_LENGTH);

	orm_destroy(AccLib_g_sAccountName[playerid]);
	AccLib_g_sLoggedIn[playerid] = false;
	return 1;
}

#if defined ACCLIB_AUTO_FETCH_ACCOUNT
	public OnPlayerConnect(playerid)
	{
		if (!AccLib_g_sLoggedIn[playerid])
		{
			AccLib_FetchAccount(playerid);
		}
		#if defined accLib_OnPlayerConnect
			return accLib_OnPlayerConnect(playerid);
		#else
			return 1;
		#endif
	}

	#if defined _ALS_OnPlayerConnect
		#undef OnPlayerConnect
	#else
		#define _ALS_OnPlayerConnect
	#endif

	#define OnPlayerConnect accLib_OnPlayerConnect
	#if defined accLib_OnPlayerConnect
		forward accLib_OnPlayerConnect(playerid);
	#endif
#endif

public OnPlayerDisconnect(playerid, reason)
{
	accLib_ClearData__(playerid);
	#if defined accLib_OnPlayerDisconnect
		return accLib_OnPlayerDisconnect(playerid, reason);
	#else
		return 1;
	#endif
}
#if defined _ALS_OnPlayerDisconnect
	#undef OnPlayerDisconnect
#else
	#define _ALS_OnPlayerDisconnect
#endif

#define OnPlayerDisconnect accLib_OnPlayerDisconnect
#if defined accLib_OnPlayerDisconnect
	forward accLib_OnPlayerDisconnect(playerid, reason);
#endif

// no longer using it, it was meant for internal only.
#undef accLib_LogDebug
#undef accLib_LogWarning
#undef accLib_LogError
