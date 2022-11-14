
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

static
	MySQL:AccLib_g_sDbId = MYSQL_INVALID_HANDLE,
	AccLib_g_sDbTable[32],
	AccLib_g_sDbColumnId[32],
	AccLib_g_sDbColumnName[32],
	AccLib_g_sDbColumnPassword[32]
;

static
	AccLib_g_sAccountMySQLId[MAX_PLAYERS],
	AccLib_g_sAccountName[MAX_PLAYERS][MAX_PLAYER_NAME + 1]
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
	if (!_Valid_Handle(db))
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
	return (AccLib_g_sAccountName[playerid][0] != EOS);
}

/**
 * <summary>
 * Will trying to authenticate user via username and password
 * </summary>
 * 
 * <param name="name"> Player name to be used for authentication </params>
 * <param name="password"> Player password to be used for authentication </params>
**/
stock ERROR_CODES:Account_Login(playerid, password[])
{
	if (!IsPlayerConnected(playerid) || !Account_Exists(name))
	{
		return 0;
	}

	bcrypt_hash(playerid, "AccLib_OnHashed", password, BCRYPT_COST);
	return 1;
}

stock Account_Register(playerid, password[])
{
	if (!IsPlayerConnected(playerid))
	{
		return 0;
	}

	bcrypt_hash(playerid, "AccLib_OnRegister", password, BCRYPT_COST);
	return 1;
}

static _Valid_Handle(MySQL:db)
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

// Internal function
static _Check_Account(playerid, const name[])
{
	new checkQuery[128];
	mysql_format(AccLib_g_sDbId, checkQuery, sizeof(checkQuery), "SELECT %s FROM %s WHERE name = '%e'", AccLib_g_sDbTable, AccLib_g_sDbColumnName, name);
	mysql_tquery(AccLib_g_sDbId, checkQuery, "AccLib_OnChecked", "i", playerid);
	return 1;
}

// internal callbacks
forward AccLib_OnHashed(playerid, hashid);
public AccLib_OnHashed(playerid, hashid)
{
	new passwordHash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(passwordHash, BCRYPT_HASH_LENGTH);

	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));

	new insertQuery[256];
	mysql_format(AccLib_g_sDbId, insertQuery, sizeof(insertQuery), "INSERT INTO %s ('%s', '%s') VALUES ('%e', '%e')", 
		AccLib_g_sDbTable, 
		AccLib_g_sDbColumnName, AccLib_g_sDbColumnPassword,
		name, passwordHash
	);

	mysql_pquery(AccLib_g_sDbId, insertQuery);
	return 1;
}

forward AccLib_OnRegister(playerid, hashid);
public AccLib_OnRegister(playerid, hashid)
{
	new passwordHash[BCRYPT_HASH_LENGTH];
	bcrypt_get_hash(passwordHash, BCRYPT_HASH_LENGTH);

	// Fire the query to do internal checks again
	new checkQuery[128];
	mysql_format(AccLib_g_sDbId, checkQuery, sizeof(checkQuery), "SELECT %s FROM %s WHERE password = '%e'", AccLib_g_sDbColumnId, AccLib_g_sDbTable, passwordHash);
	mysql_tquery(AccLib_g_sDbId, checkQuery, "AccLib_OnVerify", "i", playerid);
	return 1;
}

forward AccLib_OnVerify(playerid);
public AccLib_OnVerify(playerid)
{
	if (cache_num_rows() > 0)
	{
		new id;

		cache_get_value_index_int(0, 0, id);

		if (id != AccLib_g_sAccountMySQLId[playerid])
		{
			return CallLocalFunction("OnAccountLogin", "ii", playerid, 1);
		}
	}
	return CallLocalFunction("OnAccountLogin", "ii", playerid, 0);
}

forward AccLib_OnChecked(playerid);
public AccLib_OnChecked(playerid)
{
	if (cache_num_rows() > 0)
	{
		new name[MAX_PLAYER_NAME + 1],
			resultName[MAX_PLAYER_NAME + 1];

		cache_get_value_index_int(0, 0, AccLib_g_sAccountMySQLId[playerid]);
		cache_get_value_index(0, 1, resultName);

		GetPlayerName(playerid, name, sizeof(name));

		// Need to verify if the results is same as player name
		// I guess this is some "race check" thing.
		if (!strcmp(name, resultName))
		{
			strcopy(AccLib_g_sAccountName[playerid], resultName, MAX_PLAYER_NAME + 1);
			CallLocalFunction("OnAccountChecked", "ii", playerid, 1);
			return;
		}
	}

	// If anything happens during the query, we'll just treat it as user doesn't exists.
	AccLib_g_sAccountName[playerid][0] = EOS;

	// We call the function
	CallLocalFunction("OnAccountChecked", "ii", playerid, 0);
	return;
}

// Hooks
public OnPlayerConnect(playerid)
{
	// reset
	AccLib_g_sAccountMySQLId[playerid] = 0;
	AccLib_g_sAccountName[playerid][0] = EOS;

	// fire query early
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof(name));
	_Check_Account(playerid, name);
	
	#if defined AccLib_OnPlayerConnect
		return AccLib_OnPlayerConnect(playerid);
	#else
		return 1;
	#endif
}

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
