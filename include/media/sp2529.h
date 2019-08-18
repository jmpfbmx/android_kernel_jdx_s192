/*
 * ov5640.h - header for YUV camera sensor OV5640 driver.
 *
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#ifndef __SP2529_H__
#define __SP2529_H__

#include <linux/ioctl.h>  /* For IOCTL macros */
#include <media/nvc.h>
#include <media/nvc_image.h>

struct sp2529_mode
{
	__u32 xres;
	__u32 yres;
};

struct sp2529_reg {
	u16 u16RegAddr;
	u8 u8Val;
	u8 u8Mask;
	u32 u32Delay_ms;
};


#define SP2529_IOCTL_SET_SENSOR_MODE	_IOW('o', 1, struct sp2529_mode)
#define SP2529_IOCTL_GET_SENSOR_STATUS	_IOR('o', 2, __u8)
#define SP2529_IOCTL_POWER_LEVEL		_IOW('o', 3, __u32)
#define SP2529_IOCTL_GET_AF_STATUS		_IOR('o', 4, __u8)
#define SP2529_IOCTL_SET_AF_MODE		_IOR('o', 5, __u8)
#define SP2529_IOCTL_SET_COLOR_EFFECT	_IOR('o', 6, __u8)
#define SP2529_IOCTL_SET_WHITE_BALANCE	_IOR('o', 7, __u8)
#define SP2529_IOCTL_SET_EXPOSURE		_IOR('o', 8, __u8)
#define SP2529_IOCTL_SET_SCENE_MODE		_IOR('o', 9, __u8)

#define SP2529_POWER_LEVEL_OFF			0
#define SP2529_POWER_LEVEL_ON			1


#ifdef __KERNEL__
struct sp2529_power_rail {
	struct regulator *avdd;
	struct regulator *dovdd;
	struct regulator *dvdd;
};

struct sp2529_platform_data {
	int (*power_on)(struct sp2529_power_rail *pw);
	int (*power_off)(struct sp2529_power_rail *pw);
	const char *mclk_name;
};
#endif /* __KERNEL__ */

enum {
	SP2529_MODE_1600x1200 = 0,
};

enum {
	YUV_ColorEffect_None,
	YUV_ColorEffect_Mono,
	YUV_ColorEffect_Sepia,
	YUV_ColorEffect_Negative,
	YUV_ColorEffect_Solarize,
	YUV_ColorEffect_Posterize,
};

enum {
	YUV_Whitebalance_Auto,
	YUV_Whitebalance_Incandescent,
	YUV_Whitebalance_Daylight,
	YUV_Whitebalance_Fluorescent,
	YUV_Whitebalance_CloudyDaylight,
};

enum {
	YUV_EXPOSURE_0,
	YUV_EXPOSURE_1,
	YUV_EXPOSURE_2,
	YUV_EXPOSURE_m1,
	YUV_EXPOSURE_m2,
};

#endif  /* __SP2529_H__ */
