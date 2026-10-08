// MyLoader - هيكل إضافة GoldHEN أصلي: محمّل ملفات بنظام الخانات (slots)
// ملاحظة: راجع أسماء الدوال والـ includes مع أمثلة GoldHEN Plugin SDK عندك،
// لأنها بتختلف بين إصدارات الـ SDK.

#include <Common.h>          // من GoldHEN Plugin SDK
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

attr_public const char *g_pluginName = "MyLoader";
attr_public const char *g_pluginDesc = "Slot-based file loader";
attr_public const char *g_pluginAuth = "7md";
attr_public uint32_t g_pluginVersion = 0x00000100;

#define SCRIPTS_DIR "/data/MyLoader/Scripts"
#define MAX_SLOTS   16

struct Slot {
    bool  used;
    char  name[128];
    char *data;
    size_t size;
};

static Slot g_slots[MAX_SLOTS];

static int FindSlotByName(const char *name) {
    for (int i = 0; i < MAX_SLOTS; i++)
        if (g_slots[i].used && strcmp(g_slots[i].name, name) == 0) return i;
    return -1;
}

static int FindFreeSlot() {
    for (int i = 0; i < MAX_SLOTS; i++)
        if (!g_slots[i].used) return i;
    return -1;
}

static bool ReadFile(const char *path, char **out, size_t *outSize) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 4 * 1024 * 1024) { fclose(f); return false; }
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return false; }
    size_t n = fread(buf, 1, sz, f);
    fclose(f);
    buf[n] = 0;
    *out = buf;
    *outSize = n;
    return true;
}

// رفض الملفات UTF-16 (BOM) زي ما لاحظنا في الأداة الأصلية كفكرة عامة
static bool LooksLikeUtf16(const char *d, size_t n) {
    return n >= 2 && (((unsigned char)d[0] == 0xFF && (unsigned char)d[1] == 0xFE) ||
                      ((unsigned char)d[0] == 0xFE && (unsigned char)d[1] == 0xFF));
}

static void UnloadSlot(int i) {
    if (i < 0 || i >= MAX_SLOTS || !g_slots[i].used) return;
    final_printf("[Loader] Unloading slot %d: %s\n", i, g_slots[i].name);
    free(g_slots[i].data);
    memset(&g_slots[i], 0, sizeof(Slot));
}

static bool LoadFile(const char *fileName) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", SCRIPTS_DIR, fileName);

    char *data = nullptr;
    size_t size = 0;
    if (!ReadFile(path, &data, &size)) {
        final_printf("[Loader] Failed to read %s\n", path);
        return false;
    }
    if (LooksLikeUtf16(data, size)) {
        final_printf("[Loader] %s is UTF-16, save it as UTF-8\n", fileName);
        free(data);
        return false;
    }

    int idx = FindSlotByName(fileName);
    if (idx >= 0) {
        final_printf("[Loader] Replacing existing file at slot %d\n", idx);
        UnloadSlot(idx);
    } else {
        idx = FindFreeSlot();
        if (idx < 0) {
            final_printf("[Loader] No free slots\n");
            free(data);
            return false;
        }
    }

    g_slots[idx].used = true;
    strncpy(g_slots[idx].name, fileName, sizeof(g_slots[idx].name) - 1);
    g_slots[idx].data = data;
    g_slots[idx].size = size;
    final_printf("[Loader] Loaded %s (%zu bytes) into slot %d\n", fileName, size, idx);

    // هنا تحط اللي تحب تعمله بمحتوى الملف (parse إعدادات، قراءة قائمة، إلخ)
    return true;
}

static void RefreshAll() {
    DIR *dir = opendir(SCRIPTS_DIR);
    if (!dir) {
        final_printf("[Loader] Folder not found: %s\n", SCRIPTS_DIR);
        return;
    }
    int found = 0;
    struct dirent *e;
    while ((e = readdir(dir)) != nullptr) {
        if (e->d_name[0] == '.') continue;
        found++;
        LoadFile(e->d_name);
    }
    closedir(dir);
    final_printf("[Loader] Total files found: %d\n", found);
}

extern "C" {

int32_t attr_module_hidden module_start(size_t argc, const void *args) {
    final_printf("[%s] v1.0 starting\n", g_pluginName);
    notify("%s loaded successfully\nBy 7md", g_pluginName);
    memset(g_slots, 0, sizeof(g_slots));
    RefreshAll();
    return 0;
}

int32_t attr_module_hidden module_stop(size_t argc, const void *args) {
    for (int i = 0; i < MAX_SLOTS; i++) UnloadSlot(i);
    final_printf("[%s] stopped\n", g_pluginName);
    return 0;
}

}
