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


#include "libmodule.h"
#include "utility.h"
#include "psp2shell.h"
#include "module.h"

#ifndef DEBUG
#include "../../psp2shell_k/psp2shell_k.h"
#endif

#ifndef __KERNEL__
static void printModuleInfoFull(SceKernelModuleInfo *moduleInfo) {

    PRINT_OK("module_name: %s\n", moduleInfo->module_name);
    PRINT_OK("\tpath: %s\n", moduleInfo->path);
    PRINT_OK("\thandle: 0x%08X\n", moduleInfo->handle);
    PRINT_OK("\tflags: 0x%08X\n", moduleInfo->flags);
    PRINT_OK("\tmodule_start: 0x%08X\n", moduleInfo->module_start);
    PRINT_OK("\tmodule_stop: 0x%08X\n", moduleInfo->module_stop);
    PRINT_OK("\texidxTop: 0x%08X\n", moduleInfo->exidxTop);
    PRINT_OK("\texidxBtm: 0x%08X\n", moduleInfo->exidxBtm);
    PRINT_OK("\ttlsInit: 0x%08X\n", moduleInfo->tlsInit);
    PRINT_OK("\ttlsInitSize: 0x%08X\n", moduleInfo->tlsInitSize);
    PRINT_OK("\ttlsAreaSize: 0x%08X\n", moduleInfo->tlsAreaSize);
    PRINT_OK("\ttype: %i\n", moduleInfo->type);
    PRINT_OK("\tunk28: 0x%08X\n", moduleInfo->unk28);
    PRINT_OK("\tunk30: 0x%08X\n", moduleInfo->unk30);
    PRINT_OK("\tunk40: 0x%08X\n", moduleInfo->unk40);
    PRINT_OK("\tunk44: 0x%08X\n", moduleInfo->unk44);
    for (int i = 0; i < 4; ++i) {
        if (moduleInfo->segments[i].memsz <= 0) {
            continue;
        }
        PRINT_OK("\tsegment[%i].perms: 0x%08X\n", i, moduleInfo->segments[i].perms);
        PRINT_OK("\tsegment[%i].vaddr: 0x%08X\n", i, moduleInfo->segments[i].vaddr);
        PRINT_OK("\tsegment[%i].memsz: 0x%08X\n", i, moduleInfo->segments[i].memsz);
        PRINT_OK("\tsegment[%i].flags: 0x%08X\n", i, moduleInfo->segments[i].flags);
        PRINT_OK("\tsegment[%i].res: %i\n", i, moduleInfo->segments[i].res);
    }
    PRINT_OK("\n\n");
}
#endif

int p2s_moduleInfo(SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleInfo\n");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleInfoForPid(pid, uid);
#endif
}

int p2s_moduleInfoForPid(SceUID pid, SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleInfoForPid\n");
    return 0;
#else
    SceKernelModuleInfo moduleInfo;
    memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
    moduleInfo.size = sizeof(SceKernelModuleInfo);

#ifdef DEBUG
    int res = sceKernelGetModuleInfo(uid, &moduleInfo);
#else
    int res = kpsp2shell_get_module_info(pid, uid, &moduleInfo);
#endif
    if (res == 0) {
        printModuleInfoFull(&moduleInfo);
    } else {
        PRINT_ERR("\ngetting module info failed: 0x%08X\n", res);
    }

    return res;
#endif
}

int p2s_moduleList() {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleList");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleListForPid(pid);
#endif
}

int p2s_moduleListForPid(SceUID pid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleListForPid\n");
    return 0;
#else
    SceUID ids[256];
    size_t count = 256;

#ifdef DEBUG
    int res = sceKernelGetModuleList(0xFF, ids, (int *) &count);
#else
    int res = kpsp2shell_get_module_list(pid, 0xFF, 1, ids, &count);
#endif
    if (res != 0) {
        PRINT_ERR("module list failed: 0x%08X\n", res);
        return res;
    } else {
        SceKernelModuleInfo moduleInfo;
        for (int i = 0; i < count; i++) {
            if (ids[i] > 0) {
                memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
#ifdef DEBUG
                moduleInfo.size = sizeof(SceKernelModuleInfo);
                res = sceKernelGetModuleInfo(ids[i], &moduleInfo);
#else
                res = kpsp2shell_get_module_info(pid, ids[i], &moduleInfo);
#endif
                if (res == 0) {
                    PRINT_OK("%s (uid: 0x%08X)\n",
                             moduleInfo.module_name, moduleInfo.handle);
                }
            }
        }
    }

    return 0;
#endif
}

SceUID p2s_moduleLoadStart(char *modulePath) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleLoadStart\n");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleLoadStartForPid(pid, modulePath);
#endif
}

SceUID p2s_moduleLoadStartForPid(SceUID pid, char *modulePath) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleLoadStartForPid\n");
    return 0;
#else
    SceUID uid = taiLoadStartModuleForPid(pid, modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("\nmodule load/start failed: 0x%08X\n\n", uid);
    } else {
        PRINT_OK("\nmodule loaded/started: uid = 0x%08X\n\n", uid);
    }

    return uid;
#endif
}

int p2s_moduleStopUnload(SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleStopUnload\n");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleStopUnloadForPid(pid, uid);
#endif
}

int p2s_moduleStopUnloadForPid(SceUID pid, SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleStopUnloadForPid\n");
    return 0;
#else
    int status;

    int res = taiStopUnloadModuleForPid(pid, uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("\nmodule stop/unload failed: 0x%08X\n\n", status);
    } else {
        PRINT_OK("\nmodule stopped/unloaded\n\n");
    }

    return res;
#endif
}

SceUID p2s_kmoduleLoadStart(char *modulePath) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_kmoduleLoadStart\n");
    return 0;
#else
    SceUID uid = taiLoadStartKernelModule(modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("\nmodule load/start failed: 0x%08X\n\n", uid);
    } else {
        PRINT_OK("\nmodule loaded/started: uid = 0x%08X\n\n", uid);
    }

    return uid;
#endif
}

int p2s_kmoduleStopUnload(SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_kmoduleStopUnload\n");
    return 0;
#else
    int status;

    int res = taiStopUnloadKernelModule(uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("\nmodule stop/unload failed: 0x%08X\n\n", status);
    } else {
        PRINT_OK("\nmodule stopped/unloaded\n\n");
    }

    return res;
#endif
}

int p2s_moduleDumpForPid(SceUID pid, SceUID uid, const char *dst) {

#ifdef DEBUG
    PRINT_ERR("TODO: p2s_moduleDumpForPid\n");
    return -1;
#else
    int res = kpsp2shell_dump_module(pid, uid, dst);
    if (res != 0) {
        PRINT_ERR("\nmodule dump failed: 0x%08X\n\n", res);
    } else {
        PRINT_OK("\nmodule dump success\n\n");
    }

    return res;
#endif
}
