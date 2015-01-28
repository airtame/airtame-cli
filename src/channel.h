#ifndef __H_CHANNEL__
#define __H_CHANNEL__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "error.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#ifndef WINSOCK_VERSION
#define WINSOCK_VERSION MAKEWORD(2,0)
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>

#define SOCKET int
#define SOCKADDR_IN struct sockaddr_in
#define LPSOCKADDR struct sockaddr *
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#define MAX_SOCKETS 16
/* Join multicast group on all interfaces on *nix */
#if !defined(_WIN32)
#define MULTICAST_ALL_IFS_UNIX
#include <net/if.h>
#include <ifaddrs.h>
#else
#define MULTICAST_ALL_IFS_WIN
#endif

#define NUM_CHANNELS 4
#define MAIN 0
#define INPUTS 1
#define DISPLAY 2
#define AUDIO 3


typedef struct {
    int id;
    SOCKET fd;
    SOCKET fds[MAX_SOCKETS];
    int used_fds;
#ifdef MULTICAST_ALL_IFS_UNIX
    struct ifaddrs *addrs;
#elif defined(MULTICAST_ALL_IFS_WIN)
    PIP_ADAPTER_INFO pAdapter;
	PIP_ADAPTER_INFO prevAdapter;
#endif
    SOCKADDR_IN addr;
    SOCKADDR_IN client;
    int port;
    uint32_t msg_id;
    int maxfd;
} Channel_t;

#ifdef _WIN32
#ifndef inet_pton
int inet_pton(int af, const char *src, void *dst);
#endif
#ifndef inet_ntop
const char* inet_ntop(int af, const void *src, char *dst, int cnt);
#endif
#endif

int channel_init(Channel_t *channel);
uint32_t channel_get_and_incr_msgid(Channel_t *channel);
int channel_connect(Channel_t *channel, char *address, int port);
int channel_disconnect(Channel_t *channel);
int channel_bind(Channel_t *channel, int port);
int channel_multicast_monitor_ifs(Channel_t *channel, int *changed);
int channel_multicast_bind(Channel_t *channel, int port);
int channel_multicast_join(Channel_t *channel, char *address);
int channel_multicast_leave(Channel_t *channel, char *address);
int channel_broadcast_join(Channel_t *channel);
int channel_broadcast_leave(Channel_t *channel);
int channel_accept(Channel_t *channel, Channel_t *targetc);
int channel_broadcast_send(Channel_t *channel, char *buff, int size);
int channel_multicast_send(Channel_t *channel, char *address, int port, char *buff, int size);
int channel_multicast_close(Channel_t *channel);
int channel_send(Channel_t *channel, char *buff, int size);
int channel_recv(Channel_t *channel, char *buff, int size);
int channel_compare(Channel_t *mainchan, Channel_t *a);
int channel_clone(Channel_t *mainchan, Channel_t *a);
int channel_close(Channel_t *channel);
int channel_set_nonblocking(Channel_t *channel, int blk);
int channel_wait(Channel_t *channel);
int channel_wait_all(Channel_t channels[], int numchans);

#endif
