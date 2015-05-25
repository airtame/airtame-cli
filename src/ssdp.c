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

#include "ssdp.h"
#define MULTICAST_IF_MONITOR_TIMEOUT 2

int internal_to_lower_string(char *inp, int len) {
    int i;
    for(i=0; i<len; i++) {
        inp[i] = tolower(inp[i]);
    }
    return 0;
}

void internal_print_ssdp_options(SSDP_Options_t *o) {
    printf("Request type: %d\n", o->req);
    printf("From: %s:%d\n", o->ip, o->port);
    printf("Airtame name: %s (Security: %s)\n", o->name, o->security);
    printf("Cache control: %s\n", o->cache);
    printf("Location: %s\n", o->location);
    printf("NT: %s\n", o->nt);
    printf("NTS: %s\n", o->nts);
    printf("Server: %s\n", o->server);
    printf("USN: %s\n", o->usn);
    printf("ST: %s\n", o->st);
    printf("MAN: %s\n", o->man);
}

int internal_ssdp_net_init(SSDP_t *ssdp) {
    channel_init(&ssdp->sockets[SSDP_SOCKET_MULTICAST]);
    channel_init(&ssdp->sockets[SSDP_SOCKET_UNICAST]);
    channel_multicast_bind(&ssdp->sockets[SSDP_SOCKET_MULTICAST], SSDP_BIND_PORT);
    channel_bind(&ssdp->sockets[SSDP_SOCKET_UNICAST], SSDP_BIND_PORT+1);
    channel_multicast_join(&ssdp->sockets[SSDP_SOCKET_MULTICAST], SSDP_MCAST_ADDR);
    return AIRTAME_OK;
}

int internal_ssdp_net_cleanup(SSDP_t *ssdp) {
    channel_multicast_leave(&ssdp->sockets[SSDP_SOCKET_MULTICAST], SSDP_MCAST_ADDR);
    channel_multicast_close(&ssdp->sockets[SSDP_SOCKET_MULTICAST]);
    channel_disconnect(&ssdp->sockets[SSDP_SOCKET_UNICAST]);
    channel_close(&ssdp->sockets[SSDP_SOCKET_UNICAST]);
    return AIRTAME_OK;
}

int ssdp_init(SSDP_t *ssdp, ssdp_callback_f n, ssdp_callback_f r) {
    internal_ssdp_net_init(ssdp);
    ssdp->broadcast_enabled = 0;
    ssdp->last_multicast_msg = time(0);
    sprintf(ssdp->service, "");
    sprintf(ssdp->location, "");
    sprintf(ssdp->usn, "");
    ssdp->notify_callback = n;
    ssdp->resp_callback = r;
    ssdp->network_notify_callback = 0;
    return AIRTAME_OK;
}

int ssdp_register_net_change_callback(SSDP_t *ssdp, ssdp_callback_f n) {
    ssdp->network_notify_callback = n;
    return AIRTAME_OK;
}

int ssdp_parse(char *buff, int size, SSDP_Options_t *o) {
    char *ptr = buff;
    char *pptr = buff;
    char *tmp;
    char currline[OPTIONS_SIZE+1];
    char currline_bckp[OPTIONS_SIZE+1];
    int r, vallen;

    memset((void *)o, 0, sizeof(SSDP_Options_t));
    do {
        ptr = strchr(ptr+1, '\n');
        if (ptr) {
            /* Copy out a single line */
            r = (ptr-pptr < OPTIONS_SIZE ? ptr-pptr : OPTIONS_SIZE-1);
            strncpy(currline, pptr, r);
            currline[r] = 0;

            /* Notification from devices */
            if (strncmp(currline, "NOTIFY", 6) == 0) {
                o->req = SSDP_REQ_NOTIFY;
                /* M-SEARCH request? should we be receiving this even? */
            } else if (strncmp(currline, "M-SEARCH", 8) == 0) {
                o->req = SSDP_REQ_MSEARCH;
                /* Response to the M-SEARCH request */
            } else if (strncmp(currline, "HTTP/1.1", 8) == 0) {
                o->req = SSDP_REQ_RESP;
                /* It wasn't the first line, must the headers */
            }  else {
                /* Get rid of \r */
                tmp = strchr(currline, '\r');
                if (tmp)
                    *tmp = 0;
                /* Back up the string before lowering-casing it */

                strcpy(currline_bckp, currline);
                internal_to_lower_string(currline, r);

                tmp = strchr(currline, ':');
                if (tmp) {
                    if (*(tmp+1) == ' ') tmp++;
                    /* Calculate the string length of the value in the key: value pair */
                    vallen = r - (tmp-currline) - 1;
                    /* Let's find the important headers for us! */
                    if (strncmp(currline, "cache-control",13) == 0) {
                        sprintf(o->cache, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "location", 8) == 0) {
                        sprintf(o->location, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "nts", 3) == 0) {
                        sprintf(o->nts, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "nt", 2) == 0) {
                        sprintf(o->nt, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "server", 6) == 0) {
                        sprintf(o->server, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "usn", 3) == 0) {
                        sprintf(o->usn, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "st", 2) == 0) {
                        sprintf(o->st, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "man", 3) == 0) {
                        sprintf(o->man, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "airtame-name", 12) == 0) {
                        /* The only case sensitive parameter here is the computer name */
                        tmp = strchr(currline_bckp, ':');
                        if (*(tmp+1) == ' ') tmp++;
                        sprintf(o->name, "%.*s", vallen, tmp+1);
                    } else if (strncmp(currline, "airtame-security", 16) == 0) {
                        sprintf(o->security, "%.*s", vallen, tmp+1);
                    }
                }
            }
        }
        pptr = ptr+1;
    } while (ptr);
    return AIRTAME_OK;
}

int ssdp_advertise(SSDP_t *ssdp, char nts, char *service, char *location, char *usn, char *name, char *security) {
    char buffer[1000];
    int cache_age = 100;
    char *nt_types[] = { "ssdp:alive", "ssdp:byebye", "ssdp:update" };

    sprintf(buffer, "NOTIFY * HTTP/1.1\r\n"
            "HOST: %s:%d\r\n"
            "CACHE-CONTROL: max-age=%d\r\n"
            "LOCATION: %s\r\n"
            "NT: %s\r\n"
            "NTS: %s\r\n"
            "SERVER: Dummy/1.0 UPnP/1.1 AirTame-SSDP/1.0\r\n"
            "USN: %s\r\n"
            "AIRTAME-NAME: %s\r\n"
            "AIRTAME-SECURITY: %s\r\n\r\n"
            ,SSDP_MCAST_ADDR, SSDP_MCAST_PORT, cache_age, location, service, nt_types[(int)nts], usn, name, security);

    channel_multicast_send(&ssdp->sockets[SSDP_SOCKET_MULTICAST], SSDP_MCAST_ADDR, SSDP_MCAST_PORT, buffer, strlen(buffer));

    strcpy(ssdp->service, service);
    strcpy(ssdp->location, location);
    strcpy(ssdp->usn, usn);
    strcpy(ssdp->name, name);
    strcpy(ssdp->security, security);
    return AIRTAME_OK;
}

int ssdp_search(SSDP_t *ssdp, char *st) {
    char buffer[1000];
    int cache_age = 5;

    sprintf(buffer, "M-SEARCH * HTTP/1.1\r\n"
            "HOST: %s:%d\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "MX: %d\r\n"
            "ST: %s\r\n"
            "USER-AGENT: Dummy/1.0 UPnP/1.1 AirTame-SSDP/1.0\r\n\r\n"
            ,SSDP_MCAST_ADDR, SSDP_MCAST_PORT, cache_age, st);

    channel_multicast_send(&ssdp->sockets[SSDP_SOCKET_MULTICAST], SSDP_MCAST_ADDR, SSDP_MCAST_PORT, buffer, strlen(buffer));
    return AIRTAME_OK;
}

int ssdp_reply(SSDP_t *ssdp, char *st, char *location, char *usn, char *name, char *security) {
    char buffer[1000];
    int cache_age = 100;
    sprintf(buffer, "HTTP/1.1 200 OK\r\n"
            "CACHE-CONTROL: max-age=%d\r\n"
            "ST: %s\r\n"
            "LOCATION: %s\r\n"
            "USN: %s\r\n"
            "AIRTAME-NAME: %s\r\n"
            "AIRTAME-SECURITY: %s\r\n\r\n"
            ,cache_age, st, location, usn, name, security);

    channel_clone(&ssdp->sockets[SSDP_SOCKET_MULTICAST], &ssdp->sockets[SSDP_SOCKET_UNICAST]);
    ssdp->sockets[SSDP_SOCKET_UNICAST].client.sin_port = htons(SSDP_MCAST_PORT+1);
    channel_send(&ssdp->sockets[SSDP_SOCKET_UNICAST], buffer, strlen(buffer));
    return AIRTAME_OK;
}

int ssdp_handle(SSDP_t *ssdp) {
    int rc, curr_sock = 0, ifs_changed = 0;
    char buff[1501];
    SSDP_Options_t o;

    curr_sock = channel_wait_all(ssdp->sockets, 2);
    if (curr_sock < 0) {
        if (time(0) - ssdp->last_monitor_handle > MULTICAST_IF_MONITOR_TIMEOUT) {
            rc = channel_multicast_monitor_ifs(&ssdp->sockets[SSDP_SOCKET_MULTICAST], &ifs_changed);
            if (rc == AIRTAME_ERROR) {
                printf("An error occured in monitoring ifs!\n");
            }
            if (ifs_changed) {
                printf("Change in network configuration detected! Reinitalizing discovery...\n");
                internal_ssdp_net_cleanup(ssdp);
                internal_ssdp_net_init(ssdp);
                if (ssdp->network_notify_callback) ssdp->network_notify_callback((void *)ssdp, NULL);
            }
            ssdp->last_monitor_handle = time(NULL);
        }
        return AIRTAME_TIMEOUT;
    }

    memset(buff, 0, 1500);
    rc = channel_recv(&ssdp->sockets[curr_sock], buff, sizeof(buff));
    if (rc < 1) {
        return AIRTAME_TIMEOUT;
    }

    memset(&o, 0, sizeof(o));
    ssdp_parse(buff, rc, &o);
    o.ip = inet_ntoa(ssdp->sockets[curr_sock].client.sin_addr);
    o.port = ssdp->sockets[curr_sock].client.sin_port;

    /* Notify message */
    if (o.req == 1) {
        if (ssdp->notify_callback) ssdp->notify_callback((void *)ssdp, (void *)&o);
        /* Search request */
    } else if (o.req == 2) {
        /* If someone requests our service, or queries all then reply, otherwise ignore! */
        if (strcmp(o.st, ssdp->service) == 0 ||
                strncmp(o.st, "ssdp:all", 7) == 0) {
            ssdp_reply(ssdp, ssdp->service, ssdp->location, ssdp->usn, ssdp->name, ssdp->security);
        }
        /* Search response */
    } else if (o.req == 3) {
        if (ssdp->resp_callback) ssdp->resp_callback((void *)ssdp, (void *)&o);
    }

    ssdp->last_multicast_msg = time(0);
    return AIRTAME_OK;
}

int ssdp_cleanup(SSDP_t *ssdp) {
    return internal_ssdp_net_cleanup(ssdp);
}
