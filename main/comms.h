/* DESCRIPTION ***************************************************

 File:                comms.h

 Author:              Robert Carey

 Creation Date:       9th November 2020

 Description:

 END DESCRIPTION ***************************************************************/

#pragma once
typedef struct
{
    char *wifiSsid;
    char *wifiPass;
    char *userEmail;
    char *userPass;
    char **userId;
    char **deviceId;
} TCommsSetup;

void comms_init(const TCommsSetup *const commsSetup);
