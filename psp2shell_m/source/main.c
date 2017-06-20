/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/net/net.h>
#include <main.h>

#include "../include/main.h"
#include "../include/utility.h"
#include "../include/psp2shell.h"
#include "../include/taipool.h"
#include "../include/libmodule.h"
#include "../../psp2shell_k/psp2shell_k.h"

static SceUID thid_wait = -1;
static SceUID thid_kbuf = -1;
static SceUID thid_client = -1;
#ifdef DEBUG
static int listen_port = 5555;
#else
static int listen_port = 3333;
#endif
static s_client *client;
static int server_sock_msg;
static int server_sock_cmd;
static int quit = 0;

void p2s_cmd_parse(s_client *client, P2S_CMD *cmd);

static void close_server() {

    if (server_sock_msg >= 0) {
        sceNetSocketClose(server_sock_msg);
        server_sock_msg = -1;
    }
    if (server_sock_cmd >= 0) {
        sceNetSocketClose(server_sock_cmd);
        server_sock_cmd = -1;
    }
}

static int open_server() {

    close_server();

    server_sock_msg = p2s_bind_port(server_sock_msg, listen_port);
    if (server_sock_msg <= 0) {
        return -1;
    }
    server_sock_cmd = p2s_bind_port(server_sock_cmd, listen_port + 1);
    if (server_sock_cmd <= 0) {
        return -1;
    }

    return 0;
}

void psp2shell_print_color_advanced(SceSize size, int color, const char *fmt, ...) {

    if (size >= P2S_KMSG_SIZE || client == NULL) {
        return;
    }

    client->msg.color = color;
    memset(client->msg.buffer, 0, size);
    va_list args;
    va_start(args, fmt);
    vsnprintf(client->msg.buffer, size, fmt, args);
    va_end(args);

    if (client->msg_sock > 0) {
        printf("p2s_cmd_send_fmt: %s\n", client->msg.buffer);
        p2s_cmd_send_fmt(client->msg_sock, "%i\"%s\"", color, client->msg.buffer);
        // TODO: wait for client to receive message, should maybe add a timeout
        p2s_cmd_wait_result(client->msg_sock);
    }
}

static void welcome() {

    char msg[1024];
    memset(msg, 0, 1024);
    strcpy(msg, "\n");
    strcat(msg, "                     ________         .__           .__  .__   \n");
    strcat(msg, "______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n");
    strcat(msg, "\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n");
    strcat(msg, "|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n");
    strcat(msg, "|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n");
    sprintf(msg + strlen(msg), "|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", VERSION);
    psp2shell_print_color(COL_GREEN, msg);
}

int cmd_thread(SceSize args, void *argp) {

    printf("cmd_thread\n");

    int sock = *((int *) argp);

    // setup clients data
    client = taipool_alloc(sizeof(s_client));
    if (client == NULL) { // crap
        printf("client alloc failed\n");
        sceNetSocketClose(sock);
        sceKernelExitDeleteThread(0);
        return 0;
    }

    memset(client, 0, sizeof(s_client));
    client->msg_sock = sock;

    // init client file listing memory/**/
    memset(&client->fileList, 0, sizeof(s_FileList));
    strcpy(client->fileList.path, HOME_PATH);
    s_fileListGetEntries(&client->fileList, HOME_PATH);

    // Welcome!
    printf("welcome client\n");
    welcome();

    // get data sock
    client->cmd_sock = p2s_get_sock(server_sock_cmd);
    printf("got data sock: %i\n", client->cmd_sock);

#ifndef DEBUG
    kpsp2shell_set_ready(1);
#endif

    P2S_CMD cmd;

    while (!quit) {

        int res = p2s_cmd_receive(client->cmd_sock, &cmd);
        if (res != 0) {
            if (res == P2S_ERR_SOCKET) {
                PRINT_ERR("p2s_cmd_receive sock failed: 0x%08X\n", res);
                break;
            } else {
                PRINT_ERR("p2s_cmd_receive failed: 0x%08X\n", res);
            }
        } else {
            p2s_cmd_parse(client, &cmd);
        }
    }

    printf("closing connection\n");
#ifndef DEBUG
    kpsp2shell_set_ready(0);
#endif
    if (client->msg_sock >= 0) {
        sceNetSocketClose(client->msg_sock);
        client->msg_sock = -1;
    }
    if (client->cmd_sock >= 0) {
        sceNetSocketClose(client->cmd_sock);
        client->cmd_sock = -1;
    }
    s_fileListEmpty(&client->fileList);
    taipool_free(client);
    client = NULL;

    sceKernelExitDeleteThread(0);
    return 0;
}

static int thread_wait(SceSize args, void *argp) {

    // setup sockets
    if (open_server() != 0) {
        psp2shell_exit();
        sceKernelExitDeleteThread(0);
        return -1;
    }

    // accept incoming connections
    printf("Waiting for connections...\n");
    while (!quit) {

        int client_sock = p2s_get_sock(server_sock_msg);
        if (client_sock < 0) {
            if (quit) {
                break;
            }
            printf("network disconnected: reset (%i)\n", client_sock);
            sceKernelDelayThread(1000 * 1000);
            open_server();
            continue;
        } else if (client_sock == 0) {
            printf("client_sock == 0\n");
        }

        if (quit) {
            break;
        }

        printf("new connexion on port %i (sock=%i)\n", listen_port, client_sock);
        // max client/socket count reached
        if (client != NULL && client->cmd_sock > 0) {
            printf("Connection refused, max client reached (1)\n");
            sceNetSocketClose(client_sock);
            continue;
        }

        printf("Connection accepted\n");
        thid_client = sceKernelCreateThread("psp2shell_cmd", cmd_thread, 64, 0x4000, 0, 0x10000, 0);
        if (thid_client >= 0)
            sceKernelStartThread(thid_client, sizeof(int), (void *) &client_sock);
    }

    psp2shell_exit();

    sceKernelExitDeleteThread(0);
    return 0;
}

#ifndef DEBUG

static int thread_kbuf(SceSize args, void *argp) {

    char buffer[P2S_KMSG_SIZE];

    while (!quit) {

        if (client != NULL) {
            SceSize len = kpsp2shell_wait_buffer(buffer);
            if (client != NULL && client->msg_sock > 0 && len > 0) {
                psp2shell_print_color_advanced(len, 0, "%s", buffer);
            } else {
                sceKernelDelayThread(100);
            }
        } else {
            sceKernelDelayThread(100);
        }
    }

    sceKernelExitDeleteThread(0);
    return 0;
}

#endif

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    // init pool
    int res = taipool_init_advanced(0x10000, POOL_TYPE_BLOCK); // 64K
    printf("taipool_init_advanced(%i): 0x%08X\n", 0x10000, res);

    // load network modules
    p2s_netInit();

#ifndef DEBUG
    thid_kbuf = sceKernelCreateThread("psp2shell_kbuf", thread_kbuf, 64, 0x2000, 0, 0x10000, 0);
    if (thid_kbuf >= 0) {
        sceKernelStartThread(thid_kbuf, 0, NULL);
    }
#endif
    thid_wait = sceKernelCreateThread("psp2shell_wait", thread_wait, 64, 0x1000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        sceKernelStartThread(thid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    psp2shell_exit();
    return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void) {

}

void psp2shell_exit() {

    if (quit) {
        return;
    }

    printf("psp2shell_exit\n");
    quit = 1;

#ifndef DEBUG
    kpsp2shell_set_ready(0);
#endif

    printf("close_client\n");
    if (client != NULL) {
        if (client->msg_sock >= 0) {
            sceNetSocketClose(client->msg_sock);
            client->msg_sock = -1;
        }
        if (client->cmd_sock >= 0) {
            sceNetSocketClose(client->cmd_sock);
            client->cmd_sock = -1;
        }
        if (thid_client >= 0) {
            sceKernelWaitThreadEnd(thid_client, NULL, NULL);
        }
    }

    printf("close_server\n");
    close_server();
    if (thid_wait >= 0) {
        sceKernelWaitThreadEnd(thid_wait, NULL, NULL);
    }

    if (thid_kbuf >= 0) {
        printf("sceKernelDeleteThread: thid_kbuf\n");
        sceKernelDeleteThread(thid_kbuf);
    }

    printf("taipool_term\n");
    taipool_term();
}
