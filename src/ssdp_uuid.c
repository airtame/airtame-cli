/*
 * Copyright (C) 2015 AIRTAME
 *
 * This file is part of airtme-cli.
 *
 *   Airtame-cli is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Airtame-cli is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Airtame-cli.  If not, see <http://www.gnu.org/licenses/>.
 *
*/

#include "ssdp_uuid.h"

/*
 * TODO: combine all three functions into a single one that will try to load the
 * uuid from file, and if it doesn't exist then it will generate and save it.
 * TODO: remove len? it is the same as strlen(uuid).
 */
int airtame_load_uuid(char *uuid, int *len) {
    FILE *fp;
    fp = fopen(".saved_uuid", "r");
    if (!fp) return AIRTAME_ERROR;

    int r = fscanf(fp, "%s\n", uuid);
    *len = strlen(uuid);

    fclose(fp);
    return AIRTAME_OK;
}

int airtame_save_uuid(char *uuid) {
    FILE *fp;
    fp = fopen(".saved_uuid", "w");
    if (!fp) return AIRTAME_ERROR;

    fprintf(fp, "%s\n", uuid);
    fclose(fp);

    return AIRTAME_OK;
}

// Need uuid format of:
// xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

void airtame_generate_uuid(char *uuid, int *len) {
    char genneduuid[100];
#ifdef _WIN32
    GUID guid;
    CoCreateGuid(&guid);
    WCHAR *pszGUID;
    HRESULT hr = StringFromCLSID(&guid, &pszGUID);
    wcstombs(genneduuid, pszGUID, sizeof(genneduuid));
    CoTaskMemFree(pszGUID);
    sprintf(uuid, "%.*s", strlen(genneduuid)-2, genneduuid+1); // Replace leading and trailing curly brackets
    *len = strlen(genneduuid)-2;
#else
    uuid_t u;
    uuid_generate(u);
    uuid_unparse(u, genneduuid);
    *len = strlen(genneduuid);
    strcpy(uuid, genneduuid);
#endif
}