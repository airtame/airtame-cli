/*
 * utils.c
 *
 *  Created on: May 25, 2015
 *      Author: elisescu
 */

#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ ) || defined( __WINDOWS_WASAPI__ ) || defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "utils.h"

void asleep(int ms) {
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ ) || defined( __WINDOWS_WASAPI__ ) || defined(_WIN32)
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
}