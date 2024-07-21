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
#define FILE_NAME        "test/bbb.txt"
#define TEST_STR_1       "0123456789abcdef\r\n"
#define TEST_STR_CNT_1   353
#define FILE_SIZE_1      ((sizeof(TEST_STR_1) - 1) * TEST_STR_CNT_1)
#define TEST_STR_3       "fedcba9876543210\r\n"
#define TEST_STR_CNT_3   206
#define FILE_SIZE_3      ((sizeof(TEST_STR_3) - 1) * TEST_STR_CNT_3)



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
static void check1(void)
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
		eq = (finfo.size == FILE_SIZE_1) ? true : false;
		printf(ANSI_YELLOW"Size of %s is %d. Check passed = %s\r\n"ANSI_CLEAR, FILE_NAME, finfo.size, eq ? "true" : "false");
		if (eq)
		{
			buf_pc = malloc(FILE_SIZE_1);
			for (size_t cnt = 0; cnt < TEST_STR_CNT_1; cnt++)
			{
				memcpy(buf_pc + (sizeof(TEST_STR_1) - 1) * cnt, TEST_STR_1, (sizeof(TEST_STR_1) - 1));
			}
			res = lfs_file_open(&lfs, &fd, FILE_NAME, LFS_O_RDONLY);
			assert(res == LFS_ERR_OK);
			buf_fs = malloc(finfo.size);
			res = lfs_file_read(&lfs, &fd, buf_fs, finfo.size);
			eq = (memcmp(buf_pc, buf_fs, FILE_SIZE_1)) ? false : true;
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
static void check3(void)
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
		eq = (finfo.size == FILE_SIZE_3) ? true : false;
		printf(ANSI_YELLOW"Size of %s is %d. Check passed = %s\r\n"ANSI_CLEAR, FILE_NAME, finfo.size, eq ? "true" : "false");
		if (eq)
		{
			buf_pc = malloc(FILE_SIZE_3);
			for (size_t cnt = 0; cnt < TEST_STR_CNT_3; cnt++)
			{
				memcpy(buf_pc + (sizeof(TEST_STR_3) - 1) * cnt, TEST_STR_3, (sizeof(TEST_STR_3) - 1));
			}
			res = lfs_file_open(&lfs, &fd, FILE_NAME, LFS_O_RDONLY);
			assert(res == LFS_ERR_OK);
			buf_fs = malloc(finfo.size);
			res = lfs_file_read(&lfs, &fd, buf_fs, finfo.size);
			eq = (memcmp(buf_pc, buf_fs, FILE_SIZE_3)) ? false : true;
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
TEST(test1w, "1w", "test1w.pcap", init, reload, check1, cleanup);
TEST(test1u, "1u", "test1u.pcap", init, reload, check1, cleanup);
TEST(test3w, "3w", "test3w.pcap", init, reload, check3, cleanup);
TEST(test3u, "3u", "test3u.pcap", init, reload, check3, cleanup);
