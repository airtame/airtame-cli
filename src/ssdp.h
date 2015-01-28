#ifndef __H_SSDP__
#define __H_SSDP__
/* Simple Service Discovery Protocol implementation */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define u_int32 UINT32
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#endif

#include "error.h"
#include "channel.h"

#define SSDP_MCAST_ADDR "239.255.255.250"
#define SSDP_MCAST_PORT 1900
#define SSDP_BIND_PORT 1900
#define AIRTAME_SSDP_ANNOUNCE_TIME 100

#define MULTICAST_TIMEOUT 10
#define OPTIONS_SIZE 500

#define SSDP_REQ_NOTIFY 1
#define SSDP_REQ_MSEARCH 2
#define SSDP_REQ_RESP 3

typedef void (*ssdp_callback_f)(void *ssdp, void *data);

typedef struct {
    char req;
    char *ip;
    uint16_t port;
    char cache[OPTIONS_SIZE];
    char location[OPTIONS_SIZE];
    char server[OPTIONS_SIZE];
    char usn[OPTIONS_SIZE];
    char nts[OPTIONS_SIZE];
    char nt[OPTIONS_SIZE];
    char st[OPTIONS_SIZE];
    char man[OPTIONS_SIZE];
    char name[OPTIONS_SIZE];
    char security[OPTIONS_SIZE];
} SSDP_Options_t;

#define SSDP_SOCKET_MULTICAST 0
#define SSDP_SOCKET_UNICAST 1

typedef struct {
    ssdp_callback_f notify_callback;
    ssdp_callback_f resp_callback;
    ssdp_callback_f network_notify_callback;
    char service[OPTIONS_SIZE];
    char location[OPTIONS_SIZE];
    char usn[OPTIONS_SIZE];
    char name[OPTIONS_SIZE];
    char security[OPTIONS_SIZE];
    Channel_t sockets[2];
    time_t last_multicast_msg;
    time_t last_monitor_handle;
    char broadcast_enabled;
    void *user;
} SSDP_t;

int ssdp_init(SSDP_t *ssdp, ssdp_callback_f n, ssdp_callback_f r);
int ssdp_register_net_change_callback(SSDP_t *ssdp, ssdp_callback_f n);
int ssdp_advertise(SSDP_t *ssdp,  char nt, char *uuid, char *location, char *usn, char *name, char *security);
int ssdp_search(SSDP_t *ssdp, char *st);
int ssdp_reply(SSDP_t *ssdp, char *st, char *location, char *usn, char *name, char *security);
int ssdp_handle(SSDP_t *ssdp);
int ssdp_cleanup(SSDP_t *ssdp);

#endif
