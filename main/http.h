/* DESCRIPTION ***************************************************

 File:                http.c

 Author:              Robert Carey

 Creation Date:       9th November 2020

 Description:         

 END DESCRIPTION ***************************************************************/

#ifndef MAIN_HTTP_H_
#define MAIN_HTTP_H_

void http_init(void);
esp_err_t http_get_jwt(const char *usr, const char *pass, char **userId, char *jwt);
esp_err_t http_reg_device(const char *jwt, char **deviceId);
esp_err_t http_get_device(const char *deviceId, const char *jwt);

#endif