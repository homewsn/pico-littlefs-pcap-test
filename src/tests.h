/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TESTS_H_
#define TESTS_H_

 //--------------------------------------------
typedef struct
{
	char *id;
	char *file;
	void(*littlefs_init)(void);
	void(*littlefs_reload)(void);
	void(*littlefs_check)(void);
	void(*littlefs_cleanup)(void);
} test_t;

//--------------------------------------------
#define TEST(name, id, file, littlefs_init, littlefs_reload, littlefs_check, littlefs_cleanup) \
	const test_t name = { id, file, littlefs_init, littlefs_reload, littlefs_check, littlefs_cleanup }

//--------------------------------------------
#define TESTS(...) \
	const test_t *tests[] = { __VA_ARGS__, NULL }

//--------------------------------------------
const test_t *get_test(char *id);

//--------------------------------------------
#define ANSI_CLEAR     "\x1b[0m"
#define ANSI_YELLOW    "\x1b[33m"


#endif /* TESTS_H_ */
