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

#include "channel.h"

#ifdef _WIN32

/* Hack to get inet_pton to work on Windows */
#ifndef inet_pton
#warning "inet_pton not defined, trying to use our own version..."
#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

int inet_pton4(const char *src, void *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    int ch;
    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}

int inet_pton6(const char *src, void *dst)
{
    static const char xdigits[] = "0123456789abcdef";
    uint8_t tmp[NS_IN6ADDRSZ];
    int i;

    uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
    uint8_t *endp = tp + NS_IN6ADDRSZ;
    uint8_t *colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
    {
        if (*++src != ':')
            return 0;
    }

    const char *curtok = src;
    int saw_xdigit = 0;
    uint32_t val = 0;
    int ch;
    while ((ch = tolower(*src++)) != '\0')
    {
        const char *pch = strchr(xdigits, ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!saw_xdigit)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return 0;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, (char*) tp) > 0)
        {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return 0;
    }
    if (saw_xdigit)
    {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (uint8_t) (val >> 8) & 0xff;
        *tp++ = (uint8_t) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;

        if (tp == endp)
            return 0;

        for (i = 1; i <= n; i++)
        {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);

    return 1;
}

int inet_pton(int af, const char *src, void *dst)
{
    switch (af)
    {
    case AF_INET:
        return inet_pton4(src, dst);
    case AF_INET6:
        return inet_pton6(src, dst);
    default:
        return -1;
    }
}
#endif

/* Hack to get inet_ntop to work on Windows */
#ifndef inet_ntop
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#endif
#include <string.h>

const char* inet_ntop(int af, const void* src, char* dst, int cnt) {
#ifdef InetNtop
    InetNtop(af, src, dst, cnt);
#else
    static const char fmt[] = "%u.%u.%u.%u";
    char tmp[sizeof "255.255.255.255"];
    unsigned char *charsrc = (unsigned char *)src;

    if (af == AF_INET) {
        if (cnt < strlen("255.255.255.255")) {
            return (NULL);
        }
        sprintf(tmp, fmt, charsrc[0], charsrc[1], charsrc[2], charsrc[3]);
        strcpy(dst, tmp);
        return (dst);
    } else {
        errno = EAFNOSUPPORT;
        return (NULL);
    }
#if 0
    struct sockaddr_in srcaddr;
    memset(&srcaddr, 0, sizeof(struct sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
    srcaddr.sin_family = af;
    if (WSAAddressToString((struct sockaddr*)&srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD)&cnt) != 0) {
        DWORD rv = WSAGetLastError();
        printf("WSAAdressToString(): %d\n", rv);
        return NULL;
    }
    return dst;
#endif
#endif
}
#endif

#endif

int channel_init(Channel_t *channel) {
    int i;
    memset(channel, 0, sizeof(Channel_t));
    channel->msg_id = 0;
    memset(&channel->addr, 0, sizeof(channel->addr));
#ifdef _WIN32
    WSADATA wsaData;
    SOCKADDR_IN sockaddr;
    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        printf("winsock could not be initialized!\n");
        WSACleanup();
        return 0;
    }
#endif

#if defined(MULTICAST_ALL_IFS_WIN)
    channel->pAdapter = NULL;
    channel->prevAdapter = NULL;
#endif
    return 1;
}

uint32_t channel_get_and_incr_msgid(Channel_t *channel) {
#if defined(__GNUC__) || defined(__clang__)
    return __sync_fetch_and_add(&channel->msg_id, 1);
#else
    return channel->msg_id++;
#endif
}


int channel_connect(Channel_t *channel, char *address, int port) {
    char buff[1500];
    char sbuff[10];
    short size = 0;

    channel->client.sin_family = AF_INET;
    channel->client.sin_port = htons(port);
    channel->client.sin_addr.s_addr = inet_addr(address);

    return 1;
}

int channel_disconnect(Channel_t *channel) {
#ifdef _WIN32
    WSACleanup();
#endif
    shutdown(channel->fd, 0);
    return 1;
}

int channel_bind(Channel_t *channel, int port) {
    int rc = 0;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        printf("winsock could not be initiated\n");
        WSACleanup();
        return 0;
    }
#endif

    channel->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (channel->fd == INVALID_SOCKET) {
        printf("error creating socket\n");
        printf("error: %s\n", strerror(errno));
        return 0;
    }

    if (channel->fd > channel->maxfd)
        channel->maxfd = channel->fd;

    int yes=1;
    if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        printf("Failed to enable SO_REUSEADDR\n");
        return 0;
    }

    channel->port = port;
    channel->addr.sin_family = AF_INET;
    channel->addr.sin_port = htons(port);
#ifdef _WIN32
    channel->addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
    channel->addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    rc = bind(channel->fd, (const LPSOCKADDR)&channel->addr, sizeof(channel->addr));
    if (rc == SOCKET_ERROR) {
        printf("error binding to port %d: %d\n", port, rc);
        printf("error: %s\n", strerror(errno));
        return 0;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(channel->fd, (struct sockaddr *)&sin, &len) == -1) {
        printf("error getting socket details!\n");
        printf("error: %s\n", strerror(errno));
    } else {
        channel->port = ntohs(sin.sin_port);
    }

    return 1;
}

#if defined(MULTICAST_ALL_IFS_UNIX)
#ifndef SOCK_ADDR_IN_PTR
#define SOCK_ADDR_IN_PTR(sa)    ((struct sockaddr_in *)(sa))
#endif

#ifndef SOCK_ADDR_IN_ADDR
#define SOCK_ADDR_IN_ADDR(sa)   SOCK_ADDR_IN_PTR(sa)->sin_addr
#endif

int internal_multicast_get_ifs(Channel_t *channel) {
    int result = getifaddrs(&channel->addrs);
    if (result < 0) {
        printf("Error getting list of interfaces!\n");
        return AIRTAME_ERROR;
    }
    return AIRTAME_OK;
}

int channel_multicast_monitor_ifs(Channel_t *channel, int *changed) {
    int match = 1;
    struct ifaddrs *prev_addrs = channel->addrs;
    *changed = 0;

    if (!prev_addrs || internal_multicast_get_ifs(channel) == AIRTAME_ERROR) {
        return AIRTAME_ERROR;
    }

    struct ifaddrs *cursor = channel->addrs;
    struct ifaddrs *prev_cursor = prev_addrs;

    while (cursor != NULL) {
        if (cursor->ifa_addr->sa_family != AF_INET
                || (cursor->ifa_flags & IFF_LOOPBACK)
                || (cursor->ifa_flags & IFF_POINTOPOINT)) {
            cursor = cursor->ifa_next;
            continue;
        }

        match = 0;
        while (prev_cursor != NULL) {
            if (cursor->ifa_addr->sa_family == prev_cursor->ifa_addr->sa_family &&
                    SOCK_ADDR_IN_ADDR(cursor->ifa_addr).s_addr == SOCK_ADDR_IN_ADDR(prev_cursor->ifa_addr).s_addr &&
                    strcmp(cursor->ifa_name, prev_cursor->ifa_name) == 0) {
                match = 1;
                break;
            }
            prev_cursor = prev_cursor->ifa_next;
        }
        if (!match) {
            *changed = 1;
            goto monitor_quit;
        }
        cursor = cursor->ifa_next;
    }

    monitor_quit:
    freeifaddrs(prev_addrs);
    return AIRTAME_OK;
}

int channel_multicast_bind(Channel_t *channel, int port) {
    if (internal_multicast_get_ifs(channel) == AIRTAME_ERROR) {
        return 0;
    }
    channel->used_fds = 0;
    int first_fd = 1;
    int rc;

    /* Loop through all available interfaces which are not loopback, p2p and multicast */
    const struct ifaddrs *cursor = channel->addrs;
    while ( cursor != NULL ) {
        if ( cursor->ifa_addr && cursor->ifa_addr->sa_family == AF_INET
                && !(cursor->ifa_flags & IFF_LOOPBACK)
                && !(cursor->ifa_flags & IFF_POINTOPOINT)
                &&  (cursor->ifa_flags & IFF_MULTICAST) ) {

            /* Create sockets for each if */
            channel->fds[channel->used_fds] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (channel->fds[channel->used_fds] == -1) {
                printf("Failed to create multicast socket\n");
                return 0;
            }

            /* Make the socket transmit through this interface */
            if (setsockopt(channel->fds[channel->used_fds], IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)cursor->ifa_addr)->sin_addr, sizeof(struct in_addr)) != 0) {
                printf("Failed to join multicast on if %s\n", cursor->ifa_name);
                return 0;
            }
            /* We're not interested in receiving our own messages, so we can disable loopback
             (don't rely solely on this - in some cases you can still receive your own messages) */
            unsigned char loop = 0;
            if (setsockopt(channel->fds[channel->used_fds], IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
                printf("Failed to disable multicast loopback\n");
                return 0;
            }

            if (first_fd) {
                /* Bind to the primary socket */
                channel->fd = channel->fds[0];
                /* Enable SO_REUSEADDR to allow multiple instances of receive copies */
                int reuse = 1;
                if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
                    printf("error setting SO_REUSEADDR\n");
                    printf("error: %s\n", strerror(errno));
                    return 0;
                }

#ifndef ANDROID
                reuse = 1;
                if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEPORT, (char *)&reuse, sizeof(reuse)) < 0) {
                    printf("warning setting SO_REUSEPORT\n");
                }
#endif

                channel->addr.sin_family = AF_INET;
                channel->addr.sin_port = htons(port);
                channel->addr.sin_addr.s_addr = htonl(INADDR_ANY);
                rc = bind(channel->fd, (const LPSOCKADDR)&channel->addr, sizeof(channel->addr));
                if (rc == SOCKET_ERROR) {
                    printf("error binding to socket: %d\n", rc);
                    printf("error: %s\n", strerror(errno));
                    return 0;
                }

                channel->maxfd = channel->fd;

                /* Use the first fd as the primary socket for receiving */
                first_fd = 0;
            }

            channel->used_fds++;
        }
        cursor = cursor->ifa_next;
    }
    return 1;
}

int channel_multicast_join(Channel_t *channel, char *address) {
    const struct ifaddrs *cursor = channel->addrs;
    while ( cursor != NULL ) {
        if ( cursor->ifa_addr && cursor->ifa_addr->sa_family == AF_INET
                && !(cursor->ifa_flags & IFF_LOOPBACK)
                && !(cursor->ifa_flags & IFF_POINTOPOINT)
                &&  (cursor->ifa_flags & IFF_MULTICAST) ) {

            /* Prepare multicast group join request */
            struct ip_mreq multicast_req;
            memset(&multicast_req, 0, sizeof(multicast_req));
            multicast_req.imr_multiaddr.s_addr = inet_addr(address);
            multicast_req.imr_interface = ((struct sockaddr_in *)cursor->ifa_addr)->sin_addr;

            /* Workaround for some odd join behaviour: It's perfectly legal to join the same group on more than one interface,
               and up to 20 memberships may be added to the same socket (see ip(4)), but for some reason, OS X spews
               'Address already in use' errors when we actually attempt it.  As a workaround, we can 'drop' the membership
               first, which would normally have no effect, as we have not yet joined on this interface.  However, it enables
               us to perform the subsequent join, without dropping prior memberships. */

            setsockopt(channel->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_req, sizeof(multicast_req));
            /* Join multicast group on this interface */
            if (setsockopt(channel->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0) {
                printf("Failed to join multicast group on if %s!\n", cursor->ifa_name);
            }
        }
        cursor = cursor->ifa_next;
    }
    return 1;
}

int channel_multicast_leave(Channel_t *channel, char *address) {
    const struct ifaddrs *cursor = channel->addrs;
    while ( cursor != NULL ) {
        if ( cursor->ifa_addr && cursor->ifa_addr->sa_family == AF_INET
                && !(cursor->ifa_flags & IFF_LOOPBACK)
                && !(cursor->ifa_flags & IFF_POINTOPOINT)
                &&  (cursor->ifa_flags & IFF_MULTICAST) ) {

            /* Prepare multicast group leave request */
            struct ip_mreq multicast_req;
            memset(&multicast_req, 0, sizeof(multicast_req));
            multicast_req.imr_multiaddr.s_addr = inet_addr(address);
            multicast_req.imr_interface = ((struct sockaddr_in *)cursor->ifa_addr)->sin_addr;

            /* Leave multicast group on this interface */
            if (setsockopt(channel->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0) {
                printf("Failed to leave multicast group on if %s!\n", cursor->ifa_name);
            }
        }
        cursor = cursor->ifa_next;
    }
    /* Free up the linked list once we are done with it */
    if (channel->addrs) {
        freeifaddrs(channel->addrs);
        channel->addrs = NULL;
    }
    return 1;
}

int channel_multicast_send(Channel_t *channel, char *address, int port, char *buff, int size) {
    int i, rc;
    for(i=0;i<channel->used_fds;i++) {
        channel->client.sin_family = AF_INET;
        channel->client.sin_port = htons(port);
        channel->client.sin_addr.s_addr = inet_addr(address);

        rc = sendto(channel->fds[i], buff, size, 0, (struct sockaddr *)&channel->client, (int)sizeof(struct sockaddr_in));
    }
    return rc;
}

int channel_multicast_close(Channel_t *channel) {
    int i;
    for(i=0;i<channel->used_fds;i++) {
        close(channel->fds[i]);
    }
    return 1;
}

#elif defined(MULTICAST_ALL_IFS_WIN)

int internal_multicast_get_ifs(Channel_t *channel) {
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    DWORD dwRetVal = 0;
    int i;

    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    PIP_ADAPTER_INFO pAdapter = NULL;

    if (pAdapterInfo == NULL) {
        printf("Error getting list of interfaces!\n");
        return AIRTAME_ERROR;
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory for GetAdaptersInfo!\n");
            return AIRTAME_ERROR;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        if (channel->prevAdapter)
            free(channel->prevAdapter);
        channel->prevAdapter = channel->pAdapter;
        channel->pAdapter = pAdapterInfo;
    } else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
        channel->pAdapter = NULL;
        return AIRTAME_ERROR;
    }
    return AIRTAME_OK;
}

int channel_multicast_monitor_ifs(Channel_t *channel, int *changed) {
    int match = 1;
    *changed = 0;
    PIP_ADAPTER_INFO addrs = channel->pAdapter;
    if (!addrs) {
        if (internal_multicast_get_ifs(channel) == AIRTAME_ERROR)
            return AIRTAME_ERROR;
    }

    if (internal_multicast_get_ifs(channel) == AIRTAME_ERROR) {
        return AIRTAME_ERROR;
    }

    PIP_ADAPTER_INFO prev_cursor = channel->prevAdapter;
    PIP_ADAPTER_INFO cursor = channel->pAdapter;

    while (cursor) {
        match = 0;
        while (prev_cursor != NULL) {
            if (strcmp(cursor->IpAddressList.IpAddress.String, prev_cursor->IpAddressList.IpAddress.String) == 0) {
                match = 1;
                break;
            }
            prev_cursor = prev_cursor->Next;
        }
        if (!match) {
            *changed = 1;
            goto monitor_quit;
        }
        cursor = cursor->Next;
    }

    monitor_quit:
    //if (prev_cursor){
    //   free(prev_cursor);
    //}

    return AIRTAME_OK;
}

int channel_multicast_bind(Channel_t *channel, int port) {
    if (internal_multicast_get_ifs(channel) == AIRTAME_ERROR) {
        return 0;
    }
    channel->used_fds = 0;
    int first_fd = 1;
    int rc;
    unsigned long addr = 0;

    /* Loop through all available interfaces which are not loopback, p2p and multicast */
    PIP_ADAPTER_INFO cursor = channel->pAdapter;
    while ( cursor != NULL ) {
        if (cursor->Type == MIB_IF_TYPE_OTHER || cursor->Type == MIB_IF_TYPE_ETHERNET || cursor->Type == MIB_IF_TYPE_LOOPBACK ) {
            /* Create sockets for each if */
            channel->fds[channel->used_fds] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (channel->fds[channel->used_fds] == -1) {
                printf("Failed to create multicast socket\n");
                return 0;
            }

            addr = inet_addr(cursor->IpAddressList.IpAddress.String);

            /* Make the socket transmit through this interface */
            if (setsockopt(channel->fds[channel->used_fds], IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, sizeof(addr)) != 0) {
                printf("Failed to join multicast on if %s\n", cursor->AdapterName);
                return 0;
            }

            /* We're not interested in receiving our own messages, so we can disable loopback
             (don't rely solely on this - in some cases you can still receive your own messages) */
            unsigned char loop = 0;
            if (setsockopt(channel->fds[channel->used_fds], IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
                printf("Failed to disable multicast loopback\n");
                return 0;
            }

            if (first_fd) {
                /* Bind to the primary socket */
                channel->fd = channel->fds[0];
                /* Enable SO_REUSEADDR to allow multiple instances of receive copies */
                int reuse = 1;
                if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
                    printf("error setting SO_REUSEADDR\n");
                    return 0;
                }

                reuse = 1;
                if (setsockopt(channel->fd, SOL_SOCKET, SO_BROADCAST, (char *)&reuse, sizeof(reuse)) < 0) {
                    printf("warning setting SO_BROADCAST\n");
                }

                channel->addr.sin_family = AF_INET;
                channel->addr.sin_port = htons(port);
                channel->addr.sin_addr.s_addr = htonl(INADDR_ANY);
                rc = bind(channel->fd, (const LPSOCKADDR)&channel->addr, sizeof(channel->addr));
                if (rc == SOCKET_ERROR) {
                    printf("error binding to socket: %d\n", rc);
                    return 0;
                }

                channel->maxfd = channel->fd;

                /* Use the first fd as the primary socket for receiving */
                first_fd = 0;
            }

            channel->used_fds++;
        }
        cursor = cursor->Next;
    }
    return 1;
}

int channel_multicast_join(Channel_t *channel, char *address) {
    PIP_ADAPTER_INFO cursor = channel->pAdapter;
    while ( cursor != NULL ) {
        if (cursor->Type == MIB_IF_TYPE_OTHER || cursor->Type == MIB_IF_TYPE_ETHERNET || cursor->Type == MIB_IF_TYPE_LOOPBACK ) {
            /* Prepare multicast group join request */
            struct ip_mreq multicast_req;
            memset(&multicast_req, 0, sizeof(multicast_req));
            multicast_req.imr_multiaddr.s_addr = inet_addr(address);
            multicast_req.imr_interface.s_addr = inet_addr(cursor->IpAddressList.IpAddress.String); // On windows you can give it the interface index!

            /* Workaround for some odd join behaviour: It's perfectly legal to join the same group on more than one interface,
               and up to 20 memberships may be added to the same socket (see ip(4)), but for some reason, OS X spews
               'Address already in use' errors when we actually attempt it.  As a workaround, we can 'drop' the membership
               first, which would normally have no effect, as we have not yet joined on this interface.  However, it enables
               us to perform the subsequent join, without dropping prior memberships. */

            setsockopt(channel->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_req, sizeof(multicast_req));
            /* Join multicast group on this interface */
            if (setsockopt(channel->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0) {
                printf("Failed to join multicast group on if %s!\n", cursor->AdapterName);
            }
        }
        cursor = cursor->Next;
    }
    return 1;
}

int channel_multicast_leave(Channel_t *channel, char *address) {
    PIP_ADAPTER_INFO cursor = channel->pAdapter;
    while ( cursor != NULL ) {
        if (cursor->Type == MIB_IF_TYPE_OTHER || cursor->Type == MIB_IF_TYPE_ETHERNET || cursor->Type == MIB_IF_TYPE_LOOPBACK ) {
            /* Prepare multicast group leave request */
            struct ip_mreq multicast_req;
            memset(&multicast_req, 0, sizeof(multicast_req));
            multicast_req.imr_multiaddr.s_addr = inet_addr(address);
            multicast_req.imr_interface.s_addr = inet_addr(cursor->IpAddressList.IpAddress.String);

            /* Leave multicast group on this interface */
            if (setsockopt(channel->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0) {
                printf("Failed to leave multicast group on if %s!\n", cursor->AdapterName);
            }
        }
        cursor = cursor->Next;
    }
    if (channel->pAdapter) {
        free(channel->pAdapter);
        channel->pAdapter = NULL;
    }
    return 1;
}

int channel_multicast_send(Channel_t *channel, char *address, int port, char *buff, int size) {
    int i, rc;
    for(i=0;i<channel->used_fds;i++) {
        channel->client.sin_family = AF_INET;
        channel->client.sin_port = htons(port);
        channel->client.sin_addr.s_addr = inet_addr(address);

        rc = sendto(channel->fds[i], buff, size, 0, (struct sockaddr *)&channel->client, (int)sizeof(struct sockaddr_in));
    }
    return rc;
}

int channel_multicast_close(Channel_t *channel) {
    int i;
    for(i=0;i<channel->used_fds;i++) {
        closesocket(channel->fds[i]);
    }
    return 1;
}
#else

int channel_multicast_monitor_ifs(Channel_t *channel, int *changed) {
    *changed = 0;
    return AIRTAME_OK;
}

int channel_multicast_bind(Channel_t *channel, int port) {
    int rc = 0;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        printf("winsock could not be initiated\n");
        WSACleanup();
        return 0;
    }
#endif

    channel->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (channel->fd == INVALID_SOCKET) {
        printf("error creating socket\n");
        return 0;
    }

    if (channel->fd > channel->maxfd)
        channel->maxfd = channel->fd;

    /* Enable SO_REUSEADDR to allow multiple instances of receive copies */
    int reuse = 1;
    if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        printf("error setting SO_REUSEADDR\n");
        return 0;
    }

#ifdef _WIN32
    /* Enable SO_BROADCAST to allow multiple instances of receive copies */
    reuse = 1;
    if (setsockopt(channel->fd, SOL_SOCKET, SO_BROADCAST, (char *)&reuse, sizeof(reuse)) < 0) {
        printf("error setting SO_BROADCAST\n");
    }
#else
#ifndef ANDROID
    /* Enable SO_REUSEPORT to allow multiple instances of receive copies */
    reuse = 1;
    if (setsockopt(channel->fd, SOL_SOCKET, SO_REUSEPORT, (char *)&reuse, sizeof(reuse)) < 0) {
        printf("error setting SO_REUSEPORT\n");
    }
#endif
#endif

    /* Disable multicast loopback */
    int loopch = 0;
    if (setsockopt(channel->fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
        printf("failed to disable local multicast loop\n");
    }

    channel->port = port;
    channel->addr.sin_family = AF_INET;
    channel->addr.sin_port = htons(port);
#ifdef _WIN32
    channel->addr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
    channel->addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    rc = bind(channel->fd, (const LPSOCKADDR)&channel->addr, sizeof(channel->addr));
    if (rc == SOCKET_ERROR) {
        printf("error binding to socket: %d\n", rc);
        return 0;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(channel->fd, (struct sockaddr *)&sin, &len) == -1) {
        printf("error getting socket details!\n");
    } else {
        channel->port = ntohs(sin.sin_port);
    }

    return 1;
}

int channel_multicast_join(Channel_t *channel, char *address) {
    int rc;
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    rc = setsockopt(channel->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq));

    if (rc < 0)
        return 0;
    return 1;
}

int channel_multicast_leave(Channel_t *channel, char *address) {
    int rc;
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    rc = setsockopt(channel->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&mreq, sizeof(mreq));
    if (rc < 0)
        return 0;
    return 1;
}
int channel_multicast_send(Channel_t *channel, char *address, int port, char *buff, int size) {
    int rc;
    channel->client.sin_family = AF_INET;
    channel->client.sin_port = htons(port);
    channel->client.sin_addr.s_addr = inet_addr(address);

    rc = channel_send(channel, buff, size);
    return rc;
}

#define channel_multicast_close channel_close

#endif

int channel_broadcast_set(Channel_t *c, int broadcast) {
    int broadcastEnable = broadcast;
    setsockopt(c->fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    return 1;
}

int channel_broadcast_join(Channel_t *c) {
    return channel_broadcast_set(c, 1);
}

int channel_broadcast_leave(Channel_t *c) {
    return channel_broadcast_set(c, 0);
}

int channel_accept(Channel_t *channel, Channel_t *targetc) {
    Channel_t backupchan;
    memcpy(&backupchan, targetc, sizeof(Channel_t));

    targetc->fd = accept(channel->fd, NULL, NULL);
    if (targetc->fd == INVALID_SOCKET) {
        printf("Error accepting connection!\n");
        return 0;
    }
    // Close existing connection if exists
    if (backupchan.fd) {
        channel_close(&backupchan);
    }

    return 1;
}

int channel_set_nonblocking(Channel_t *channel, int blk) {
#ifdef _WIN32
    // If iMode!=0, non-blocking mode is enabled.
    unsigned long iMode=blk;
    ioctlsocket(channel->fd,FIONBIO,&iMode);
#else
    int opts;
    if (blk) {
        fcntl(channel->fd, F_SETFL, O_NONBLOCK);
    } else {
        opts = fcntl(channel->fd,F_GETFL);
        opts = opts & (~O_NONBLOCK);
        fcntl(channel->fd, F_SETFL, opts);
    }
#endif
    return 1;
}

int channel_send(Channel_t *channel, char *buff, int size) {
    int rc = 0;
    rc = sendto(channel->fd, buff, size, 0, (struct sockaddr *)&channel->client, (int)sizeof(struct sockaddr_in));
    return rc;
}

int channel_broadcast_send(Channel_t *channel, char *buff, int size) {
    int rc;
    channel->client.sin_family = AF_INET;
    channel->client.sin_port = htons(channel->port);
    channel->client.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    rc = channel_send(channel, buff, size);
    return rc;
}

int channel_recv(Channel_t *channel, char *buff, int size) {
    int rc = 0;
    unsigned int slen = sizeof(struct sockaddr_in);
    rc = recvfrom(channel->fd, buff, size, 0, (struct sockaddr *)&channel->client, &slen);
    return rc;
}

int channel_compare(Channel_t *mainchan, Channel_t *a) {
    if (mainchan->client.sin_family == a->client.sin_family &&
            mainchan->client.sin_port == a->client.sin_port &&
            mainchan->client.sin_addr.s_addr == a->client.sin_addr.s_addr)
        return 1;
    else
        return 0;
}

int channel_clone(Channel_t *mainchan, Channel_t *a) {
    a->fd = mainchan->fd;
    memcpy(&a->client, &mainchan->client, sizeof(mainchan->client));
    return 1;
}

int channel_close(Channel_t *channel) {
    if (channel->fd) {
#ifdef _WIN32
        closesocket(channel->fd);
#else
        close(channel->fd);
#endif
    }
    return 1;
}

int channel_wait(Channel_t *channel) {
    struct timeval waitv;
    fd_set read_flags;
    int err = 0;
    waitv.tv_sec = 0;
    waitv.tv_usec = 5000;

    FD_ZERO(&read_flags);
    FD_SET(channel->fd, &read_flags);
    err = select(channel->maxfd+1, &read_flags, NULL, NULL, &waitv);
    return err;
}

int channel_wait_all(Channel_t channels[], int numchans) {
    struct timeval waitv;
    fd_set read_flags;
    int i = 0, err = 0;
    waitv.tv_sec = 0;
    waitv.tv_usec = 5000;
    int maxfd=0;

    FD_ZERO(&read_flags);

    for(i=0; i<numchans; i++) {
        if (channels[i].fd) {
            FD_SET(channels[i].fd, &read_flags);
            if (channels[i].fd > maxfd)
                maxfd = channels[i].fd;
        }
    }

    err = select(maxfd+1, &read_flags, NULL, NULL, &waitv);
    if (err < 0)
        return err;

    for(i=0; i<numchans; i++)
        if (FD_ISSET(channels[i].fd, &read_flags)) {
            return i;
        }

    return -1;
}
