/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>     /* uint8_t ... uint64_t */
#include <string.h>     /* strncmp */
#include <stdlib.h>     /* size_t */
#include "lfs.h"
#include "mimic_fat.h"
#include "tests.h"
#include "prng.h"

//--------------------------------------------
#define DIR_NAME         "test"
#define FILE_NAME        "test/aaa.bin"
#define TEST_STR_2       "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf"
#define TEST_STR_CNT_2   7500
#define FILE_SIZE_2      ((sizeof(TEST_STR_2) - 1) * TEST_STR_CNT_2)

//--------------------------------------------
extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
static lfs_t lfs;

//--------------------------------------------
static void init(void)
{
	int res;
	struct lfs_info finfo;

	res = lfs_mount(&lfs, &lfs_pico_flash_config);
	if (res)
	{
		printf(ANSI_YELLOW"\r\n-----------------\r\nlittlefs_init: format and mount\r\n"ANSI_CLEAR);
		res = lfs_format(&lfs, &lfs_pico_flash_config);
		res = lfs_mount(&lfs, &lfs_pico_flash_config);
	}

	res = lfs_stat(&lfs, DIR_NAME, &finfo);
	if (res == LFS_ERR_NOENT)
	{
		res = lfs_mkdir(&lfs, DIR_NAME);
		if (res != LFS_ERR_OK)
		{
			printf(ANSI_YELLOW"littlefs_init: can't create %s directory: err=%d\r\n"ANSI_CLEAR, DIR_NAME, res);
			assert(0);
			return;
		}
	}

	lfs_file_t fd;

	res = lfs_file_open(&lfs, &fd, FILE_NAME, LFS_O_WRONLY);
	if (res != LFS_ERR_OK)
	{
		uint8_t buf[] = TEST_STR_2;

		res = lfs_file_open(&lfs, &fd, FILE_NAME, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
		assert(res == LFS_ERR_OK);

		for (size_t cnt = 0; cnt < TEST_STR_CNT_2; cnt++)
		{
			res = lfs_file_write(&lfs, &fd, buf, (sizeof(TEST_STR_2) - 1));
			assert(res >= 0);
		}
	}
	lfs_file_close(&lfs, &fd);

	sprng();
	mimic_fat_init(&lfs_pico_flash_config);
	mimic_fat_create_cache();
}

//--------------------------------------------
static void reload(void)
{
	printf(ANSI_YELLOW"\r\n-----------------\r\nlittlefs_reload\r\n"ANSI_CLEAR);
	assert(lfs.cfg);
	lfs_unmount(&lfs);
	init();
}

//--------------------------------------------
static void check(void)
{
	int res;
	lfs_file_t fd;
	struct lfs_info finfo;
	bool eq;
	uint8_t *buf_fs;
	uint8_t *buf_pc;

	printf(ANSI_YELLOW"\r\n-----------------\r\nlittlefs_check\r\n"ANSI_CLEAR);
	assert(lfs.cfg);

	res = lfs_stat(&lfs, FILE_NAME, &finfo);
	if (res == LFS_ERR_OK)
	{
		eq = (finfo.size == FILE_SIZE_2) ? true : false;
		printf(ANSI_YELLOW"Size of %s is %d. Check passed = %s\r\n"ANSI_CLEAR, FILE_NAME, finfo.size, eq ? "true" : "false");
		if (eq)
		{
			buf_pc = malloc(FILE_SIZE_2);
			for (size_t cnt = 0; cnt < TEST_STR_CNT_2; cnt++)
			{
				memcpy(buf_pc + (sizeof(TEST_STR_2) - 1) * cnt, TEST_STR_2, (sizeof(TEST_STR_2) - 1));
			}
			res = lfs_file_open(&lfs, &fd, FILE_NAME, LFS_O_RDONLY);
			assert(res == LFS_ERR_OK);
			buf_fs = malloc(finfo.size);
			res = lfs_file_read(&lfs, &fd, buf_fs, finfo.size);
			eq = (memcmp(buf_pc, buf_fs, FILE_SIZE_2)) ? false : true;
			if (eq)
			{
				printf(ANSI_YELLOW"Content of %s is right. Check passed = true\r\n"ANSI_CLEAR, FILE_NAME);
			}
			else
			{
				printf(ANSI_YELLOW"Content of %s is wrong. Check passed = false\r\n"ANSI_CLEAR, FILE_NAME);
			}
		}
	}
	else
	{
		printf(ANSI_YELLOW"There is no %s in littlefs. Check passed = false\r\n"ANSI_CLEAR, FILE_NAME);
	}
}

//--------------------------------------------
static void cleanup(void)
{
	printf(ANSI_YELLOW"\r\n-----------------\r\nlittlefs_cleanup\r\n"ANSI_CLEAR);
	assert(lfs.cfg);
	lfs_unmount(&lfs);
}

//--------------------------------------------
TEST(test2w, "2w", "test2w.pcap", init, reload, check, cleanup);
TEST(test2u, "2u", "test2u.pcap", init, reload, check, cleanup);
