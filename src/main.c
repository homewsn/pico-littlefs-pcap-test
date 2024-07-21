/*
 * Copyright (c) 2024, Vladimir Alemasov
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>     /* uint8_t ... uint64_t */
#include <stdlib.h>     /* exit */
#include <stdio.h>      /* printf */
#include <string.h>     /* memcpy */
#include <time.h>       /* time */
#include <stdbool.h>    /* bool */
#include <errno.h>		/* errno */
#include <assert.h>     /* assert */
#include <pcap/pcap.h>  /* pcap library stuff */
#ifdef _WIN32
#include "win/getopt.h"
#else
#include <stddef.h>     /* offsetof */
#include <signal.h>     /* signal */
#include <sys/time.h>   /* gettimeofday */
#include <fcntl.h>      /* open */
#include <unistd.h>     /* usleep, getopt, write, close */
extern char *optarg;
#ifndef HANDLE
#define HANDLE int
#endif
#endif
#include "lfs.h"
#include "mimic_fat.h"
#include "tests.h"


//--------------------------------------------
typedef struct options
{
	int opt_r;
	int opt_c;
	char *opt_t_arg;
} options_t;
static options_t ts;
static test_t *test;

//--------------------------------------------
static int dlt;
static pcap_t *pd;
static char err_buf[PCAP_ERRBUF_SIZE];

//--------------------------------------------
#define LINKTYPE_USBPCAP                249
#define LINKTYPE_USB_LINUX_MMAPPED      220
#define USB_MSC_BOT_CBW_SIGNATURE       0x43425355
#define SCSI_READ_10                    0x28
#define SCSI_WRITE_10                   0x2A

//--------------------------------------------
#pragma pack(push, 1)

typedef struct
{
	uint16_t headerLen;        /* This header length */
	uint64_t irpId;            /* I/O Request packet ID */
	uint32_t status;           /* USB status code (on return from host controller) */
	uint16_t function;         /* URB Function */
	uint8_t info;              /* I/O Request info */
	uint16_t bus;              /* bus (RootHub) number */
	uint16_t device;           /* device address */
	uint8_t endpoint;          /* endpoint number and transfer direction */
	uint8_t transfer;          /* transfer type */
	uint32_t dataLength;       /* Data length */
} usbpcap_packet_header_t;

typedef struct
{
	uint64_t id;               /* 0: URB ID - from submission to callback */
	uint8_t type;              /* 8: Same as text; extensible. */
	uint8_t xfer_type;         /* 9: ISO (0), Intr, Control, Bulk (3) */
	uint8_t epnum;             /* 10: Endpoint number and transfer direction */
	uint8_t devnum;            /* 11: Device address */
	uint16_t busnum;           /* 12: Bus number */
	uint8_t flag_setup;        /* 14: Same as text */
	uint8_t flag_data;         /* 15: Same as text; Binary zero is OK. */
	int64_t ts_sec;            /* 16: gettimeofday */
	int32_t ts_usec;           /* 24: gettimeofday */
	int32_t status;            /* 28: */
	uint32_t length;           /* 32: Length of data (submitted or actual) */
	uint32_t len_cap;          /* 36: Delivered length */
	union {                    /* 40: */
		uint8_t setup[8];      /* Only for Control S-type */
		struct iso_rec {       /* Only for ISO */
			uint32_t error_count;
			uint32_t numdesc;
		} iso;
	} s;
	int32_t interval;          /* 48: Only for Interrupt and ISO */
	int32_t start_frame;       /* 52: For ISO */
	uint32_t xfer_flags;       /* 56: copy of URB's transfer_flags */
	uint32_t ndesc;            /* 60: Actual number of ISO descriptors */
} usbmon_packet_header_t;

// Command Block Wrapper
typedef struct
{
	uint32_t dSignature;
	uint32_t dTag;
	uint32_t dDataTransferLength;
	uint8_t bmFlags;
	uint8_t bLUN;
	uint8_t bCBLength;
	uint8_t CB[16];
} usb_msc_bot_cbw_t;

// READ(10), WRITE(10) command
typedef struct
{
	uint8_t operation_code;
	uint8_t bit_fields;
	uint8_t logical_block_address[4];
	uint8_t group_number;
	uint8_t transfer_length[2];
	uint8_t control;
} cdb_rw_10_t;

#pragma pack(pop)

//--------------------------------------------
static uint8_t *data_buffer;
static bool fs_reboot;
static bool data_comparison;

//--------------------------------------------
void pcap_callback(u_char *ptr, const struct pcap_pkthdr *header, const u_char *packet)
{
	static size_t packet_num;
	static uint16_t dev_addr;
	static bool read_data;
	static bool write_data;
	static uint32_t lba;
	static uint16_t lbn;
	uint32_t data_length;
	size_t header_length;
	uint16_t actual_dev_addr;

	packet_num++;

	switch (dlt)
	{
		case LINKTYPE_USBPCAP:
		{
			usbpcap_packet_header_t *usbpcap_packet_header = (usbpcap_packet_header_t *)packet;
			data_length = usbpcap_packet_header->dataLength;
			actual_dev_addr = usbpcap_packet_header->device;
			header_length = sizeof(usbpcap_packet_header_t);
			break;
		}
		case LINKTYPE_USB_LINUX_MMAPPED:
		{
			usbmon_packet_header_t *usbmon_packet_header = (usbmon_packet_header_t *)packet;
			data_length = usbmon_packet_header->len_cap;
			actual_dev_addr = usbmon_packet_header->devnum;
			header_length = sizeof(usbmon_packet_header_t);
			break;
		}
	}

	if (data_length == sizeof(usb_msc_bot_cbw_t))
	{
		usb_msc_bot_cbw_t *usb_msc_bot_cbw = (usb_msc_bot_cbw_t *)(packet + header_length);
		if (usb_msc_bot_cbw->dSignature == USB_MSC_BOT_CBW_SIGNATURE && usb_msc_bot_cbw->bCBLength == sizeof(cdb_rw_10_t))
		{
			if (dev_addr != actual_dev_addr)
			{
				dev_addr = actual_dev_addr;
				if (ts.opt_r)
				{
					printf(ANSI_YELLOW"\r\nReload littlefs and mimic_fat (It's equivalent to rebooting the MCU).\r\n"ANSI_CLEAR);
					test->littlefs_reload();
				}
				else
				{
					printf(ANSI_YELLOW"\r\nUSB cable inserted (or pulled out and reinserted).\r\n"ANSI_CLEAR);
				}
			}
			cdb_rw_10_t *cdb_rw_10 = (cdb_rw_10_t *)(packet + header_length + sizeof(usb_msc_bot_cbw_t) - sizeof(((usb_msc_bot_cbw_t *)0)->CB));
			if (cdb_rw_10->operation_code == SCSI_READ_10 || cdb_rw_10->operation_code == SCSI_WRITE_10)
			{
				lba =
					cdb_rw_10->logical_block_address[0] << 24 |
					cdb_rw_10->logical_block_address[1] << 16 |
					cdb_rw_10->logical_block_address[2] << 8 |
					cdb_rw_10->logical_block_address[3];
				lbn =
					cdb_rw_10->transfer_length[0] << 8 |
					cdb_rw_10->transfer_length[1];
				if (cdb_rw_10->operation_code == SCSI_READ_10)
				{
					assert(!data_buffer);
					data_buffer = (uint8_t *)malloc(lbn * 512);
					assert(data_buffer);
					read_data = true;
					printf(ANSI_YELLOW"\r\nPacket No %ld, read %d sectors from %d\r\n"ANSI_CLEAR, packet_num, lbn, lba);
					for (size_t cnt = 0; cnt < lbn; cnt++)
					{
						mimic_fat_read(0, lba + cnt, data_buffer + 512 * cnt, 512);
					}
				}
				if (cdb_rw_10->operation_code == SCSI_WRITE_10)
				{
					write_data = true;
				}
			}
		}
	}
	if (data_length >= 512)
	{
		assert(read_data || write_data);
		if (read_data)
		{
			uint8_t *read_buffer = (uint8_t *)packet + header_length;
			if (ts.opt_c)
			{
#if 0
				int res = memcmp(data_buffer, read_buffer, usbpcap_packet_header->dataLength);
				assert(!res);
#else
				for (size_t cnt = 0; cnt < data_length; cnt++)
				{
					if (data_buffer[cnt] != read_buffer[cnt])
					{
						int32_t num = (int32_t)cnt;
						uint32_t sec = lba;
						while (num >= 0)
						{
							num -= 512;
							sec++;
						}
						num += 512;
						sec--;
						printf(ANSI_YELLOW"Data is not equal, sector %d, byte %d, actual data = 0x%02x, read data = 0x%02x\r\n"ANSI_CLEAR, sec, num, data_buffer[cnt], read_buffer[cnt]);
					}
				}
#endif
			}
			assert(data_buffer);
			free(data_buffer);
			data_buffer = 0;
			read_data = false;
		}
		if (write_data)
		{
			printf(ANSI_YELLOW"\r\nPacket No %ld, write %d sectors from %d\r\n"ANSI_CLEAR, packet_num, lbn, lba);
			for (size_t cnt = 0; cnt < data_length / 512; cnt++)
			{
				mimic_fat_write(0, lba + cnt, (uint8_t *)packet + header_length + 512 * cnt, 512);
			}
			write_data = false;
		}
	}
}

//--------------------------------------------
static int pcap_lib_init(const char *name)
{
	assert(name);

#ifdef _WIN32
	SetDllDirectory("C:\\Windows\\System32\\Npcap\\");
	if (LoadLibrary("wpcap.dll") == NULL)
	{
		printf(ANSI_YELLOW"FATAL ERROR: Could not find Npcap runtime libraries installed.\n"ANSI_CLEAR);
		return -1;
	}
#endif
	if ((pd = pcap_open_offline(name, err_buf)) == NULL)
	{
		printf(ANSI_YELLOW"FATAL ERROR: %s\n"ANSI_CLEAR, err_buf);
		return -1;
	}
	else
	{
		printf(ANSI_YELLOW"%s open\n"ANSI_CLEAR, name);
	}

	dlt = pcap_datalink(pd);
	switch (dlt)
	{
	case LINKTYPE_USB_LINUX_MMAPPED:
	case LINKTYPE_USBPCAP:
		break;
	default:
		printf(ANSI_YELLOW"FATAL ERROR: Link-layer header type %d in %s is not supported\n"ANSI_CLEAR, dlt, name);
		return -1;
	}
	return 0;
}

//--------------------------------------------
static void print_usage(void)
{
	printf("Usage:\n");
	printf("  pico-littlefs-pcap-test -t <test_id>\n");
	printf("Mandatory arguments for input:\n");
	printf("  -t <test_id>          Test Id\n");
	printf("Optional arguments for input:\n");
	printf("  -c                    Compare actual and PCAP data\n");
#if 0
	printf("  -r                    Reload FS every time the USB device number changes\n");
#endif
}

//--------------------------------------------
int main(int argc, char *argv[])
{
	int option;

	while ((option = getopt(argc, argv, "t:rc")) != -1)
	{
		switch (option)
		{
		case 't':
			ts.opt_t_arg = optarg;
			break;
#if 0
		// Doesn't work as expected
		case 'r':
			ts.opt_r = 1;
			break;
#endif
		case 'c':
			ts.opt_c = 1;
			break;
		default: // '?'
			print_usage();
			exit(EXIT_FAILURE);
		}
	}
	if (!ts.opt_t_arg)
	{
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (!(test = (test_t *)get_test(ts.opt_t_arg)))
	{
		printf("This -t option argument is not supported.\n\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	test->littlefs_init();

	if (pcap_lib_init(test->file) < 0)
	{
		exit(EXIT_FAILURE);
	}

	pcap_loop(pd, 0, pcap_callback, NULL);
	pcap_close(pd);

	test->littlefs_check();
	test->littlefs_cleanup();

	if (data_buffer)
	{
		free(data_buffer);
	}

	exit(EXIT_SUCCESS);
}
