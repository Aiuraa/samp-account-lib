/*
	SA:MP Account Lib
	Version: 1.0.0
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

#include <YSI_Core\y_utils>

// Simple logging system
#if defined ACCLIB_DEBUG_MODE
	#define accLib_LogDebug(%0) printf("[AccLib Debug]: "%0)
#else
	#define accLib_LogDebug(%0);
#endif

#define accLib_LogWarning(%0) printf("[AccLib Warning]: "%0)
#define accLib_LogError(%0) printf("[AccLib Error]: "%0)

// Public functions that you can use
forward OnAccountFetched(playerid, bool:success);
forward OnAccountLogin(playerid, bool:success);
forward OnAccountRegister(playerid);
forward OnAccountLogout(playerid);

// For people who are using some ancient ysi i guess.
#if !defined MemSet
	#define MemSet memset
#endif

// From pawn-stdlib, for compatibility only.
#if !defined strcopy
stock strcopy(dest[], const source[], maxlength = sizeof (dest))
{
	return strcat((dest[0] = EOS, dest), source, maxlength);
}
#endif

// Core init functions
static stock
	MySQL:accLib_g_sDbId = MYSQL_INVALID_HANDLE,
	string:accLib_g_sDbTable[32],
	string:accLib_g_sDbColumnId[32],
	string:accLib_g_sDbColumnName[32],
	string:accLib_g_sDbColumnPassword[32]
;

// We only need these for players
static stock
	accLib_g_sAccountMySQLId[MAX_PLAYERS],
	string:accLib_g_sAccountName[MAX_PLAYERS][MAX_PLAYER_NAME],
	string:accLib_g_sAccountPassword[MAX_PLAYERS][BCRYPT_HASH_LENGTH],
	bool:accLib_g_sLoggedIn[MAX_PLAYERS]
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

	accLib_g_sDbId = db;
	accLib_LogDebug("Linked connection to MySQL handle ID: %d", _:db);

	if (IsNull(table))
	{
		accLib_LogWarning("Creating `accounts` tables since table arguments is empty.");
		mysql_pquery(db, "CREATE TABLE IF NOT EXISTS accounts");
		strcopy(accLib_g_sDbTable, "accounts");
	}
	else
	{
		strcopy(accLib_g_sDbTable, table);
	}

	if (IsNull(columnId))
	{
		accLib_LogWarning("Creating `id` column in `%s` tables since columnId arguments is empty.", accLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN id INT NOT NULL AUTO_INCREMENT PRIMARY KEY");
		strcopy(accLib_g_sDbColumnId, "id");
	}
	else 
	{
		strcopy(accLib_g_sDbColumnId, columnId);
	}

	if (IsNull(columnName))
	{
		accLib_LogWarning("Creating `name` column in `%s` tables since columnName arguments is empty.", accLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN name VARCHAR(24) NOT NULL");
		strcopy(accLib_g_sDbColumnName, "name");
	}
	else
	{
		strcopy(accLib_g_sDbColumnName, columnName);
	}

	if (IsNull(columnPassword))
	{
		accLib_LogWarning("Creating `name` column in `%s` tables since columnPassword arguments is empty.", accLib_g_sDbTable);
		mysql_pquery(db, "ALTER TABLE accounts ADD COLUMN password VARCHAR(60) NOT NULL");
		strcopy(accLib_g_sDbColumnPassword, "password");
	}
	else
	{
		strcopy(accLib_g_sDbColumnPassword, columnPassword);
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
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, MAX_PLAYER_NAME + 1);
	accLib_LogDebug("Success, player %d name is \"%s\"", playerid, name);

	strcopy(accLib_g_sAccountName[playerid], name, MAX_PLAYER_NAME + 1);

	accLib_LogDebug("Prepare to fetching accounts..");
	new queryCheckFmt[128];
	mysql_format(accLib_g_sDbId, queryCheckFmt, sizeof(queryCheckFmt), "SELECT %s, %s, %s FROM %s WHERE %s = '%e'",
		accLib_g_sDbColumnId,
		accLib_g_sDbColumnName,
		accLib_g_sDbColumnPassword,
		accLib_g_sDbTable,
		accLib_g_sDbColumnName,
		accLib_g_sAccountName[playerid]
	);

	accLib_LogDebug("Fetching accounts now!");
	mysql_tquery(accLib_g_sDbId, queryCheckFmt, "accLib_AccountFetched__", "i", playerid);
	return 1;
}

stock bool:Account_Exists(playerid)
{
	new bool:ret = (IsPlayerConnected(playerid) && accLib_g_sAccountMySQLId[playerid] != 0);
	accLib_LogDebug("Account_Exists(%d) fired, returns %d", playerid, _:ret);
	return ret;
}

stock bool:Account_IsLoggedIn(playerid)
{
	new bool:ret = (Account_Exists(playerid) && accLib_g_sLoggedIn[playerid]);
	accLib_LogDebug("Account_IsLoggedIn(%d) fired, returns %d", playerid, _:ret);
	return ret;
}

stock AccLib_LoginPlayer(playerid, const string:password[])
{
	if (!Account_Exists(playerid))
	{
		accLib_LogDebug("AccLib_LoginPlayer called when user doesn't exists");
		return 0;
	}

	#if !defined ACCLIB_ALLOW_MULTI_USER
	if (accLib_g_sLoggedIn[playerid])
	{
		accLib_LogDebug("Player %s(%d) trying to request login that the accounts already been logged-in before", accLib_g_sAccountName[playerid], playerid);
		return 0;
	}
	#endif

	accLib_LogDebug("Verifying password for player %s(%d)...", accLib_g_sAccountName[playerid], playerid);
	bcrypt_verify(playerid, "accLib_VerifyPassword__", password, accLib_g_sAccountPassword[playerid]);
	return 1;
}

stock AccLib_RegisterPlayer(playerid, const string:password[])
{
	if (Account_Exists(playerid))
	{
		accLib_LogDebug("AccLib_RegisterPlayer called when user does exists");
		return 0;
	}

	accLib_LogDebug("Hashing password for player %s(%d)...", accLib_g_sAccountName[playerid], playerid);
	bcrypt_hash(playerid, "accLib_HashPassword__", password, BCRYPT_COST);
	return 1;
}

stock AccLib_LogoutPlayer(playerid)
{
	accLib_LogDebug("Clearing account data since player logout...");
	accLib_ClearData__(playerid);

	accLib_LogDebug("Calling \"OnAccountLogout\" callback...");
	CallLocalFunction("OnAccountLogout", "i", playerid);

	#if defined ACCLIB_AUTO_FETCH_ACCOUNT
		// Refetch account for future login
		accLib_LogDebug("Fetching account for future login.");
		AccLib_FetchAccount(playerid);
	#endif
	return 1;
}

// Get & Set
stock AccLib_GetAccountId(playerid)
{
	if (!Account_IsLoggedIn(playerid))
	{
		return INVALID_ACCOUNT_ID;
	}
	return (accLib_g_sAccountMySQLId[playerid]);
}

stock AccLib_GetAccountName(playerid, string:name[], len = sizeof(name))
{
	if (!Account_IsLoggedIn(playerid))
	{
		return 0;
	}

	strcopy(name, accLib_g_sAccountName[playerid], len);
	return 1;
}

stock AccLib_SetAccountName(playerid, const string:name[])
{
	if (!Account_IsLoggedIn(playerid))
	{
		return 0;
	}

	strcopy(accLib_g_sAccountName[playerid], name, MAX_PLAYER_NAME + 1);
	return 1;
}

stock AccLib_SetAccountPassword(playerid, const string:password[])
{
	if (!Account_IsLoggedIn(playerid))
	{
		accLib_LogDebug("AccLib_SetAccountPassword called when user not logged in");
		return 0;
	}

	accLib_LogDebug("Hashing password for player %s(%d)...", accLib_g_sAccountName[playerid], playerid);
	bcrypt_hash(playerid, "accLib_HashSetPassword__", password, BCRYPT_COST);
	return 1;
}

// Internal callback, shouldn't be used.
forward accLib_AccountFetched__(playerid);
forward accLib_VerifyPassword__(playerid, bool:success);
forward accLib_HashPassword__(playerid, hashid);
forward accLib_FetchAfterInsert__(playerid);
forward accLib_HashSetPassword__(playerid,hashid);

public accLib_AccountFetched__(playerid)
{
	accLib_LogDebug("Preparing race check...");
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));

	if (!strcmp(name, accLib_g_sAccountName[playerid], .length = MAX_PLAYER_NAME + 1))
	{
		accLib_LogDebug("Race check success! (%s == %s)", name, accLib_g_sAccountName[playerid]);
		accLib_LogDebug("Checking if account fetched...");

		new
			success = !!cache_num_rows();
		
		accLib_LogDebug("Success, the result is %d", success);

		// set the key to id for faster query search (when needed)
		accLib_LogDebug("Store the results into the variable...");
		cache_get_value_index_int(0, 0, accLib_g_sAccountMySQLId[playerid]);
		cache_get_value_index(0, 1, accLib_g_sAccountName[playerid], MAX_PLAYER_NAME + 1);
		cache_get_value_index(0, 2, accLib_g_sAccountPassword[playerid], BCRYPT_HASH_LENGTH);
		
		accLib_LogDebug("Unset the cache, since we don't need it");
		cache_unset_active();

		// call the real callback
		accLib_LogDebug("Everything is done, we can safely call \"OnAccountFetched\" callback here");
		CallLocalFunction(#OnAccountFetched, "ii", playerid, success);
	}
	else
	{
		#if defined ACCLIB_AUTO_KICK_ON_ERROR
			accLib_LogDebug("Kicking player since the race check failed (%s != %s)", name, accLib_g_sAccountName[playerid]);
			Kick(playerid);
		#endif

		accLib_LogError("Player %s(%d) race check invalid (%s != %s)", name, playerid, name, accLib_g_sAccountName[playerid]);
	}
	return 0;
}

public accLib_VerifyPassword__(playerid, bool:success)
{
	accLib_LogDebug("Checking successful attempt to verify password...");
	if (success)
	{
		accLib_LogDebug("Succeeded verifying password for player %s(%d)", accLib_g_sAccountName[playerid], playerid);

		// remove everything related to password.
		MemSet(accLib_g_sAccountPassword[playerid], 0, BCRYPT_HASH_LENGTH);

		// set logged in to true.
		accLib_g_sLoggedIn[playerid] = true;
		CallLocalFunction("OnAccountLogin", "ii", playerid, 1);
	}
	else
	{
		CallLocalFunction("OnAccountLogin", "ii", playerid, 0);
	}
	return 1;
}

public accLib_HashPassword__(playerid, hashid)
{
	accLib_LogDebug("Getting hash from previous results...");
	new hash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(hash, sizeof(hash));
	accLib_LogDebug("Done, the hash is `*********`");

	accLib_LogDebug("Preparing race check...");
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));

	if (!strcmp(name, accLib_g_sAccountName[playerid], .length = MAX_PLAYER_NAME + 1))
	{
		accLib_LogDebug("Race check success! (%s == %s)", name, accLib_g_sAccountName[playerid]);
		accLib_LogDebug("Inserting the data to the database...");

		new fmtInsert[255];
		mysql_format(accLib_g_sDbId, fmtInsert, sizeof(fmtInsert), "INSERT INTO %e (%e, %e) VALUES ('%e', CAST('%e' AS BINARY))", 
			accLib_g_sDbTable,
			accLib_g_sDbColumnName,
			accLib_g_sDbColumnPassword,
			name,
			hash
		);
		
		accLib_LogDebug("Firing the query now!");
		mysql_tquery(accLib_g_sDbId, fmtInsert, "accLib_FetchAfterInsert__", "i", playerid);
	}
	else
	{
		#if defined ACCLIB_AUTO_KICK_ON_ERROR
			Kick(playerid);
		#endif

		accLib_LogError("Player %s(%d) race check invalid (%s != %s)", name, playerid, name, accLib_g_sAccountName[playerid]);
	}
	return 1;
}

public accLib_FetchAfterInsert__(playerid)
{
	accLib_LogDebug("Clearing account data...");
	accLib_ClearData__(playerid);

	accLib_LogDebug("Fetching the account again");
	AccLib_FetchAccount(playerid);
	return 1;
}

public accLib_HashSetPassword__(playerid,hashid)
{
	accLib_LogDebug("Getting hash from previous results...");
	new string:hash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(hash, sizeof(hash));
	accLib_LogDebug("Done, the hash is `*********`");


	accLib_LogDebug("Preparing race check...");
	new string:name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));

	if (!strcmp(name, accLib_g_sAccountName[playerid], .length = MAX_PLAYER_NAME + 1))
	{
		accLib_LogDebug("Race check success! (%s == %s)", name, accLib_g_sAccountName[playerid]);
		accLib_LogDebug("Inserting the data to the database...");

		new string:queryInsertFmt[128];
		mysql_format(accLib_g_sDbId, queryInsertFmt, sizeof(queryInsertFmt), "UPDATE %s SET %s = '%e' WHERE %s = '%e'",
			accLib_g_sDbTable,
			accLib_g_sDbColumnPassword,
			hash,
			accLib_g_sDbColumnName,
			name
		);
		
		accLib_LogDebug("Firing the query now!");
		mysql_tquery(accLib_g_sDbId, queryInsertFmt);
	}
	else
	{
		#if defined ACCLIB_AUTO_KICK_ON_ERROR
			Kick(playerid);
		#endif

		accLib_LogError("Player %s(%d) race check invalid (%s != %s)", name, playerid, name, accLib_g_sAccountName[playerid]);
	}
	return 1;
}

static accLib_IsValidHandle__(MySQL:db)
{
	if (db == MYSQL_INVALID_HANDLE || mysql_errno(db) != 0)
	{
		return 0;
	}

	if (accLib_g_sDbId != MYSQL_INVALID_HANDLE)
	{
		return 0;
	}
	return 1;
}

static accLib_ClearData__(playerid)
{
	MemSet(accLib_g_sAccountName[playerid], EOS, MAX_PLAYER_NAME + 1);
	MemSet(accLib_g_sAccountPassword[playerid], EOS, BCRYPT_HASH_LENGTH);

	accLib_g_sAccountMySQLId[playerid] = 0;
	accLib_g_sLoggedIn[playerid] = false;
	return 1;
}

#if defined ACCLIB_AUTO_FETCH_ACCOUNT
	public OnPlayerConnect(playerid)
	{
		if (!accLib_g_sLoggedIn[playerid])
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
