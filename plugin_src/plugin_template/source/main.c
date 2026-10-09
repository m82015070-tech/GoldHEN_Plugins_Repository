// ============================================================
//  7md Script Loader  v1.0
//  Author  : 7md
//  Discord : just_7md
//  Path    : /data/7mdXscripts
// ============================================================

#include "plugin_common.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/SystemService.h>

// بعض الدوال بتكون متاحة من خلال الـ SDK مباشرة
extern int sceKernelMkdir(const char *path, int mode);
extern int sceKernelUsleep(unsigned int microseconds);
extern int scePthreadCreate(ScePthread *thread, const ScePthreadAttr *attr, void *(*entry)(void *), void *arg, const char *name);
extern int scePthreadJoin(ScePthread thread, void **value_ptr);

#define PLUGIN_NAME         "7md_ScriptLoader"
#define SCRIPTS_DIR         "/data/7mdXscripts"
#define MAX_SCRIPTS         64
#define MAX_NAME_LEN        128

attr_public const char *g_pluginName = PLUGIN_NAME;
attr_public const char *g_pluginDesc = "7md Script Loader | Discord: just_7md";
attr_public const char *g_pluginAuth = "7md";
attr_public u32 g_pluginVersion = 0x00010000;

// ===================== Structures =====================
typedef struct {
    char name[MAX_NAME_LEN];
    int  loaded;
} ScriptEntry;

// ===================== Globals =====================
static ScriptEntry g_scripts[MAX_SCRIPTS];
static int  g_scriptCount = 0;
static int  g_selected    = 0;
static int  g_menuOpen    = 0;
static int  g_comboLast   = 0;
static int  g_running     = 1;
static ScePthread g_thread;
static int  g_padHandle   = -1;

// ===================== Helpers =====================
static void Notify(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    final_printf("[7md] %s\n", buf);
}

// ===================== Folder =====================
static void EnsureFolder(void) {
    sceKernelMkdir(SCRIPTS_DIR, 0777);
}

// ===================== Script List =====================
static void RefreshScripts(void) {
    g_scriptCount = 0;
    g_selected = 0;

    // قائمة تجريبية عشان القائمة تظهر وتشتغل
    const char *demo[] = {
        "godmode.oxc",
        "money.oxc",
        "never_wanted.oxc",
        "give_weapons.oxc",
        "teleport.oxc"
    };

    for (int i = 0; i < 5 && g_scriptCount < MAX_SCRIPTS; i++) {
        strncpy(g_scripts[g_scriptCount].name, demo[i], MAX_NAME_LEN - 1);
        g_scripts[g_scriptCount].name[MAX_NAME_LEN - 1] = '\0';
        g_scripts[g_scriptCount].loaded = 0;
        g_scriptCount++;
    }

    Notify("Scripts refreshed (%d found)", g_scriptCount);
}

// ===================== Load / Unload =====================
static void LoadSelected(void) {
    if (g_scriptCount <= 0) {
        Notify("No scripts available");
        return;
    }
    if (g_scripts[g_selected].loaded) {
        Notify("Already loaded: %s", g_scripts[g_selected].name);
        return;
    }

    g_scripts[g_selected].loaded = 1;
    Notify("Loaded: %s", g_scripts[g_selected].name);
}

static void UnloadSelected(void) {
    if (g_scriptCount <= 0) return;
    if (!g_scripts[g_selected].loaded) {
        Notify("Not loaded: %s", g_scripts[g_selected].name);
        return;
    }

    g_scripts[g_selected].loaded = 0;
    Notify("Unloaded: %s", g_scripts[g_selected].name);
}

static void UnloadAll(void) {
    int count = 0;
    for (int i = 0; i < g_scriptCount; i++) {
        if (g_scripts[i].loaded) {
            g_scripts[i].loaded = 0;
            count++;
        }
    }
    Notify("Unloaded %d scripts", count);
}

// ===================== Draw Menu =====================
static void DrawMenu(void) {
    if (!g_menuOpen) return;

    final_printf("\n");
    final_printf("========================================\n");
    final_printf("          7md Script Loader\n");
    final_printf("        Discord: just_7md\n");
    final_printf("     Path: /data/7mdXscripts\n");
    final_printf("========================================\n");

    if (g_scriptCount == 0) {
        final_printf("  No scripts found\n");
        final_printf("  Put .oxc files in the folder\n");
    } else {
        for (int i = 0; i < g_scriptCount; i++) {
            if (i == g_selected)
                final_printf(" > %s %s\n", g_scripts[i].name, g_scripts[i].loaded ? "[ON]" : "");
            else
                final_printf("   %s %s\n", g_scripts[i].name, g_scripts[i].loaded ? "[ON]" : "");
        }
    }

    final_printf("----------------------------------------\n");
    final_printf(" X = Load          Square = Unload\n");
    final_printf(" Triangle = Unload All\n");
    final_printf(" O = Close         Options = Refresh\n");
    final_printf(" R1 + R2 = Toggle Menu\n");
    final_printf("========================================\n");
}

// ===================== Input + Tick =====================
static void Tick(void) {
    ScePadData pad;
    memset(&pad, 0, sizeof(pad));

    if (g_padHandle >= 0) {
        scePadReadState(g_padHandle, &pad);
    }

    int r1 = (pad.buttons & SCE_PAD_BUTTON_R1) != 0;
    int r2 = (pad.buttons & SCE_PAD_BUTTON_R2) != 0;

    if (r1 && r2) {
        if (!g_comboLast) {
            g_menuOpen = !g_menuOpen;
            if (g_menuOpen) {
                RefreshScripts();
                Notify("Menu Opened");
            } else {
                Notify("Menu Closed");
            }
            g_comboLast = 1;
        }
    } else {
        g_comboLast = 0;
    }

    if (!g_menuOpen) return;

    static uint32_t lastButtons = 0;
    uint32_t pressed = pad.buttons & \~lastButtons;
    lastButtons = pad.buttons;

    if (pressed & SCE_PAD_BUTTON_UP) {
        if (g_selected > 0) g_selected--;
    }
    if (pressed & SCE_PAD_BUTTON_DOWN) {
        if (g_selected < g_scriptCount - 1) g_selected++;
    }
    if (pressed & SCE_PAD_BUTTON_CROSS) {
        LoadSelected();
    }
    if (pressed & SCE_PAD_BUTTON_SQUARE) {
        UnloadSelected();
    }
    if (pressed & SCE_PAD_BUTTON_TRIANGLE) {
        UnloadAll();
    }
    if (pressed & SCE_PAD_BUTTON_CIRCLE) {
        g_menuOpen = 0;
        Notify("Menu Closed");
    }
    if (pressed & SCE_PAD_BUTTON_OPTIONS) {
        RefreshScripts();
    }

    DrawMenu();
}

// ===================== Thread =====================
static void* LoaderThread(void *arg) {
    sceKernelUsleep(12000000); // استنى 12 ثانية

    EnsureFolder();

    // Pad init
    scePadInit();

    OrbisUserServiceInitializeParams param;
    memset(&param, 0, sizeof(param));
    param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&param);

    int userId = 0;
    sceUserServiceGetInitialUser(&userId);
    g_padHandle = scePadOpen(userId, SCE_PAD_PORT_TYPE_STANDARD, 0, NULL);

    RefreshScripts();
    Notify("7md Script Loader ready - Discord: just_7md");

    while (g_running) {
        Tick();
        sceKernelUsleep(16000);
    }

    if (g_padHandle >= 0)
        scePadClose(g_padHandle);

    return NULL;
}

// ===================== Plugin Entry =====================
s32 attr_public plugin_load(s32 argc, const char *argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    scePthreadCreate(&g_thread, NULL, LoaderThread, NULL, "7md_Loader");
    return 0;
}

s32 attr_public plugin_unload(s32 argc, const char *argv[]) {
    g_running = 0;
    scePthreadJoin(g_thread, NULL);
    final_printf("[GoldHEN] <%s> %s\n", g_pluginName, __func__);
    return 0;
}
