// MyLoader: shows a notification when the plugin loads.
// Author: 7md

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "GoldHEN.h"

#include <orbis/libkernel.h>

#include "plugin_common.h"

attr_public const char *g_pluginName = "MyLoader";
attr_public const char *g_pluginDesc = "Shows a message when loaded";
attr_public const char *g_pluginAuth = "7md";
attr_public u32 g_pluginVersion = 0x00000100; // 1.00

// Called by plugin_loader after the game starts
int32_t attr_public plugin_load(int32_t argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    NotifyStatic(TEX_ICON_SYSTEM, "MyLoader loaded successfully\nBy 7md");
    return 0;
}

int32_t attr_public plugin_unload(int32_t argc, const char *argv[])
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    return 0;
}

int32_t attr_module_hidden module_start(size_t argc, const void *args)
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    return 0;
}

int32_t attr_module_hidden module_stop(size_t argc, const void *args)
{
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    return 0;
}
