#ifndef __H_SSDP_UUID__
#define __H_SSDP_UUID__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <uuid/uuid.h>
#endif


int airtame_load_uuid(char *uuid, int *len);
int airtame_save_uuid(char *uuid);
void airtame_generate_uuid(char *uuid, int *len);

#endif
