
#include <a_samp>
#include <account-lib>

public OnGameModeInit()
{
    printf("Account system by Aiura");
}

public OnAccountChecked(playerid, bool:success)
{
    if (!success)
    {
        ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login System", "Please put password below to login", "Login", "Quit");
    }
    else
    {
        ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Register System", "Please put password below to register your account", "Login", "Quit");
    }

    return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (dialogid == DIALOG_LOGIN)
    {
        if (!response)
        {
            return 0;
        }

        if (IsNull(inputtext))
        {
            SendClientMessage(playerid, 0xFF0000, "Error: {FFFFFF}Invalid password specified");
        }
        else if (strlen(inputtext) < 6)
        {
            SendClientMessage(playerid, 0xFF0000, "Error: {FFFFFF}Password must contain more than 6 characters");
        }
        else if (!Account_Login(playerid, inputtext))
        {
            SendClientMessage(playerid, 0xFF0000, "Error: {FFFFFF}The accounts is currently logged-in, if you think this is a mistake please contact administrator as soon as possible.");
        }
        else
        {
            return 1; // prevent showing the dialog.  
        }

        ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login System", "Please put password below to login", "Login", "Quit");
        return 1;
    }
    else if (dialogid == DIALOG_REGISTER)
    {
        if (!response)
        {
            return 0;
        }

        if (IsNull(inputtext))
        {
            SendClientMessage(playerid, 0xFF0000, "Error: {FFFFFF}Invalid password specified");
        }
        else if (strlen(inputtext) < 6)
        {
            SendClientMessage(playerid, 0xFF0000, "Error: {FFFFFF}Password must contain more than 6 characters");
        }
        else
        {
            Account_Register(playerid, inputtext);
            return 1; // prevent showing the dialog.  
        }

        ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_PASSWORD, "Register System", "Please put password below to register your account", "Login", "Quit");
        return 1;
    }
}

public OnAccountVerified(playerid, bool:success)
{
        if (success)
        {
            // Spawn here
        }
        else
        {
            SendClientMessage(playerid, 0xFF0000, "Something happens with internal code, please relogin and if it's not working contact our staff.");
            Kick(playerid);
            return 1;
        }
}

public OnPlayerRegister(playerid)
{
    SendClientMessage(playerid, -1, "Successfully registered to our server");
    return 1;
}

