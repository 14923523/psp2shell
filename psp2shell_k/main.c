#include <vitasdkkern.h>
#include <libk/stdio.h>
#include <taihen.h>
#include <libk/string.h>

#include "include/psp2shell_k.h"

static int sock = 0;

static SceUID g_hooks[2];
static tai_hook_ref_t ref_hooks[2];
static int __stdout_fd = 1073807367;

void set_hooks();

void delete_hooks();

void log_write(const char *msg) {

    //ksceIoMkdir("ux0:/tai/", 6);

    SceUID fd = ksceIoOpen("ux0:/tai/psp2shell_k.log",
                           SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    ksceIoWrite(fd, msg, strlen(msg));
    ksceIoClose(fd);
}

#define LOG(...) \
    do { \
        char buffer[256]; \
        snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
        log_write(buffer); \
    } while (0)

int isprint(int c) {
    return (c >= 0x20 && c <= 0x7E);
}

int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    if (ref_hooks[0] <= 0) {
        return 0;
    }

    if (fd == __stdout_fd && sock > 0) {
        kpsp2shell_print(sock, size, data);
    }

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

int _sceKernelGetStdout() {

    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;

    //LOG("hook: __stdout_fd: 0x%08X\n", __stdout_fd);

    return fd;
}

/*
int _sceClibPrintf(const char *fmt, ...) {

    // user to kernel
    size_t len = 1024;
    char kbuf[len];
    memset(kbuf, 0, len);
    ksceKernelStrncpyUserToKernel(kbuf, (uintptr_t) fmt, len);

    len = strlen(kbuf);
    char buffer[len];
    memset(buffer, 0, len);

    va_list args;
    va_start(args, kbuf);
    vsnprintf(buffer, len, kbuf, args);
    va_end(args);

    if (sock > 0) {
        ksceNetSendto(sock, buffer, strlen(buffer), 0, NULL, 0);
        ksceNetRecvfrom(sock, buffer, 2, 0x1000, NULL, 0);
    }

    //return 0;
    return TAI_CONTINUE(int, ref_hooks[0], fmt, args);
}
*/

void kpsp2shell_set_sock(int s) {
    sock = s;
    if (sock > 0) {
        set_hooks();
    } else {
        delete_hooks();
    }
}

void kpsp2shell_print(int s, unsigned int size, const char *msg) {

    if (size > 256) {
        return;
    }

    char buf[size];
    memset(buf, 0, size);

    uint32_t state;
    ENTER_SYSCALL(state);

    ksceKernelStrncpyUserToKernel(buf, (uintptr_t) msg, size);

    /*
    // remove invalid chars
    char buf_clean[size];
    memset(buf_clean, 0, size);
    for (int i = 0; i < size; i++) {
        buf_clean[i] = (char) (isprint(buf[i]) ? buf[i] : '?');
    }
    buf_clean[size - 1] = '\0';
    */

    ksceNetSendto(s, buf, size, 0, NULL, 0);
    ksceNetRecvfrom(s, buf, 2, 0x1000, NULL, 0);

    EXIT_SYSCALL(state);
}

void set_hooks() {

    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceIofilemgr",
            0xF2FF276E,
            0x34EFD876,
            _sceIoWrite);
    //LOG("hook: sceIoWrite: 0x%08X\n", g_hooks[0]);

    g_hooks[1] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            "SceProcessmgr",
            0x2DD91812,
            0xE5AA625C,
            _sceKernelGetStdout);
    //LOG("hook: sceKernelGetStdout: 0x%08X\n", g_hooks[1]);
}

void delete_hooks() {

    if (g_hooks[0] >= 0)
        taiHookReleaseForKernel(g_hooks[0], ref_hooks[0]);

    if (g_hooks[1] >= 0)
        taiHookReleaseForKernel(g_hooks[1], ref_hooks[1]);
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    /*
    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceLibKernel",
            0xCAE9ACE6,
            0xFA26BC62,
            _sceClibPrintf);
    */

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    return SCE_KERNEL_STOP_SUCCESS;
}
