/* Example are taken from: https://github.com/pBlueG/SA-MP-MySQL/tree/master/example_scripts */

#include <a_samp>

// MySQL configuration
#define		MYSQL_HOST 			"localhost"
#define		MYSQL_USER 			"user"
#define 	MYSQL_PASSWORD 		"password"
#define		MYSQL_DATABASE 		"database"

/* Comment this line to disable some features */
#define ACCLIB_AUTO_FETCH_ACCOUNT
#define ACCLIB_AUTO_KICK_ON_ERROR
#define ACCLIB_ALLOW_MULTI_USER
#define ACCLIB_DEBUG_MODE

#include <account-lib>

#define MAX_WRONG_PASSWORD          3
#define MINIMUM_REQUIRED_PASSWORD   8

static 
	MySQL: g_SQL,
	g_sPlayerLoginAttempts[MAX_PLAYERS];

enum
{
	DIALOG_UNUSED,

	DIALOG_LOGIN,
	DIALOG_REGISTER
};

public OnGameModeInit()
{
	print("Connecting to MySQL service...");

	new MySQLOpt: option_id = mysql_init_options();
	mysql_set_option(option_id, AUTO_RECONNECT, true);
	g_SQL = mysql_connect(MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD, MYSQL_DATABASE, option_id); 

	if (g_SQL == MYSQL_INVALID_HANDLE || mysql_errno(g_SQL) != 0)
	{
		print("MySQL connection failed. Server is shutting down.");
		SendRconCommand("exit");
		return 1;
	}
	print("MySQL connection is successful.");

	// Init the lib
	AccLib_Init(g_SQL, "accounts", "id", "name", "password");
	return 1;
}

main()
{
	printf("Account system by Aiura");
}

public OnPlayerConnect(playerid)
{
	new hour, minute;
	gettime(hour, minute, _);
	SetPlayerTime(playerid, hour, minute);
	TogglePlayerSpectating(playerid, true);
	return 1;
}

public OnAccountFetched(playerid, bool:success)
{
	new 
		string:szDialogFormat[128],
		string:name[MAX_PLAYER_NAME + 1];

	GetPlayerName(playerid, name, sizeof(name));

	if (success)
	{
		format(szDialogFormat, sizeof szDialogFormat, "This account (%s) is registered. Please login by entering your password in the field below:", name);
		ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", szDialogFormat, "Login", "Abort");
	}
	else
	{
		format(szDialogFormat, sizeof szDialogFormat, "Welcome %s, you can register by entering your password in the field below:", name);
		ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Registration", szDialogFormat, "Register", "Abort");
	}
	return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, string:inputtext[])
{
	switch (dialogid)
	{
		case DIALOG_LOGIN: 
		{
			if (IsNull(inputtext))
			{
				if (++ g_sPlayerLoginAttempts[playerid] >= MAX_WRONG_PASSWORD)
				{
					ShowPlayerDialog(playerid, DIALOG_UNUSED, DIALOG_STYLE_MSGBOX, "Login", "You have mistyped your password too often (3 times).", "Okay", "");
					DelayedKick(playerid);
				}
				else
				{
					ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", "Invalid password specified!\nPlease enter your password in the field below:", "Login", "Abort");
				}
				return 1;
			}

			AccLib_LoginPlayer(playerid, inputtext);
		}
		case DIALOG_REGISTER:
		{
			if (strlen(inputtext) <= MINIMUM_REQUIRED_PASSWORD)
			{
				ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Registration", "Your password must be longer than "#MINIMUM_REQUIRED_PASSWORD" characters!\nPlease enter your password in the field below:", "Register", "Abort");
				return 1;
			}

			AccLib_RegisterPlayer(playerid, inputtext);
		}

		default: return 0;
	}
	return 1;
}

public OnAccountLogin(playerid, bool:success)
{
	if (!success)
	{
		if (++ g_sPlayerLoginAttempts[playerid] >= MAX_WRONG_PASSWORD)
		{
			ShowPlayerDialog(playerid, DIALOG_UNUSED, DIALOG_STYLE_MSGBOX, "Login", "You have mistyped your password too often (3 times).", "Okay", "");
			DelayedKick(playerid);
		}
		else
		{
			ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", "Invalid password specified!\nPlease enter your password in the field below:", "Login", "Abort");
		}
	}
	else
	{
		ShowPlayerDialog(playerid, DIALOG_UNUSED, DIALOG_STYLE_MSGBOX, "Login", "You have been successfully logged in.", "Okay", "");
		// Do something here like spawn, or idk.
	}
	return 1;
}

forward _KickPlayerDelayed(playerid);
public _KickPlayerDelayed(playerid)
{
	Kick(playerid);
	return 1;
}


//-----------------------------------------------------

DelayedKick(playerid, time = 500)
{
	SetTimerEx("_KickPlayerDelayed", time, false, "d", playerid);
	return 1;
}
