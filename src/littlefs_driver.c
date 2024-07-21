/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>
#include "lfs.h"
#include "mimic_fat.h"

//--------------------------------------------
#define FLASH_SECTOR_SIZE   4096
#define FLASH_SECTOR_COUNT  256
#define FS_SIZE             (FLASH_SECTOR_SIZE * FLASH_SECTOR_COUNT)

//--------------------------------------------
static uint8_t flash_memory[FS_SIZE];

//--------------------------------------------
static int read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t addr = (block * c->block_size) + off;
	memcpy(buffer, flash_memory + addr, size);
	return LFS_ERR_OK;
}

//--------------------------------------------
static int prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t addr = (block * c->block_size) + off;
	memcpy(flash_memory + addr, buffer, size);
	return LFS_ERR_OK;
}

//--------------------------------------------
static int erase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t addr = (block * c->block_size);
	memset(flash_memory + addr, 0xff, c->block_size);
	return LFS_ERR_OK;
}

//--------------------------------------------
static int sync(const struct lfs_config* c)
{
	return LFS_ERR_OK;
}

//--------------------------------------------
const struct lfs_config lfs_pico_flash_config =
{
	// block device operations
	.read = &read,
	.prog = &prog,
	.erase = &erase,
	.sync = &sync,

	// block device configuration
	.read_size = 1,
	.prog_size = 32,
	.block_size = FLASH_SECTOR_SIZE,
	.block_count = FLASH_SECTOR_COUNT,
	.cache_size = 1024,
	.lookahead_size = 16,
	.block_cycles = 500,
};
