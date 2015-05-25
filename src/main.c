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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <jsonrpc-c.h>

#include "ssdp.h"
#include "ssdp_uuid.h"
#include "threading.h"
#include "airtame-server-rpc.h"

#define AIRTAME_SSDP_SERVICE "airtame:streamer"

// Platform-dependent sleep routines.
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ ) || defined( __WINDOWS_WASAPI__ ) || defined(_WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

#ifndef __GNUC__
#include "wingetopt.h"
#else
#include <getopt.h>
#endif

#define AIRTAME_VERSION ("0.1")
#define MAX_NUM_DEVS 1000

int dontstop = 1;
int currcmd_dontstop = 1;
typedef struct {
	char name[50];
	char ip[20];
	int port;
} DevDetails;

typedef struct {
	int num_devices;
	DevDetails devices[MAX_NUM_DEVS];
} Devices;

enum Function {NONE, CLOSE_STREAMER, INIT_STREAMER, CONNECT_IP, DISCONNECT_IP, CONNECT,
    DISCONNECT, SCAN, LISTEN, AVMODE, AVBUFFER, FRAMERATE, QUALITY, SHELL};

enum Function current_function = NONE;
char *additional_args = NULL;

Devices devs;
Thread_t ssdp_thread;

// server RPC defitions
#define CMDS_PORT (8004)
#define NOTFS_PORT (8005)
struct jrpc_server *notifications_listener = NULL;
struct jrpc_client cmds_client;
Thread_t notifications_thread;

// functions definitions
void scan_devices();
DevDetails *search_for_device(char *name, int timeout);
void rpc_set_framerate(char *fps);
void rpc_set_quality(char *q);
void rpc_set_mode(char *mode);
void rpc_set_buffer(char *buff_period);
void rpc_set_audio_enabled(char *enabled);
void rpc_set_fplayback_enabled(char *enabled);
void rpc_disconnect(char *ip, char *port);
void rpc_connect(char *ip, char *port);
void rpc_init_streamer();
void rpc_close_streamer();
void print_rpc_result(char *method, cJSON *result);
void rpc_get_state();


void int_handler(int s) {
    currcmd_dontstop = 0;
    printf("Caught Ctrl-C\n");
    SLEEP(1000);
    if (current_function != SHELL) {
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
    }
#ifndef _WIN32
    signal(SIGQUIT, SIG_DFL);
#endif
}

void add_dev(char *name, char *ip, int port) {
	int i;
	int found = 0;
	for(i=0;i<devs.num_devices;i++) {
		if (strcmp(devs.devices[i].ip, ip) == 0)
			found = 1;
	}
	if (found == 0) {
		strcpy(devs.devices[devs.num_devices].name, name);
		strcpy(devs.devices[devs.num_devices].ip, ip);
		devs.devices[devs.num_devices].port = port;
		devs.num_devices++;
	}
}

void ssdp_notify_callback(SSDP_t *ssdp, void *user) {
    SSDP_Options_t *o = (SSDP_Options_t *)user;
    if (strncmp(o->nt, "airtame:recv",12) == 0 || strncmp(o->st, "airtame:recv", 12) == 0) {
		add_dev(o->name, o->ip, o->port);
    }
}

void ssdp_resp_callback(SSDP_t *ssdp, void *user) {
    SSDP_Options_t *o = (SSDP_Options_t *)user;

    ssdp_notify_callback(ssdp, user);
}

cJSON * streaming_state(jrpc_context * ctx, cJSON * params, cJSON *id) {
    char *str_params = cJSON_Print(params);
    printf("\nstreaming state: %s\n", str_params); fflush(stdout);
    free(str_params);
	return NULL;
}

cJSON * connection_state(jrpc_context * ctx, cJSON * params, cJSON *id) {
    char *str_params = cJSON_Print(params);
    printf("\nconnection state: %s\n", str_params); fflush(stdout);
    free(str_params);
	return NULL;
}

void notifications_thread_main(void *user) {
	jrpc_server_run(notifications_listener);
	jrpc_server_destroy(notifications_listener);
}

void ssdp_main_thread(void *user) {
    SSDP_t ssdp;
    time_t ssdp_last_announce = 0;
    char usn[200];

    /* Try to load the saved UUID */
    char uuid[100];
    int len;
    int r = airtame_load_uuid(uuid, &len);
    if (r == AIRTAME_ERROR) {
        airtame_generate_uuid(uuid, &len);
       printf("Could not find saved UUID. Generating a new one: %s!\n", uuid);
        r = airtame_save_uuid(uuid);
        if (r == AIRTAME_ERROR) {
            printf("Failed to save the UUID to a file!\n");
		}
	}
    sprintf(usn, "uuid:%s", uuid);

    /* Initialize the SSDP service */
    ssdp_init(&ssdp, (ssdp_callback_f) ssdp_notify_callback, (ssdp_callback_f) ssdp_resp_callback);
    ssdp.user = 0;

    ssdp_search(&ssdp, "airtame:recv");

    /* Handle everything to do with SSDP */
    while (dontstop) {
        ssdp_handle(&ssdp);
        if ((time(0) - ssdp_last_announce) > AIRTAME_SSDP_ANNOUNCE_TIME) {
            char name[256];
            gethostname(name, 255);
            ssdp_advertise(&ssdp, 0, AIRTAME_SSDP_SERVICE, "http://my.airtame.com/", usn,name ,"none");
            ssdp_last_announce = time(0);
        }

    }
    ssdp_cleanup(&ssdp);

}

void print_help(char *name) {
    printf("\nusage: %s [-C ip] [-D ip] [-c dev_name] [-d dev_name]\n"
           "            [-slSv] [-m mode]\n"
           "        -i/x            : init/clean the streamer\n"
           "        -C/D <ip>       : Connect/disconnect by IP\n"
           "        -c/d <dev_name> : Connect/disconnect by device name (got from devices scan)\n"
           "        -s              : Scan for devices on the network\n"
           "        -m <mode>       : Set streaming mode (manual, video, work, present)\n"
           "        -f <fps>        : Frames per second throttling [1-60 fps]\n"
           "        -q <quality>    : Quality setting [0-5, 0 = minimum 5 = maximum]\n"
           "        -a <1/0>        : Enable/disable audio\n"
           "        -l <1/0>        : Enable/disable fluent playback\n"
           "        -v              : AIRTAME version\n"
           "        -S              : Shell mode\n\n"
           ,name);
}

void print_cmdline_help() {
    printf("\n Commands:\n"
           " C/connect_ip <ip>        : Connect to an AIRTAME by IP\n"
           " D/disconnect_ip <ip>     : Disconnect from an AIRTAME by IP\n"
           " c/connect <name>         : Connect to an AIRTAME by the device name\n"
           " d/disconnect <name>      : Disconnect from an AIRTAME by name\n"
           " s/scan                   : Scan for devices on the network\n"
           " get_state                : Get the server state\n"
           " f/fps <num>              : Set the framerate (0-60 fps)\n"
           " q/quality <num>          : Set the quality (0 = minimum, 5 = maximum)\n"
           " m/mode <value>           : Mode (manual, video, work, present)\n"
           " a/audio <1/0>            : Enable/Disable audio\n"
           " l/fluent <1/0>           : Enable/Disable fluent playback\n"
           " v/version                : AIRTAME version\n"
           " e/exit                   : Quit\n"
           "\n"
    );
}

void print_version() {
    printf("AIRTAME Version: %s\n", AIRTAME_VERSION);
}

// command shell stuff
#define MAX_CMDLINE_LENGTH (256)
char cmd_line[MAX_CMDLINE_LENGTH];
Thread_t cmdline_thread;

int parse_cmdline(char *line) {
    int l = strlen(line);
    DevDetails *curr_dev;

    //remove the new line
    if (line[l-1] == '\n') line[l-1] = '\0';

    if (line[0] == 'h' || strncmp(line, "help", 4) == 0) {
        print_cmdline_help();
        return 0;
    }

    if (line[0] == 'v' || strncmp(line, "version", 7) == 0) {
        print_version();
        return 0;
    }

    if (line[0] == 'l' || strncmp(line, "fluent", 3) == 0) {
        char *fluent_enabled = strchr(line, ' ');
        if (fluent_enabled == NULL) return 0;
        while (fluent_enabled && fluent_enabled[0] == ' ') fluent_enabled++;
        rpc_set_fplayback_enabled(fluent_enabled);
        return 0;
    }

    if (line[0] == 'a' || strncmp(line, "audio", 3) == 0) {
        char *audio_enabled = strchr(line, ' ');
        if (audio_enabled == NULL) return 0;
        while (audio_enabled && audio_enabled[0] == ' ') audio_enabled++;
        rpc_set_audio_enabled(audio_enabled);
        return 0;
    }

    if (line[0] == 'f' || strncmp(line, "fps", 3) == 0) {
        char *fps = strchr(line, ' ');
        if (fps == NULL) return 0;
        while (fps && fps[0] == ' ') fps++;
        rpc_set_framerate(fps);
        return 0;
    }

    if (line[0] == 'q' || strncmp(line, "quality", 7) == 0) {
        char *q = strchr(line, ' ');
        if (q == NULL) return 0;
        while (q && q[0] == ' ') q++;
        rpc_set_quality(q);
        return 0;
    }

    if (line[0] == 'm' || strncmp(line, "mode", 4) == 0) {
        char *m = strchr(line, ' ');
        if (m == NULL) return 0;
        while (m && m[0] == ' ') m++;
        rpc_set_mode(m);
        return 0;
    }

    if (line[0] == 's' || strncmp(line, "scan", 4) == 0) {
        currcmd_dontstop = 1;
        scan_devices();
        currcmd_dontstop = 1;
        return 0;
    }

    if (line[0] == 'D' || strncmp(line, "disconnect_ip", 13) == 0) {
        char *ip = strchr(line, ' ');
        if (ip == NULL) return 0;
        while (ip && ip[0] == ' ') ip++;
        rpc_disconnect(ip, "8002");
        return 0;
    }

    if (line[0] == 'd' || strncmp(line, "disconnect", 10) == 0) {
        char *name = strchr(line, ' ');
        if (name == NULL) return 0;
        while (name && name[0] == ' ') name++;

        curr_dev = search_for_device(name, 5);
        if (!curr_dev) {
            printf("Didn't find the device! Boo :(\n");
        } else {
            printf("Disconnecting from %s.\n", curr_dev->ip);
            rpc_disconnect(curr_dev->ip, "8002");
        }
        return 0;
    }

    if (line[0] == 'C' || strncmp(line, "connect_ip", 10) == 0) {
        char *ip = strchr(line, ' ');
        if (ip == NULL) return 0;
        while (ip && ip[0] == ' ') ip++;
        rpc_connect(ip, "8002");
        return 0;
    }

    if (strncmp(line, "get_state", 9) == 0) {
        rpc_get_state();
        return 0;
    }

    if (line[0] == 'c' || strncmp(line, "connect", 7) == 0) {
        char *name = strchr(line, ' ');
        if (name == NULL) return 0;
        while (name && name[0] == ' ') name++;

        curr_dev = search_for_device(name, 5);
        if (!curr_dev) {
            printf("Didn't find the device! Boo :(\n");
        } else {
            printf("Connecting to %s.\n", curr_dev->ip);
            rpc_connect(curr_dev->ip, "8002");
        }
        return 0;
    }

    if (strstr(line, "quit") || strstr(line, "exit") || strstr(line, "e"))
        return 1;

    return 0;
}

void read_cmdline(void *data) {

    notifications_listener = (struct jrpc_server *) malloc (sizeof(struct jrpc_server));
	jrpc_server_init(notifications_listener, NOTFS_PORT);
	jrpc_register_procedure(notifications_listener, streaming_state, "streamingState", NULL );
	jrpc_register_procedure(notifications_listener, connection_state, "connectionState", NULL );

    rpc_init_streamer();

    threading_create_thread(&notifications_thread, notifications_thread_main, NULL);
    char *not_port = (char *) malloc(6);
    sprintf(not_port, "%d", NOTFS_PORT);
    cJSON *result = jrpc_client_call(&cmds_client, "registerListener", 2, "127.0.0.1", not_port);
    print_rpc_result("registerListener", result);
    while (printf("\nairtame $ ") && fgets(cmd_line, sizeof(cmd_line), stdin) && dontstop) {
        if (parse_cmdline(cmd_line)) { dontstop = 0; break; }
    }
    //rpc_close_streamer();
    result = jrpc_client_call(&cmds_client, "unregisterListener", 2, "127.0.0.1", not_port);
    print_rpc_result("unregisterListener", result);
    rpc_close_streamer();
    jrpc_server_stop(notifications_listener);
    free(not_port);
    printf("\nSee you again! ;-)\n");
}

int process_args(int argc, char **argv) {
	int c;

	if (argc < 2) {
	    print_help(argv[0]);
        return 1;
	}

	opterr = 0;
    while ((c = getopt(argc, argv, "C:D:c:d:m:b:f:q:lsShvix")) != -1) {
		switch(c) {
            case 'i':
                current_function = INIT_STREAMER;
                additional_args = optarg;
                break;
            case 'x':
                current_function = CLOSE_STREAMER;
                additional_args = optarg;
                break;
            case 'C':
                current_function = CONNECT_IP;
                additional_args = optarg;
                break;
            case 'D':
                current_function = DISCONNECT_IP;
                additional_args = optarg;
                break;
            case 'c':
				current_function = CONNECT;
				additional_args = optarg;
				break;
			case 'd':
				current_function = DISCONNECT;
				additional_args = optarg;
				break;
			case 's':
				current_function = SCAN;
				break;
			case 'S':
				current_function = SHELL;
				break;
			case 'l':
				current_function = LISTEN;
				break;
			case 'm':
				current_function = AVMODE;
				additional_args = optarg;
				break;
			case 'b':
				current_function = AVBUFFER;
				additional_args = optarg;
				break;
            case 'f':
                current_function = FRAMERATE;
                additional_args = optarg;
                break;
            case 'q':
                current_function = QUALITY;
                additional_args = optarg;
                break;
            case 'h':
                print_help(argv[0]);
                exit(0);
            case 'v':
                print_version();
                exit(0);
			default:
				abort();
		}
	}
    return 0;
}

DevDetails *search_for_device(char *name, int timeout) {
	int i, found = 0;
    uint32_t start_time = time(NULL);

	while (!found) {
		for(i=0;i<devs.num_devices;i++) {
			if (strcmp(name, devs.devices[i].name) == 0) {
				found = 1;
				break;
			}
		}

		if (found) {
			printf("\nFound device %s (%s:%d)\n", devs.devices[i].name, devs.devices[i].ip, devs.devices[i].port);
			return &devs.devices[i];
		} else {
            if (timeout && (time(NULL) - start_time) > timeout) {
                return NULL;
            }
        }
		SLEEP(50);
	}
	return NULL;
}

void scan_devices() {
    int i;
	time_t start_time;
    printf("\nScanning for devices..\n");
    start_time = time(NULL);
    while (currcmd_dontstop) {
        if ((time(NULL) - start_time) > 1) {
            start_time = time(NULL);
            printf("Discovered devices:\n");
            for(i=0;i<devs.num_devices;i++) {
                printf("    %d. %s (%s)\n", i, devs.devices[i].name, devs.devices[i].ip);
            }
            printf("\n");
        } else {
            SLEEP(50);
        }
    }
    printf("\nStopped scanning.\n");
}

int main(int argc, char **argv) {
	char intbuff[2000];
	int rc, len;
	DevDetails *curr_dev;
	char *val;
	int args = 0;

	devs.num_devices = 0;

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);
#ifndef _WIN32
    signal(SIGQUIT, int_handler);
#endif

	if (process_args(argc, argv)) {
	    return 1;
	}

	jrpc_client_init(&cmds_client);

	if (jrpc_client_connect(&cmds_client, "127.0.0.1", CMDS_PORT) == -1) {
	    printf("Cannnot connect to the airtame-server. Exiting...");
	    return 1;
	}

    threading_create_thread(&ssdp_thread, ssdp_main_thread, NULL);

	switch(current_function) {
        case CLOSE_STREAMER:
            rpc_close_streamer();
            break;
        case INIT_STREAMER:
            rpc_init_streamer();
            break;
        case CONNECT_IP:
            rpc_connect(additional_args, "8002");
            break;
        case DISCONNECT_IP:
            rpc_disconnect(additional_args, "8002");
            break;
        case CONNECT:
			printf("Searching for device: %s\n", additional_args);
			curr_dev = search_for_device(additional_args, 5);
            if (!curr_dev) {
				printf("Didn't find the device! Boo :(\n");
			} else {
                printf("Connecting to %s.\n", curr_dev->ip);
                rpc_connect(curr_dev->ip, "8002");
			}
			break;
		case DISCONNECT:
			printf("Searching for device: %s\n", additional_args);
			curr_dev = search_for_device(additional_args, 5);
			if (!curr_dev) {
				printf("Didn't find the device! Boo :(\n");
			} else {
                printf("Disconnecting from %s.\n", curr_dev->ip);
				rpc_disconnect(curr_dev->ip, "8002");
			}
			break;
		case SCAN:
		    scan_devices();
			break;
		case SHELL: {
		    threading_create_thread(&cmdline_thread, read_cmdline, NULL);
		    break;
		}
		case AVMODE: {
            printf("Mode set to %s\n", additional_args);
            rpc_set_mode(additional_args);
			break;
		}
		case AVBUFFER:
            printf("Buffer set to %s\n", additional_args);
            rpc_set_buffer(additional_args);
			break;
        case FRAMERATE:
            rpc_set_framerate(additional_args);
            printf("Framerate set to %s\n", additional_args);
            break;
        case QUALITY:
            rpc_set_quality(additional_args);
            printf("Quality set to %s\n", additional_args);
            break;
		default:
			break;
	}

    if (current_function != SHELL) {
        dontstop = 0;
        currcmd_dontstop = 0;
    }


	int *runner = &dontstop;
	if (current_function != SHELL) runner = &currcmd_dontstop;
	while (*runner) SLEEP(100);

    threading_cleanup_thread(&ssdp_thread);
    threading_cleanup_thread(&notifications_thread);
    printf("\n");
	return 0;
}

void rpc_set_framerate(char *fps) {
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, SET_STREAMER_SETTINGS_FRAMERATE_PNAME, fps);
    print_rpc_result("set framerate", result);
}

void rpc_set_quality(char *q) {
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, SET_STREAMER_SETTINGS_QUALITY_PNAME, q);
    print_rpc_result("set quality ", result);
}

void rpc_set_mode(char *mode) {
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, STREAMING_MODE_PNAME, mode);
    print_rpc_result("set mode ", result);
}

void rpc_set_buffer(char *buff_period) {
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, SET_STREAMER_SETTINGS_BUFF_PNAME, buff_period);
    print_rpc_result("set buffer period", result);
}

void rpc_disconnect(char *ip, char *port) {
    cJSON *result = jrpc_client_call(&cmds_client, DISCONNECT_METHOD, 2, ip, port);
    print_rpc_result("disconnect", result);
}

void rpc_connect(char *ip, char *port) {
    cJSON *result = jrpc_client_call(&cmds_client, CONNECT_METHOD, 2, ip, port);
    print_rpc_result("connect", result);
}


void rpc_init_streamer() {
    cJSON *result = jrpc_client_call(&cmds_client, INIT_STREAMER_METHOD, 0);
    print_rpc_result("init_streamer ", result);
}

void rpc_close_streamer() {
    cJSON *result = jrpc_client_call(&cmds_client, CLOSE_STREAMER_METHOD, 0);
    print_rpc_result("close_streamer", result);
}

void rpc_get_state() {
    cJSON *result = jrpc_client_call(&cmds_client, GET_STATE_METHOD, 0);
    print_rpc_result("get_state", result);
}

void rpc_set_audio_enabled(char *enabled) {
    char *av_caps = "";
    // TODO: improve the JSON RPC APIs and add a specific API to control AUDIO
    if (strcmp("enabled", "1") == 0) {
        av_caps = "3";
    } else {
        av_caps = "1";
    }
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, SET_STREAMER_SETTINGS_AV_CAPS_FLAGS_PNAME, av_caps);
    print_rpc_result("enable audio", result);
}

void rpc_set_fplayback_enabled(char *enabled) {
    cJSON *result = jrpc_client_call(&cmds_client, SET_STREAMER_SETTINGS_METHOD,
            2, SET_STREAMER_SETTINGS_VJB_FLAGS_PNAME, enabled);
    print_rpc_result("fluent playback", result);
}

void print_rpc_result(char *method, cJSON *result) {
    if (result == NULL) {
        printf("\n%s: unknown error with the RPC call", method);
        return;
    }

#if 0 // DEBUG
    char *res_str = cJSON_Print(result);
    printf("\n%s result: %s\n", method, res_str);
    free(res_str);
#endif

    cJSON *res = cJSON_GetObjectItem(result, "result");
    cJSON *error = cJSON_GetObjectItem(result, "error");
    if (error != NULL) {
        cJSON *error_msg = cJSON_GetObjectItem(error, "message");
        if (error_msg != NULL) {
            printf("\n%s error: %s", method, error_msg->valuestring);
        } else {
            printf("\n%s: unknown error with the RPC call", method);
        }
    } else if (res != NULL) {
            printf("\n%s result: %s", method, res->valuestring);
    } else {
            printf("\n%s: unknown error with the RPC call", method);
    }
}
