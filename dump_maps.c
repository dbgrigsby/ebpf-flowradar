#define _GNU_SOURCE
#include "bpf_load.h"
#include "common.h"
#include "cjson/cJSON.h"

#include <linux/bpf.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

int map_pin_fds[NUM_MAP_PINS];
#define KEY_NAME_MAX 32
#define NUM_VALUE_MAX 128
char key_name[KEY_NAME_MAX];
char num_value[NUM_VALUE_MAX];

void dump_hash(int map_fd, cJSON *map_data, int index) {
    uint32_t key = -1, next_key;
    uint64_t value;

    while (bpf_map_get_next_key(map_fd, &key, &next_key) == 0) {
        key = next_key;
        if ((bpf_map_lookup_elem(map_fd, &key, &value)) != 0) {
            fprintf(
                stderr,
                "ERR: failed to read key %x from map(%d): %s\n",
                key, errno, strerror(errno)
            );
        }
        if (value) {
            if ((map_pins[index].format_key)(&key, key_name, KEY_NAME_MAX) <= 0)
                continue;
            if ((map_pins[index].format_value)(&value, num_value, NUM_VALUE_MAX) <= 0)
                continue;

            if (cJSON_AddStringToObject(map_data, key_name, num_value) == NULL)
                fprintf(
                    stderr,
                    "ERR: Failed to add key(value) %x(%lu) to json\n",
                    key, value
                );
        }
    }
}


int main() {
    int fd, i;
    for (i=0; i<NUM_MAP_PINS; i++) {
        fd = bpf_obj_get(map_pins[i].path);
        if (fd <= 0) {
            fprintf(stderr,
                "ERR: Failed to load map pin %s(%d): %s\n",
                map_pins[i].path, errno, strerror(errno)
            );
            return 1;
        }
        map_pin_fds[i] = fd;
    }

    char *string = NULL;
    cJSON *data = cJSON_CreateObject();

    for (i=0; i<NUM_MAP_PINS; i++) {
        cJSON *map_data = cJSON_AddObjectToObject(data, map_pins[i].name);
        dump_hash(map_pin_fds[i], map_data, i);
    }

    string = cJSON_Print(data);
    printf("%s\n", string);

    cJSON_Delete(data);

    return 0;
}
