/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>     /* uint8_t ... uint64_t */
#include <string.h>     /* strncmp */
#include <stdlib.h>     /* size_t */
#include "lfs.h"
#include "tests.h"

//--------------------------------------------
extern const test_t test1w;
extern const test_t test1u;
extern const test_t test2w;
extern const test_t test2u;
extern const test_t test3w;
extern const test_t test3u;

//--------------------------------------------
TESTS(
	&test1w,
	&test1u,
	&test2w,
	&test2u,
	&test3w,
	&test3u
);

//--------------------------------------------
const test_t *get_test(char *id)
{
	size_t cnt;
	for (cnt = 0; ; cnt++)
	{
		if (tests[cnt] == NULL)
		{
			return NULL;
		}
		if (!strncmp(id, tests[cnt]->id, sizeof(((test_t *)0)->id)))
		{
			return tests[cnt];
		}
	}
}