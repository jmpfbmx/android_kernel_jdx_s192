/*
 * sp2529.c - sp2529 sensor driver
 *
 * Copyright (c) 2012, PortableView, All Rights Reserved.
 *
 * Contributors:
 *      Dong He <dhe@pvegps.com>
 *
 * Leverage hm2055.c
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/tegra-pm.h>
#include <media/sp2529.h>

#define SP2529_TABLE_END		0xFF
#define SP2529_MAX_RETRIES		3

struct sp2529_info {
	struct miscdevice miscdev_info;
	struct device *dev;
	struct i2c_client *client;
	struct sp2529_platform_data *pdata;
	struct clk *mclk;
	struct sp2529_power_rail power;
	int mode;
};

static struct sp2529_reg SP2529_setting_15fps_UXGA_1600_1200[] = {
	{0xfd,0x01,0,0},
	{0x36,0x00,0,0},
	{0xfd,0x00,0,0},
	{0xe7,0x03,0,0},
	{0xe7,0x00,0,0},
	{0xfd,0x00,0,0},
	{0x31,0x00,0,0},
	{0x33,0x00,0,0},
	{0x95,0x06,0,0},
	{0x94,0x40,0,0},
	{0x97,0x04,0,0},
	{0x96,0xb0,0,0},
	{0x98,0x3a,0,0},
	{0xfd,0x00,0,0},
	{0x0c,0x55,0,0},
	{0x27,0xa5,0,0},
	{0x1a,0x4b,0,0},
	{0x20,0x2f,0,0},
	{0x22,0x5a,0,0},
	{0x25,0xbd,0,0},
	{0x21,0x0d,0,0},
	{0x28,0x08,0,0},
	{0x1d,0x00,0,0},
	{0x7a,0x5d,0,0},
	{0x70,0x41,0,0},
	{0x74,0x40,0,0},
	{0x75,0x40,0,0},
	{0x15,0x3e,0,0},
	{0x71,0x3f,0,0},
	{0x7c,0x3f,0,0},
	{0x76,0x3f,0,0},
	{0x7e,0x29,0,0},
	{0x72,0x29,0,0},
	{0x77,0x28,0,0},
	{0x1e,0x01,0,0},
	{0x1c,0x0f,0,0},
	{0x2e,0xc0,0,0},
	{0x1f,0xc0,0,0},
	{0x6c,0x00,0,0},
	{0xfd,0x01,0,0},
	{0x32,0x00,0,0},
	{0xfd,0x02,0,0},
	{0x85,0x00,0,0},
	{0xfd,0x00,0,0},
	{0xfd,0x00,0,0},
	{0x2f,0x11,0,0},
	{0x03,0x03,0,0},
	{0x04,0x30,0,0},
	{0x05,0x00,0,0},
	{0x06,0x00,0,0},
	{0x07,0x00,0,0},
	{0x08,0x00,0,0},
	{0x09,0x00,0,0},
	{0x0a,0xb5,0,0},
	{0xfd,0x01,0,0},
	{0xf0,0x00,0,0},
	{0xf7,0x88,0,0},
	{0xf8,0x71,0,0},
	{0x02,0x0b,0,0},
	{0x03,0x01,0,0},
	{0x06,0x88,0,0},
	{0x07,0x00,0,0},
	{0x08,0x01,0,0},
	{0x09,0x00,0,0},
	{0xfd,0x02,0,0},
	{0x3d,0x0d,0,0},
	{0x3e,0x71,0,0},
	{0x3f,0x00,0,0},
	{0x88,0xc3,0,0},
	{0x89,0x87,0,0},
	{0x8a,0x43,0,0},
	{0xfd,0x02,0,0},
	{0xbe,0xd0,0,0},
	{0xbf,0x04,0,0},
	{0xd0,0xd0,0,0},
	{0xd1,0x04,0,0},
	{0xc9,0xd0,0,0},
	{0xca,0x04,0,0},
	{0xb8,0x70,0,0},
	{0xb9,0x80,0,0},
	{0xba,0x30,0,0},
	{0xbb,0x45,0,0},
	{0xbc,0x90,0,0},
	{0xbd,0x70,0,0},
	{0xfd,0x03,0,0},
	{0x77,0x48,0,0},
	{0xfd,0x01,0,0},
	{0xe0,0x48,0,0},
	{0xe1,0x38,0,0},
	{0xe2,0x30,0,0},
	{0xe3,0x2c,0,0},
	{0xe4,0x2c,0,0},
	{0xe5,0x2a,0,0},
	{0xe6,0x2a,0,0},
	{0xe7,0x28,0,0},
	{0xe8,0x28,0,0},
	{0xe9,0x28,0,0},
	{0xea,0x26,0,0},
	{0xf3,0x26,0,0},
	{0xf4,0x26,0,0},
	{0xfd,0x01,0,0},
	{0x04,0xc0,0,0},
	{0x05,0x26,0,0},
	{0x0a,0x48,0,0},
	{0x0b,0x26,0,0},
	{0xfd,0x01,0,0},
	{0xf2,0x09,0,0},
	{0xeb,0x78,0,0},
	{0xec,0x78,0,0},
	{0xed,0x06,0,0},
	{0xee,0x0a,0,0},
	{0xfd,0x02,0,0},
	{0x4f,0x46,0,0},
	{0xfd,0x03,0,0},
	{0x52,0xff,0,0},
	{0x53,0x60,0,0},
	{0x94,0x00,0,0},
	{0x54,0x00,0,0},
	{0x55,0x00,0,0},
	{0x56,0x80,0,0},
	{0x57,0x80,0,0},
	{0x95,0x00,0,0},
	{0x58,0x00,0,0},
	{0x59,0x00,0,0},
	{0x5a,0xf6,0,0},
	{0x5b,0x00,0,0},
	{0x5c,0x88,0,0},
	{0x5d,0x00,0,0},
	{0x96,0x00,0,0},
	{0xfd,0x03,0,0},
	{0x8a,0x00,0,0},
	{0x8b,0x00,0,0},
	{0x8c,0xff,0,0},
	{0x22,0xff,0,0},
	{0x23,0xff,0,0},
	{0x24,0xff,0,0},
	{0x25,0xff,0,0},
	{0x5e,0xff,0,0},
	{0x5f,0xff,0,0},
	{0x60,0xff,0,0},
	{0x61,0xff,0,0},
	{0x62,0x00,0,0},
	{0x63,0x00,0,0},
	{0x64,0x00,0,0},
	{0x65,0x00,0,0},
	{0xfd,0x01,0,0},
	{0x21,0x00,0,0},
	{0x22,0x00,0,0},
	{0x26,0x60,0,0},
	{0x27,0x14,0,0},
	{0x28,0x05,0,0},
	{0x29,0x00,0,0},
	{0x2a,0x01,0,0},
	{0xfd,0x01,0,0},
	{0xa1,0x1f,0,0},
	{0xa2,0x16,0,0},
	{0xa3,0x22,0,0},
	{0xa4,0x1e,0,0},
	{0xa5,0x1a,0,0},
	{0xa6,0x12,0,0},
	{0xa7,0x1c,0,0},
	{0xa8,0x1b,0,0},
	{0xa9,0x19,0,0},
	{0xaa,0x12,0,0},
	{0xab,0x1c,0,0},
	{0xac,0x16,0,0},
	{0xad,0x01,0,0},
	{0xae,0x03,0,0},
	{0xaf,0x01,0,0},
	{0xb0,0x03,0,0},
	{0xb1,0x03,0,0},
	{0xb2,0x03,0,0},
	{0xb3,0x03,0,0},
	{0xb4,0x03,0,0},
	{0xb5,0x05,0,0},
	{0xb6,0x03,0,0},
	{0xb7,0x05,0,0},
	{0xb8,0x05,0,0},
	{0xfd,0x02,0,0},
	{0x26,0xa0,0,0},
	{0x27,0x96,0,0},
	{0x28,0xcc,0,0},
	{0x29,0x01,0,0},
	{0x2a,0x00,0,0},
	{0x2b,0x00,0,0},
	{0x2c,0x20,0,0},
	{0x2d,0xdc,0,0},
	{0x2e,0x20,0,0},
	{0x2f,0x96,0,0},
	{0x1b,0x80,0,0},
	{0x1a,0x80,0,0},
	{0x18,0x16,0,0},
	{0x19,0x26,0,0},
	{0x1d,0x04,0,0},
	{0x1f,0x06,0,0},
	{0x66,0x1f,0,0},
	{0x67,0x42,0,0},
	{0x68,0xce,0,0},
	{0x69,0xee,0,0},
	{0x6a,0xa5,0,0},
	{0x7c,0x1e,0,0},
	{0x7d,0x43,0,0},
	{0x7e,0xee,0,0},
	{0x7f,0x14,0,0},
	{0x80,0xa6,0,0},
	{0x70,0x0a,0,0},
	{0x71,0x26,0,0},
	{0x72,0x14,0,0},
	{0x73,0x39,0,0},
	{0x74,0xaa,0,0},
	{0x6b,0xf0,0,0},
	{0x6c,0x0b,0,0},
	{0x6d,0x1d,0,0},
	{0x6e,0x43,0,0},
	{0x6f,0x6a,0,0},
	{0x61,0xcc,0,0},
	{0x62,0xf0,0,0},
	{0x63,0x36,0,0},
	{0x64,0x56,0,0},
	{0x65,0x5a,0,0},
	{0x75,0x00,0,0},
	{0x76,0x09,0,0},
	{0x77,0x02,0,0},
	{0x0e,0x16,0,0},
	{0x3b,0x09,0,0},
	{0xfd,0x02,0,0},
	{0x02,0x00,0,0},
	{0x03,0x10,0,0},
	{0x04,0xf0,0,0},
	{0xf5,0xb3,0,0},
	{0xf6,0x80,0,0},
	{0xf7,0xe0,0,0},
	{0xf8,0x89,0,0},
	{0xfd,0x02,0,0},
	{0x08,0x00,0,0},
	{0x09,0x04,0,0},
	{0xfd,0x02,0,0},
	{0xdd,0x0f,0,0},
	{0xde,0x0f,0,0},
	{0xfd,0x02,0,0},
	{0x57,0x30,0,0},
	{0x58,0x10,0,0},
	{0x59,0xe0,0,0},
	{0x5a,0x00,0,0},
	{0x5b,0x12,0,0},
	{0xcb,0x08,0,0},
	{0xcc,0x0b,0,0},
	{0xcd,0x10,0,0},
	{0xce,0x1a,0,0},
	{0xfd,0x03,0,0},
	{0x87,0x04,0,0},
	{0x88,0x08,0,0},
	{0x89,0x10,0,0},
	{0xfd,0x02,0,0},
	{0xe8,0x58,0,0},
	{0xec,0x68,0,0},
	{0xe9,0x60,0,0},
	{0xed,0x68,0,0},
	{0xea,0x58,0,0},
	{0xee,0x60,0,0},
	{0xeb,0x48,0,0},
	{0xef,0x40,0,0},
	{0xfd,0x02,0,0},
	{0xdc,0x04,0,0},
	{0x05,0x6f,0,0},
	{0xfd,0x02,0,0},
	{0xf4,0x30,0,0},
	{0xfd,0x03,0,0},
	{0x97,0x98,0,0},
	{0x98,0x88,0,0},
	{0x99,0x88,0,0},
	{0x9a,0x80,0,0},
	{0xfd,0x02,0,0},
	{0xe4,0xff,0,0},
	{0xe5,0xff,0,0},
	{0xe6,0xff,0,0},
	{0xe7,0xff,0,0},
	{0xfd,0x03,0,0},
	{0x72,0x18,0,0},
	{0x73,0x28,0,0},
	{0x74,0x28,0,0},
	{0x75,0x30,0,0},
	{0xfd,0x02,0,0},
	{0x78,0x20,0,0},
	{0x79,0x20,0,0},
	{0x7a,0x14,0,0},
	{0x7b,0x08,0,0},
	{0x81,0x02,0,0},
	{0x82,0x20,0,0},
	{0x83,0x20,0,0},
	{0x84,0x08,0,0},
	{0xfd,0x03,0,0},
	{0x7e,0x06,0,0},
	{0x7f,0x0d,0,0},
	{0x80,0x10,0,0},
	{0x81,0x16,0,0},
	{0x7c,0xff,0,0},
	{0x82,0x54,0,0},
	{0x83,0x43,0,0},
	{0x84,0x00,0,0},
	{0x85,0x20,0,0},
	{0x86,0x40,0,0},
	{0xfd,0x03,0,0},
	{0x66,0x18,0,0},
	{0x67,0x28,0,0},
	{0x68,0x20,0,0},
	{0x69,0x88,0,0},
	{0x9b,0x18,0,0},
	{0x9c,0x28,0,0},
	{0x9d,0x20,0,0},
	{0xfd,0x01,0,0},
	{0x8b,0x00,0,0},
	{0x8c,0x0f,0,0},
	{0x8d,0x21,0,0},
	{0x8e,0x2c,0,0},
	{0x8f,0x37,0,0},
	{0x90,0x46,0,0},
	{0x91,0x53,0,0},
	{0x92,0x5e,0,0},
	{0x93,0x6a,0,0},
	{0x94,0x7d,0,0},
	{0x95,0x8d,0,0},
	{0x96,0x9e,0,0},
	{0x97,0xac,0,0},
	{0x98,0xba,0,0},
	{0x99,0xc6,0,0},
	{0x9a,0xd1,0,0},
	{0x9b,0xda,0,0},
	{0x9c,0xe4,0,0},
	{0x9d,0xeb,0,0},
	{0x9e,0xf2,0,0},
	{0x9f,0xf9,0,0},
	{0xa0,0xff,0,0},
	{0xfd,0x02,0,0},
	{0x15,0xa9,0,0},
	{0x16,0x84,0,0},
	{0xa0,0x97,0,0},
	{0xa1,0xea,0,0},
	{0xa2,0xff,0,0},
	{0xa3,0x0e,0,0},
	{0xa4,0x77,0,0},
	{0xa5,0xfa,0,0},
	{0xa6,0x08,0,0},
	{0xa7,0xcb,0,0},
	{0xa8,0xad,0,0},
	{0xa9,0x3c,0,0},
	{0xaa,0x30,0,0},
	{0xab,0x0c,0,0},
	{0xac,0x7f,0,0},
	{0xad,0x08,0,0},
	{0xae,0xf8,0,0},
	{0xaf,0xff,0,0},
	{0xb0,0x6e,0,0},
	{0xb1,0x13,0,0},
	{0xb2,0xd2,0,0},
	{0xb3,0x6e,0,0},
	{0xb4,0x40,0,0},
	{0xb5,0x30,0,0},
	{0xb6,0x03,0,0},
	{0xb7,0x1f,0,0},
	{0xfd,0x01,0,0},
	{0xd2,0x2d,0,0},
	{0xd1,0x38,0,0},
	{0xdd,0x3f,0,0},
	{0xde,0x37,0,0},
	{0xfd,0x02,0,0},
	{0xc1,0x40,0,0},
	{0xc2,0x40,0,0},
	{0xc3,0x40,0,0},
	{0xc4,0x40,0,0},
	{0xc5,0x80,0,0},
	{0xc6,0x60,0,0},
	{0xc7,0x00,0,0},
	{0xc8,0x00,0,0},
	{0xfd,0x01,0,0},
	{0xd3,0xa0,0,0},
	{0xd4,0xa0,0,0},
	{0xd5,0xa0,0,0},
	{0xd6,0xa0,0,0},
	{0xd7,0xa0,0,0},
	{0xd8,0xa0,0,0},
	{0xd9,0xa0,0,0},
	{0xda,0xa0,0,0},
	{0xfd,0x03,0,0},
	{0x76,0x0a,0,0},
	{0x7a,0x40,0,0},
	{0x7b,0x40,0,0},
	{0xfd,0x01,0,0},
	{0xc2,0xaa,0,0},
	{0xc3,0xaa,0,0},
	{0xc4,0x66,0,0},
	{0xc5,0x66,0,0},
	{0xfd,0x01,0,0},
	{0xcd,0x08,0,0},
	{0xce,0x18,0,0},
	{0xfd,0x02,0,0},
	{0x32,0x60,0,0},
	{0x35,0x60,0,0},
	{0x37,0x13,0,0},
	{0xfd,0x01,0,0},
	{0xdb,0x00,0,0},
	{0x10,0x88,0,0},
	{0x11,0x88,0,0},
	{0x12,0x90,0,0},
	{0x13,0x90,0,0},
	{0x14,0x9a,0,0},
	{0x15,0x9a,0,0},
	{0x16,0x8b,0,0},
	{0x17,0x88,0,0},
	{0xfd,0x03,0,0},
	{0x00,0x80,0,0},
	{0x03,0x68,0,0},
	{0x06,0xd8,0,0},
	{0x07,0x28,0,0},
	{0x0a,0xfd,0,0},
	{0x01,0x16,0,0},
	{0x02,0x16,0,0},
	{0x04,0x16,0,0},
	{0x05,0x16,0,0},
	{0x0b,0x40,0,0},
	{0x0c,0x40,0,0},
	{0x0d,0x40,0,0},
	{0x0e,0x40,0,0},
	{0x08,0x0c,0,0},
	{0x09,0x0c,0,0},
	{0xfd,0x02,0,0},
	{0x8e,0x0a,0,0},
	{0x90,0x40,0,0},
	{0x91,0x40,0,0},
	{0x92,0x60,0,0},
	{0x93,0x80,0,0},
	{0x9e,0x44,0,0},
	{0x9f,0x44,0,0},
	{0xfd,0x02,0,0},
	{0x85,0x00,0,0},
	{0xfd,0x01,0,0},
	{0x00,0x00,0,0},
	{0xfb,0x25,0,0},
	{0x32,0x15,0,0},
	{0x33,0xef,0,0},
	{0x34,0xef,0,0},
	{0x35,0x40,0,0},
	{0xfd,0x00,0,0},
	{0x3f,0x03,0,0},
	{0xfd,0x01,0,0},
	{0x50,0x00,0,0},
	{0x66,0x00,0,0},
	{0xfd,0x02,0,0},
	{0xd6,0x0f,0,0},
};

static struct sp2529_reg *mode_table[] = {
	[SP2529_MODE_1600x1200] = SP2529_setting_15fps_UXGA_1600_1200,
};

static struct sp2529_reg ColorEffect_None[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg ColorEffect_Mono[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg ColorEffect_Sepia[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg ColorEffect_Negative[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg ColorEffect_Solarize[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg ColorEffect_Posterize[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Whitebalance_Auto[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Whitebalance_Incandescent[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Whitebalance_Daylight[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Whitebalance_Fluorescent[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Whitebalance_CloudyDaylight[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Exposure_0[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Exposure_1[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Exposure_2[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Exposure_m1[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static struct sp2529_reg Exposure_m2[] = {
	{SP2529_TABLE_END, 0xFF, 0, 0}
};

static int sp2529_read_reg(struct i2c_client *client, u16 addr, u8 * val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = data;

	data[0] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 1;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err != 2)
		return -EINVAL;

	*val = data[1];
	return 0;
}

static int sp2529_write_reg(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8)(addr & 0xff);
	data[1] = (u8)(val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = data;

	do
	{
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("sp2529: i2c transfer failed, retrying %x %x\n", addr, val);
		msleep(3);
	}while (retry <= SP2529_MAX_RETRIES);

	return err;
}

static int sp2529_power_put(struct sp2529_power_rail *pw)
{
	if (unlikely(!pw))
		return -EFAULT;

	if (likely(pw->avdd))
		regulator_put(pw->avdd);

	if (likely(pw->dovdd))
		regulator_put(pw->dovdd);

	pw->avdd = NULL;
	pw->dovdd = NULL;

	return 0;
}

static int sp2529_regulator_get(struct sp2529_info *info, struct regulator **vreg, char vreg_name[])
{
	struct regulator *reg = NULL;
	int err = 0;

	reg = regulator_get(&info->client->dev, vreg_name);
	if (unlikely(IS_ERR(reg)))
	{
		dev_err(&info->client->dev, "%s %s ERR: %d\n", __func__, vreg_name, (int)reg);
		err = PTR_ERR(reg);
		reg = NULL;
	}
	else
		dev_err(&info->client->dev, "%s: %s\n", __func__, vreg_name);

	*vreg = reg;
	return err;
}

static int sp2529_power_get(struct sp2529_info *info)
{
	struct sp2529_power_rail *pw = &info->power;
	int err = 0;

	err |= sp2529_regulator_get(info, &pw->avdd, "avdd_af1_cam"); /* ananlog 2.7v */
	err |= sp2529_regulator_get(info, &pw->dovdd, "vdd_lcd_1v2_s"); /* IO 1.8v */
	err |= sp2529_regulator_get(info, &pw->dvdd, "vdd_cam_1v1_cam"); /* IO 1.8v */

	return err;
}

static void sp2529_mclk_disable(struct sp2529_info *info)
{
	dev_err(&info->client->dev, "%s: disable MCLK\n", __func__);
	clk_disable_unprepare(info->mclk);
}

static int sp2529_mclk_enable(struct sp2529_info *info)
{
	int err;
	unsigned long mclk_init_rate = 24000000;

	dev_err(&info->client->dev, "%s: enable MCLK with %lu Hz\n", __func__, mclk_init_rate);

	err = clk_set_rate(info->mclk, mclk_init_rate);
	if (!err)
		err = clk_prepare_enable(info->mclk);
	return err;
}

static int sp2529_set_power(struct sp2529_info *info, u32 level)
{
	int err = 0;

	switch(level)
	{
		case SP2529_POWER_LEVEL_ON:
			if (info->pdata->power_on)
			{
				err = sp2529_mclk_enable(info);
				if (!err)
					err = info->pdata->power_on(&info->power);
				if (err < 0)
					sp2529_mclk_disable(info);
			}
			break;
		case SP2529_POWER_LEVEL_OFF:
			if (info->pdata->power_off)
			{
				info->pdata->power_off(&info->power);
				sp2529_mclk_disable(info);
			}
			break;
		default:
			dev_err(info->dev, "unknown power level %d.\n", level);
			return -EINVAL;
	}

	return err;
}

static int sp2529_write_table(struct i2c_client *client, const struct sp2529_reg table[],
		const struct sp2529_reg override_list[], int num_override_regs)
{
	int err;
	const struct sp2529_reg *next;
	int i;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u8 Mask = 0;
	register u8 Val = 0;

	u8 temp = 0;
	u8 RegVal = 0;
	int retval = 0;

	for (next = table; next->u16RegAddr != SP2529_TABLE_END; next++) {
		Delay_ms = next->u32Delay_ms;
		RegAddr = next->u16RegAddr;
		Val = next->u8Val;
		Mask = next->u8Mask;

		if (override_list)
		{
			for (i = 0; i < num_override_regs; i++)
			{
				if (RegAddr == override_list[i].u16RegAddr)
				{
					Val = override_list[i].u8Val;
					break;
				}
			}
		}

		if (Mask)
		{
			retval = sp2529_read_reg(client, RegAddr, &RegVal);
			if (retval < 0)
				goto err;

			RegVal &= ~(u8)Mask;
			Val &= Mask;
			Val |= RegVal;
		}

		err = sp2529_write_reg(client, RegAddr, Val);
		temp = 0;

		if (err)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
	msleep(500);
err:
	return err;
}

static int sp2529_set_mode(struct sp2529_info *info, struct sp2529_mode *mode)
{
	int sensor_table;
	int err;

	pr_info("%s: xres %u yres %u \n", __func__, mode->xres, mode->yres);

	if (1)
		sensor_table = SP2529_MODE_1600x1200;
	else
	{
		pr_err("%s: invalid resolution supplied to set mode %d %d\n", __func__, mode->xres, mode->yres);
		return -EINVAL;
	}

	err = sp2529_write_table(info->client, mode_table[sensor_table], NULL, 0);
	if (err)
	{
		printk("sp2529_write_table() failed!!!\n");
		return err;
	}

	info->mode = sensor_table;
	return 0;
}

static long sp2529_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sp2529_info *info = file->private_data;
	
	switch (_IOC_NR(cmd))
	{
		case _IOC_NR(SP2529_IOCTL_POWER_LEVEL):
			dev_err(&info->client->dev, "SP2529_IOCTL_POWER_LEVEL\n");
			return sp2529_set_power(info, (u32)arg);
			break;
		case _IOC_NR(SP2529_IOCTL_SET_SENSOR_MODE):
			{
				struct sp2529_mode mode;
				dev_err(&info->client->dev, "SP2529_IOCTL_SET_SENSOR_MODE\n");
				if (copy_from_user(&mode, (const void __user *)arg, sizeof(struct sp2529_mode)))
					return -EFAULT;

				return sp2529_set_mode(info, &mode);
			}
		case _IOC_NR(SP2529_IOCTL_GET_SENSOR_STATUS):
			dev_err(&info->client->dev, "SP2529_IOCTL_GET_SENSOR_STATUS\n");
			return 0;
		case _IOC_NR(SP2529_IOCTL_GET_AF_STATUS):
			dev_err(&info->client->dev, "SP2529_IOCTL_GET_AF_STATUS\n");
			return 0;
		case _IOC_NR(SP2529_IOCTL_SET_AF_MODE):
			dev_err(&info->client->dev, "SP2529_IOCTL_SET_AF_MODE\n");
			return 0;
		case _IOC_NR(SP2529_IOCTL_SET_COLOR_EFFECT):
			{
				u8 coloreffect;
				int ret = 0;
				if (copy_from_user(&coloreffect, (const void __user *)arg, sizeof(coloreffect)))
					return -EFAULT;

				switch (coloreffect)
				{
					case YUV_ColorEffect_None:
						ret = sp2529_write_table(info->client, ColorEffect_None, NULL, 0);
						break;
					case YUV_ColorEffect_Mono:
						ret = sp2529_write_table(info->client, ColorEffect_Mono, NULL, 0);
						break;
					case YUV_ColorEffect_Sepia:
						ret = sp2529_write_table(info->client, ColorEffect_Sepia, NULL, 0);
						break;
					case YUV_ColorEffect_Negative:
						ret = sp2529_write_table(info->client, ColorEffect_Negative, NULL, 0);
						break;
					case YUV_ColorEffect_Solarize:
						ret = sp2529_write_table(info->client, ColorEffect_Solarize, NULL, 0);
						break;
					case YUV_ColorEffect_Posterize:
						ret = sp2529_write_table(info->client, ColorEffect_Posterize, NULL, 0);
						break;
					default:
						break;
				}
				return 0;
			}
		case _IOC_NR(SP2529_IOCTL_SET_WHITE_BALANCE):
			{
				u8 whitebalance;
				int ret = 0;
				if (copy_from_user(&whitebalance, (const void __user *)arg, sizeof(whitebalance)))
					return -EFAULT;

				switch (whitebalance)
				{
					case YUV_Whitebalance_Auto:
						ret = sp2529_write_table(info->client, Whitebalance_Auto, NULL, 0);
						break;
					case YUV_Whitebalance_Incandescent:
						ret = sp2529_write_table(info->client, Whitebalance_Incandescent, NULL, 0);
						break;
					case YUV_Whitebalance_Daylight:
						ret = sp2529_write_table(info->client, Whitebalance_Daylight, NULL, 0);
						break;
					case YUV_Whitebalance_Fluorescent:
						ret = sp2529_write_table(info->client, Whitebalance_Fluorescent, NULL, 0);
						break;
					case YUV_Whitebalance_CloudyDaylight:
						ret = sp2529_write_table(info->client, Whitebalance_CloudyDaylight, NULL, 0);
						break;
					default:
						break;
				}
				return ret;
			}
		case _IOC_NR(SP2529_IOCTL_SET_EXPOSURE):
			{
				u8 expo;
				int ret = 0;
				if (copy_from_user(&expo, (const void __user *)arg, sizeof(expo)))
					return -EFAULT;

				switch (expo)
				{
					case YUV_EXPOSURE_0:
						ret = sp2529_write_table(info->client, Exposure_0, NULL, 0);
						break;
					case YUV_EXPOSURE_1:
						ret = sp2529_write_table(info->client, Exposure_1, NULL, 0);
						break;
					case YUV_EXPOSURE_2:
						ret = sp2529_write_table(info->client, Exposure_2, NULL, 0);
						break;
					case YUV_EXPOSURE_m1:
						ret = sp2529_write_table(info->client, Exposure_m1, NULL, 0);
						break;
					case YUV_EXPOSURE_m2:
						ret = sp2529_write_table(info->client, Exposure_m2, NULL, 0);
						break;
					default:
						break;
				}
				return ret;
			}
		case _IOC_NR(SP2529_IOCTL_SET_SCENE_MODE):
			return 0;
		default:
			dev_err(&info->client->dev, "%s:unknown cmd: %d\n", __func__, cmd);
			return -EINVAL;
	}
	
	return 0;
}

static int sp2529_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct sp2529_info *info;

	pr_info("%s\n", __func__);
	if (!miscdev)
	{
		pr_err("miscdev == NULL\n");
		return -1;
	}
	
	info = container_of(miscdev, struct sp2529_info, miscdev_info);
	file->private_data = info;

	return 0;
}

int sp2529_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	pr_info("%s\n", __func__);
	return 0;
}

static const struct file_operations sp2529_fileops = {
	.owner = THIS_MODULE,
	.open = sp2529_open,
	.unlocked_ioctl = sp2529_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sp2529_ioctl,
#endif
	.release = sp2529_release,
};

static struct miscdevice sp2529_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sp2529",
	.fops = &sp2529_fileops,
};

static int sp2529_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sp2529_info *info;
	int err = 0;
	const char *mclk_name;

	printk("sp2529: probing sensor.\n");

	info = devm_kzalloc(&client->dev, sizeof(struct sp2529_info), GFP_KERNEL);
	if (!info)
	{
		pr_err("sp2529: Unable to allocate memory!\n");
		return -ENOMEM;
	}

	memcpy(&info->miscdev_info, &sp2529_device, sizeof(struct miscdevice));

	err = misc_register(&info->miscdev_info);
	if (err)
	{
		pr_err("sp2529: Unable to register misc device!\n");
		goto fail_misc_register;
	}

	info->dev = &client->dev;
	info->pdata = client->dev.platform_data;
	info->client = client;

	i2c_set_clientdata(client, info);

	mclk_name = info->pdata->mclk_name ? info->pdata->mclk_name : "default_mclk";
	info->mclk = devm_clk_get(&client->dev, mclk_name);
	if (IS_ERR(info->mclk))
	{
		dev_err(&client->dev, "%s: unable to get clock %s\n", __func__, mclk_name);
		err = PTR_ERR(info->mclk);
		goto fail_devm_clk_get;
	}

	if (sp2529_power_get(info))
		goto fail_sp2529_power_get;

	return 0;

fail_sp2529_power_get:
fail_devm_clk_get:
	misc_deregister(&info->miscdev_info);
fail_misc_register:
	devm_kfree(&client->dev, info);
	return err;
}

static int sp2529_remove(struct i2c_client *client)
{
	struct sp2529_info *info;
	info = i2c_get_clientdata(client);

	sp2529_power_put(&info->power);
	misc_deregister(&info->miscdev_info);
	devm_kfree(&client->dev, info);
	return 0;
}

static const struct i2c_device_id sp2529_id[] = {
	{"sp2529", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, sp2529_id);

static struct i2c_driver sp2529_i2c_driver = {
	.driver = {
		.name = "sp2529",
		.owner = THIS_MODULE,
	},
	.probe = sp2529_probe,
	.remove = sp2529_remove,
	.id_table = sp2529_id,
};

static int __init sp2529_init(void)
{
	pr_info("sp2529 sensor driver loading\n");
	return i2c_add_driver(&sp2529_i2c_driver);
}

static void __exit sp2529_exit(void)
{
	i2c_del_driver(&sp2529_i2c_driver);
}

module_init(sp2529_init);
module_exit(sp2529_exit);
