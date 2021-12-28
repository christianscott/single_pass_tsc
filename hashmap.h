#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define UNREACHABLE(...)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, "unreachable: " __VA_ARGS__);                                                                  \
        exit(1);                                                                                                       \
    } while (0)

typedef struct
{
	bool in_use;
	char* key;
	int val;
} HashmapEntry;

typedef struct
{
	int cap;
	int size;
	HashmapEntry* entries;
} Hashmap;

void hm_init(Hashmap* hm)
{
	hm->cap = 2;
	hm->size = 0;
	hm->entries = calloc(hm->cap, sizeof(HashmapEntry));
}

void hm_ensure(Hashmap* hm, int min_cap)
{
	if (hm->cap >= min_cap)
	{
		return;
	}

	int prev_cap = hm->cap;
	hm->cap = prev_cap * 2;
	hm->entries = realloc(hm->entries, sizeof(HashmapEntry) * hm->cap);
	for (int i = prev_cap; i < hm->cap; i++)
	{
		hm->entries[i] = (const HashmapEntry){ 0 };
	}
}

void hm_add(Hashmap* hm, char* key, int val)
{
	hm_ensure(hm, ++hm->size);

	for (int i = 0; i < hm->cap; i++)
	{
		if (!hm->entries[i].in_use)
		{
			HashmapEntry entry = { .in_use = true, .key = key, .val = val };
			hm->entries[i] = entry;
			return;
		}

		if (strcmp(hm->entries[i].key, key) == 0)
		{
			hm->entries[i].val = val;
			return;
		}
	}

	UNREACHABLE("could not insert key '%s' into hashmap\n", key);
}

bool hm_get(Hashmap* hm, char* key, int* result)
{
	for (int i = 0; i < hm->cap; i++)
	{
		HashmapEntry entry = hm->entries[i];
		if (!entry.in_use)
		{
			continue;
		}

		if (strcmp(entry.key, key) == 0)
		{
			*result = entry.val;
			return true;
		}
	}

	return false;
}

bool hm_has(Hashmap* hm, char* key)
{
	int dummy;
	return hm_get(hm, key, &dummy);
}
