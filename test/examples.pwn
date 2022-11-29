/* Example dialog are taken from: https://github.com/pBlueG/SA-MP-MySQL/tree/master/example_scripts */

#include <a_samp>

/* Comment this line to disable some features */
#define ACCLIB_AUTO_FETCH_ACCOUNT
#define ACCLIB_AUTO_KICK_ON_ERROR
#define ACCLIB_ALLOW_MULTI_USER

#include <account-lib>

#define MAX_WRONG_PASSWORD          3
#define MINIMUM_REQUIRED_PASSWORD   8

static 
	g_sPlayerLoginAttempts[MAX_PLAYERS];

enum
{
	DIALOG_UNUSED,

	DIALOG_LOGIN,
	DIALOG_REGISTER
};

public OnGameModeInit()
{
	printf("Account system by Aiura");
	return 1;
}

public OnAccountFetched(playerid, bool:success)
{
	if (success)
	{
		format(string, sizeof string, "This account (%s) is registered. Please login by entering your password in the field below:", Player[playerid][Name]);
		ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", string, "Login", "Abort");
	}
	else
	{
		format(string, sizeof string, "Welcome %s, you can register by entering your password in the field below:", Player[playerid][Name]);
		ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Registration", string, "Register", "Abort");
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
			return 1;
		}
		case DIALOG_REGISTER:
		{
			if (strlen(inputtext) <= MINIMUM_REQUIRED_PASSWORD)
			{
				ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Registration", "Your password must be longer than "#MINIMUM_REQUIRED_PASSWORD" characters!\nPlease enter your password in the field below:", "Register", "Abort");
				return 1;
			}

			AccLib_RegisterPlayer(playerid);
			return 1;
		}

		default: return 0;
	}
}

public OnPlayerLogin(playerid, bool:success)
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
