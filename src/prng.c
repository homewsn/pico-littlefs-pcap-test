/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>     /* uint8_t ... uint64_t */

//--------------------------------------------
#define PRNG_MAX 0x7fff

//--------------------------------------------
static uint32_t seed;

//--------------------------------------------
void sprng(void)
{
	seed = 0;
}

//--------------------------------------------
uint16_t prng(void)
{

	seed++;
	if (seed > PRNG_MAX)
	{
		seed = 0;
	}

	return seed;
}
