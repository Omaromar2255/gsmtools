/**
 * gt_host_compat.h — host (Linux) build shims for at_bridge.c
 * ============================================================
 *  Lets the exact same at_bridge.c compile and run on a desktop Linux
 *  box so the session protocol can be exercised end-to-end against the
 *  mock ADB server (tests/mock_adb_server.py) without a real phone.
 *
 *  - gt_system_property_get(): reads "key=value" lines from the file in
 *    $GT_PROPS_FILE (default ./gt_host_props.txt). Missing keys -> 0.
 *  - PROP_VALUE_MAX: bionic-compatible 92.
 *  - dlopen / libsecril: intentionally absent on host (GT_HAVE_SECRIL=0).
 */
#ifndef GT_HOST_COMPAT_H
#define GT_HOST_COMPAT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROP_VALUE_MAX 92

static int gt_system_property_get(const char *key, char *value) {
    if (!key || !value) return 0;
    value[0] = '\0';

    const char *props_file = getenv("GT_PROPS_FILE");
    if (!props_file || !*props_file) props_file = "gt_host_props.txt";

    FILE *f = fopen(props_file, "r");
    if (!f) return 0;

    char line[256];
    size_t klen = strlen(key);
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char *nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';
        if (strncmp(line, key, klen) == 0 && line[klen] == '=') {
            snprintf(value, PROP_VALUE_MAX, "%s", line + klen + 1);
            found = (int)strlen(value);
            break;
        }
    }
    fclose(f);
    return found;
}

#endif /* GT_HOST_COMPAT_H */
