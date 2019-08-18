/*
 *  Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *  Copyright (C) 2011, Bosch Sensortec GmbH All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  Date: 2011/4/8 11:00:00
 *  Revision: 2.5
 *
 * file BMA250.c
 * brief This file contains all function implementations for the BMA250 in linux
 *
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


#define SENSOR_NAME 			"bma250"
#define GRAVITY_EARTH                   9806550
#define ABSMIN_2G                       (-GRAVITY_EARTH * 2)
#define ABSMAX_2G                       (GRAVITY_EARTH * 2)
#define SLOPE_THRESHOLD_VALUE 		32
#define SLOPE_DURATION_VALUE 		1
#define INTERRUPT_LATCH_MODE 		13
#define INTERRUPT_ENABLE 			1
#define INTERRUPT_DISABLE 			0
#define MAP_SLOPE_INTERRUPT 		2
#define SLOPE_X_INDEX 				5
#define SLOPE_Y_INDEX 				6
#define SLOPE_Z_INDEX 				7
#define BMA250_MAX_DELAY			10
#define BMA250_CHIP_ID				3
#define BMA250_RANGE_SET			0
#define BMA250_BW_SET				4

/*
 *
 *      register definitions
 *
 */

#define BMA250_CHIP_ID_REG                      0x00
#define BMA250_VERSION_REG                      0x01
#define BMA250_X_AXIS_LSB_REG                   0x02
#define BMA250_X_AXIS_MSB_REG                   0x03
#define BMA250_Y_AXIS_LSB_REG                   0x04
#define BMA250_Y_AXIS_MSB_REG                   0x05
#define BMA250_Z_AXIS_LSB_REG                   0x06
#define BMA250_Z_AXIS_MSB_REG                   0x07
#define BMA250_TEMP_RD_REG                      0x08
#define BMA250_STATUS1_REG                      0x09
#define BMA250_STATUS2_REG                      0x0A
#define BMA250_STATUS_TAP_SLOPE_REG             0x0B
#define BMA250_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA250_RANGE_SEL_REG                    0x0F
#define BMA250_BW_SEL_REG                       0x10
#define BMA250_MODE_CTRL_REG                    0x11
#define BMA250_LOW_NOISE_CTRL_REG               0x12
#define BMA250_DATA_CTRL_REG                    0x13
#define BMA250_RESET_REG                        0x14
#define BMA250_INT_ENABLE1_REG                  0x16
#define BMA250_INT_ENABLE2_REG                  0x17
#define BMA250_INT1_PAD_SEL_REG                 0x19
#define BMA250_INT_DATA_SEL_REG                 0x1A
#define BMA250_INT2_PAD_SEL_REG                 0x1B
#define BMA250_INT_SRC_REG                      0x1E
#define BMA250_INT_SET_REG                      0x20
#define BMA250_INT_CTRL_REG                     0x21
#define BMA250_LOW_DURN_REG                     0x22
#define BMA250_LOW_THRES_REG                    0x23
#define BMA250_LOW_HIGH_HYST_REG                0x24
#define BMA250_HIGH_DURN_REG                    0x25
#define BMA250_HIGH_THRES_REG                   0x26
#define BMA250_SLOPE_DURN_REG                   0x27
#define BMA250_SLOPE_THRES_REG                  0x28
#define BMA250_TAP_PARAM_REG                    0x2A
#define BMA250_TAP_THRES_REG                    0x2B
#define BMA250_ORIENT_PARAM_REG                 0x2C
#define BMA250_THETA_BLOCK_REG                  0x2D
#define BMA250_THETA_FLAT_REG                   0x2E
#define BMA250_FLAT_HOLD_TIME_REG               0x2F
#define BMA250_STATUS_LOW_POWER_REG             0x31
#define BMA250_SELF_TEST_REG                    0x32
#define BMA250_EEPROM_CTRL_REG                  0x33
#define BMA250_SERIAL_CTRL_REG                  0x34
#define BMA250_CTRL_UNLOCK_REG                  0x35
#define BMA250_OFFSET_CTRL_REG                  0x36
#define BMA250_OFFSET_PARAMS_REG                0x37
#define BMA250_OFFSET_FILT_X_REG                0x38
#define BMA250_OFFSET_FILT_Y_REG                0x39
#define BMA250_OFFSET_FILT_Z_REG                0x3A
#define BMA250_OFFSET_UNFILT_X_REG              0x3B
#define BMA250_OFFSET_UNFILT_Y_REG              0x3C
#define BMA250_OFFSET_UNFILT_Z_REG              0x3D
#define BMA250_SPARE_0_REG                      0x3E
#define BMA250_SPARE_1_REG                      0x3F

#define BMA250_ACC_X_LSB__POS           6
#define BMA250_ACC_X_LSB__LEN           2
#define BMA250_ACC_X_LSB__MSK           0xC0
#define BMA250_ACC_X_LSB__REG           BMA250_X_AXIS_LSB_REG

#define BMA250_ACC_X_MSB__POS           0
#define BMA250_ACC_X_MSB__LEN           8
#define BMA250_ACC_X_MSB__MSK           0xFF
#define BMA250_ACC_X_MSB__REG           BMA250_X_AXIS_MSB_REG

#define BMA250_ACC_Y_LSB__POS           6
#define BMA250_ACC_Y_LSB__LEN           2
#define BMA250_ACC_Y_LSB__MSK           0xC0
#define BMA250_ACC_Y_LSB__REG           BMA250_Y_AXIS_LSB_REG

#define BMA250_ACC_Y_MSB__POS           0
#define BMA250_ACC_Y_MSB__LEN           8
#define BMA250_ACC_Y_MSB__MSK           0xFF
#define BMA250_ACC_Y_MSB__REG           BMA250_Y_AXIS_MSB_REG

#define BMA250_ACC_Z_LSB__POS           6
#define BMA250_ACC_Z_LSB__LEN           2
#define BMA250_ACC_Z_LSB__MSK           0xC0
#define BMA250_ACC_Z_LSB__REG           BMA250_Z_AXIS_LSB_REG

#define BMA250_ACC_Z_MSB__POS           0
#define BMA250_ACC_Z_MSB__LEN           8
#define BMA250_ACC_Z_MSB__MSK           0xFF
#define BMA250_ACC_Z_MSB__REG           BMA250_Z_AXIS_MSB_REG

#define BMA250_RANGE_SEL__POS             0
#define BMA250_RANGE_SEL__LEN             4
#define BMA250_RANGE_SEL__MSK             0x0F
#define BMA250_RANGE_SEL__REG             BMA250_RANGE_SEL_REG

#define BMA250_BANDWIDTH__POS             0
#define BMA250_BANDWIDTH__LEN             5
#define BMA250_BANDWIDTH__MSK             0x1F
#define BMA250_BANDWIDTH__REG             BMA250_BW_SEL_REG

#define BMA250_EN_LOW_POWER__POS          6
#define BMA250_EN_LOW_POWER__LEN          1
#define BMA250_EN_LOW_POWER__MSK          0x40
#define BMA250_EN_LOW_POWER__REG          BMA250_MODE_CTRL_REG

#define BMA250_EN_SUSPEND__POS            7
#define BMA250_EN_SUSPEND__LEN            1
#define BMA250_EN_SUSPEND__MSK            0x80
#define BMA250_EN_SUSPEND__REG            BMA250_MODE_CTRL_REG

#define BMA250_EN_LOW__POS            1
#define BMA250_EN_LOW__LEN            4
#define BMA250_EN_LOW__MSK            0x1E
#define BMA250_EN_LOW__REG            BMA250_MODE_CTRL_REG

#define BMA250_GET_BITSLICE(regvar, bitname)\
			((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA250_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

/* range and bandwidth */

#define BMA250_RANGE_2G                 0
#define BMA250_RANGE_4G                 1
#define BMA250_RANGE_8G                 2
#define BMA250_RANGE_16G                3

#define BMA250_BW_7_81HZ        0x08
#define BMA250_BW_15_63HZ       0x09
#define BMA250_BW_31_25HZ       0x0A
#define BMA250_BW_62_50HZ       0x0B
#define BMA250_BW_125HZ         0x0C
#define BMA250_BW_250HZ         0x0D
#define BMA250_BW_500HZ         0x0E
#define BMA250_BW_1000HZ        0x0F

/* mode settings */

#define BMA250_MODE_NORMAL      0
#define BMA250_MODE_LOWPOWER    1
#define BMA250_MODE_SUSPEND     2

struct bma250acc {
	s16 x, y, z;
};

struct bma250_data {
	struct i2c_client *bma250_client;
	atomic_t delay;
	atomic_t enable;
	unsigned char mode;
	struct input_dev *input;
	struct bma250acc value;
	struct mutex value_mutex;
	struct mutex enable_mutex;
	struct mutex mode_mutex;
	struct delayed_work work;
	struct work_struct irq_work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static int bma250_i2c_read(struct i2c_client *client, char *databuff, int len)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			//.scl_rate = 100000,
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= databuff,
		},
		{
			//.scl_rate = 100000,
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= databuff,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0)
		dev_err(&client->dev, "i2c read error: %d\n", ret);

	return ret;
}

static int bma250_i2c_write(struct i2c_client *client, char *databuff, int len)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			//.scl_rate = 100000,
			.addr	= client->addr,
			.flags	= 0,
			.len	= len,
			.buf	= databuff,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0)
		dev_err(&client->dev, "i2c write error: %d\n", ret);

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h);
static void bma250_late_resume(struct early_suspend *h);
#endif

static int bma250_smbus_read_byte(struct i2c_client *client,
				  unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -1;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma250_smbus_write_byte(struct i2c_client *client,
				   unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma250_smbus_read_byte_block(struct i2c_client *client,
					unsigned char reg_addr,
					unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma250_set_mode(struct i2c_client *client, unsigned char Mode)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else {
		if (Mode < 3) {
			comres = bma250_smbus_read_byte(client,
							BMA250_EN_LOW_POWER__REG,
							&data1);
			switch (Mode) {
			case BMA250_MODE_NORMAL:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_EN_LOW_POWER,
							    0);
				data1 =
				    BMA250_SET_BITSLICE(data1,
							BMA250_EN_SUSPEND, 0);
				break;
			case BMA250_MODE_LOWPOWER:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_EN_LOW_POWER,
							    1);
				data1 =
				    BMA250_SET_BITSLICE(data1,
							BMA250_EN_SUSPEND, 0);
				break;
			case BMA250_MODE_SUSPEND:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_EN_LOW_POWER,
							    0);
				data1 =
				    BMA250_SET_BITSLICE(data1,
							BMA250_EN_SUSPEND, 1);
				break;
			default:
				break;
			}

			comres += bma250_smbus_write_byte(client,
							  BMA250_EN_LOW_POWER__REG,
							  &data1);
		} else {
			comres = -1;
		}
	}

	return comres;
}

static int bma250_get_mode(struct i2c_client *client, unsigned char *Mode)
{
	int comres = 0;

	if (client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(client,
						BMA250_EN_LOW_POWER__REG, Mode);
		*Mode = (*Mode) >> 6;
	}

	return comres;
}

static int bma250_set_range(struct i2c_client *client, unsigned char Range)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else {
		if (Range < 4) {
			comres = bma250_smbus_read_byte(client,
							BMA250_RANGE_SEL_REG,
							&data1);
			switch (Range) {
			case 0:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_RANGE_SEL,
							    0);
				break;
			case 1:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_RANGE_SEL,
							    5);
				break;
			case 2:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_RANGE_SEL,
							    8);
				break;
			case 3:
				data1 = BMA250_SET_BITSLICE(data1,
							    BMA250_RANGE_SEL,
							    12);
				break;
			default:
				break;
			}
			comres += bma250_smbus_write_byte(client,
							  BMA250_RANGE_SEL_REG,
							  &data1);
		} else {
			comres = -1;
		}
	}

	return comres;
}

static int bma250_get_range(struct i2c_client *client, unsigned char *Range)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(client, BMA250_RANGE_SEL__REG,
						&data);
		data = BMA250_GET_BITSLICE(data, BMA250_RANGE_SEL);
		*Range = data;
	}

	return comres;
}

static int bma250_set_bandwidth(struct i2c_client *client, unsigned char BW)
{
	int comres = 0;
	unsigned char data;
	int Bandwidth = 0;

	if (client == NULL) {
		comres = -1;
	} else {
		if (BW < 8) {
			switch (BW) {
			case 0:
				Bandwidth = BMA250_BW_7_81HZ;
				break;
			case 1:
				Bandwidth = BMA250_BW_15_63HZ;
				break;
			case 2:
				Bandwidth = BMA250_BW_31_25HZ;
				break;
			case 3:
				Bandwidth = BMA250_BW_62_50HZ;
				break;
			case 4:
				Bandwidth = BMA250_BW_125HZ;
				break;
			case 5:
				Bandwidth = BMA250_BW_250HZ;
				break;
			case 6:
				Bandwidth = BMA250_BW_500HZ;
				break;
			case 7:
				Bandwidth = BMA250_BW_1000HZ;
				break;
			default:
				break;
			}
			comres = bma250_smbus_read_byte(client,
							BMA250_BANDWIDTH__REG,
							&data);
			data =
			    BMA250_SET_BITSLICE(data, BMA250_BANDWIDTH,
						Bandwidth);
			comres +=
			    bma250_smbus_write_byte(client,
						    BMA250_BANDWIDTH__REG,
						    &data);
		} else {
			comres = -1;
		}
	}

	return comres;
}

static int bma250_get_bandwidth(struct i2c_client *client, unsigned char *BW)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte(client, BMA250_BANDWIDTH__REG,
						&data);
		data = BMA250_GET_BITSLICE(data, BMA250_BANDWIDTH);
		if (data <= 8) {
			*BW = 0;
		} else {
			if (data >= 0x0F)
				*BW = 7;
			else
				*BW = data - 8;

		}
	}

	return comres;
}

static int bma250_read_accel_xyz(struct i2c_client *client,
				 struct bma250acc *acc)
{
	int comres;
	unsigned char data[6];
	//printk("[bma250_0323]:%s\n",__func__);
	if (client == NULL) {
		comres = -1;
	} else {
		comres = bma250_smbus_read_byte_block(client,
						      BMA250_ACC_X_LSB__REG,
						      data, 6);

		acc->x = BMA250_GET_BITSLICE(data[0], BMA250_ACC_X_LSB)
		    | (BMA250_GET_BITSLICE(data[1],
					   BMA250_ACC_X_MSB) <<
		       BMA250_ACC_X_LSB__LEN);
		acc->x =
		    acc->x << (sizeof(short) * 8 -
			       (BMA250_ACC_X_LSB__LEN + BMA250_ACC_X_MSB__LEN));
		acc->x =
		    acc->x >> (sizeof(short) * 8 -
			       (BMA250_ACC_X_LSB__LEN + BMA250_ACC_X_MSB__LEN));
		acc->y = BMA250_GET_BITSLICE(data[2], BMA250_ACC_Y_LSB)
		    | (BMA250_GET_BITSLICE(data[3],
					   BMA250_ACC_Y_MSB) <<
		       BMA250_ACC_Y_LSB__LEN);
		acc->y =
		    acc->y << (sizeof(short) * 8 -
			       (BMA250_ACC_Y_LSB__LEN + BMA250_ACC_Y_MSB__LEN));
		acc->y =
		    acc->y >> (sizeof(short) * 8 -
			       (BMA250_ACC_Y_LSB__LEN + BMA250_ACC_Y_MSB__LEN));

		acc->z = BMA250_GET_BITSLICE(data[4], BMA250_ACC_Z_LSB)
		    | (BMA250_GET_BITSLICE(data[5],
					   BMA250_ACC_Z_MSB) <<
		       BMA250_ACC_Z_LSB__LEN);
		acc->z =
		    acc->z << (sizeof(short) * 8 -
			       (BMA250_ACC_Z_LSB__LEN + BMA250_ACC_Z_MSB__LEN));
		acc->z =
		    acc->z >> (sizeof(short) * 8 -
			       (BMA250_ACC_Z_LSB__LEN + BMA250_ACC_Z_MSB__LEN));
	}

	return comres;
}

s64 ts_ns(void)
{
	struct timespec ts;

	ktime_get_ts(&ts);
	return timespec_to_ns(&ts);
}

static void bma250_work_func(struct work_struct *work)
{
	struct bma250_data *bma250 = container_of((struct delayed_work *)work,
						  struct bma250_data, work);
	static struct bma250acc acc;
	unsigned long delay = msecs_to_jiffies(atomic_read(&bma250->delay));
	s64 ts;

	ts = ts_ns();
	//printk("[bma250_0323]:%s\n",__func__);
	bma250_read_accel_xyz(bma250->bma250_client, &acc);
	/*adjust the data output for sku1 compatible */
	//printk("[bma250_0323]:%s X:%d Y:%d Z:%d\n",__func__,acc.y * 4,-acc.x * 4,acc.z * 4);
	
//#if 0
	input_report_abs(bma250->input, ABS_X, acc.x * -4 * 17);
	input_report_abs(bma250->input, ABS_Y, acc.y * -4 *17);
	input_report_abs(bma250->input, ABS_Z, acc.z * 4 *17);
//#endif
#if 0
	input_report_rel(bma250->input, ABS_RX, acc.y * 4);
	input_report_rel(bma250->input, ABS_RY, -acc.x * 4);
	input_report_rel(bma250->input, ABS_RZ, acc.z * 4);
#endif
	input_report_abs(bma250->input, ABS_MISC, (unsigned int)(ts >> 32));
	input_report_abs(bma250->input, ABS_WHEEL,
			 (unsigned int)(ts & 0xffffffff));
	input_sync(bma250->input);

	mutex_lock(&bma250->value_mutex);
	bma250->value = acc;
	mutex_unlock(&bma250->value_mutex);
	schedule_delayed_work(&bma250->work, delay);
}

static ssize_t bma250_range_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	if (bma250_get_range(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_range_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_range(bma250->bma250_client, (unsigned char)data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_bandwidth_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	if (bma250_get_bandwidth(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma250_bandwidth_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_bandwidth(bma250->bma250_client,
				 (unsigned char)data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	if (bma250_get_mode(bma250->bma250_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma250_set_mode(bma250->bma250_client, (unsigned char)data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma250_value_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bma250_data *bma250 = input_get_drvdata(input);
	struct bma250acc acc_value;
	printk("[bma250_0323]:%s\n",__func__);
	mutex_lock(&bma250->value_mutex);
	acc_value = bma250->value;
	mutex_unlock(&bma250->value_mutex);

	return sprintf(buf, "%d %d %d\n", acc_value.x, acc_value.y,
		       acc_value.z);
}

static ssize_t bma250_delay_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	return sprintf(buf, "%d\n", atomic_read(&bma250->delay));

}

static ssize_t bma250_delay_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > BMA250_MAX_DELAY)
		data = BMA250_MAX_DELAY;
	atomic_set(&bma250->delay, (unsigned int)data);

	return count;
}

static ssize_t bma250_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	printk("[bma250_0323]:%s\n",__func__);
	return sprintf(buf, "%d\n", atomic_read(&bma250->enable));

}

static void bma250_set_enable(struct device *dev, int enable)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma250_data *bma250 = i2c_get_clientdata(client);
	int pre_enable = atomic_read(&bma250->enable);
	printk("[bma250_0323]:%s\n",__func__);
	mutex_lock(&bma250->enable_mutex);

	if (enable) {
		if (pre_enable == 0) {
			bma250_set_mode(bma250->bma250_client,BMA250_MODE_NORMAL);
			schedule_delayed_work(&bma250->work,msecs_to_jiffies(atomic_read(&bma250->delay)));
			atomic_set(&bma250->enable, 1);
		}

	} else {
		if (pre_enable == 1) {
			bma250_set_mode(bma250->bma250_client,BMA250_MODE_SUSPEND);
			cancel_delayed_work_sync(&bma250->work);
			atomic_set(&bma250->enable, 0);
		}
	}
	mutex_unlock(&bma250->enable_mutex);

}

static ssize_t bma250_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long data;
	int error;
	printk("[bma250_0323]:%s\n",__func__);
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0) || (data == 1)) {
		bma250_set_enable(dev, data);
	}

	return count;
}

static DEVICE_ATTR(range, S_IRUGO | S_IWUSR | S_IWGRP,
		   bma250_range_show, bma250_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO | S_IWUSR | S_IWGRP,
		   bma250_bandwidth_show, bma250_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWGRP,
		   bma250_mode_show, bma250_mode_store);
static DEVICE_ATTR(value, S_IRUGO, bma250_value_show, NULL);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   bma250_delay_show, bma250_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   bma250_enable_show, bma250_enable_store);

static struct attribute *bma250_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_bandwidth.attr,
	&dev_attr_mode.attr,
	&dev_attr_value.attr,
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bma250_attribute_group = {
	.attrs = bma250_attributes
};

static struct bma250_data *g_bma250_data;

static int bma250_input_init(struct bma250_data *bma250)
{
	struct input_dev *dev;
	int err;
	printk("[bma250_0323]:%s\n",__func__);
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	//dev->name = "accelerometer";	//SENSOR_NAME; changed for sku1 compatible
	//dev->name = SENSOR_NAME;
	dev->name = "MPU6050";
	dev->id.bustype = BUS_I2C;

//#if 0
	input_set_capability(dev, EV_ABS, ABS_X);
	input_set_abs_params(dev, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_capability(dev, EV_ABS, ABS_Y);
	input_set_abs_params(dev, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_capability(dev, EV_ABS, ABS_Z);
	input_set_abs_params(dev, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
//#endif	
#if 0
	/* X */
	input_set_capability(dev, EV_REL, REL_RX);
	input_set_abs_params(dev, REL_RX, ABSMIN_2G, ABSMAX_2G, 0, 0);
	/* Y */
	input_set_capability(dev, EV_REL, REL_RY);
	input_set_abs_params(dev, REL_RY, ABSMIN_2G, ABSMAX_2G, 0, 0);
	/* Z */
	input_set_capability(dev, EV_REL, REL_RZ);
	input_set_abs_params(dev, REL_RZ, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(dev, bma250);
#endif

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	bma250->input = dev;
	printk("[bma250_0323]:%s end\n",__func__);
	return 0;
}

static void bma250_input_delete(struct bma250_data *bma250)
{
	struct input_dev *dev = bma250->input;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int bma250_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	char tempvalue = 0;
	struct bma250_data *data;
	printk("[bma250_0323]:%s begin\n",__func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_INFO "i2c_check_functionality error\n");
		goto exit;
	}
	data = kzalloc(sizeof(struct bma250_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	/* read chip id */
	tempvalue = BMA250_CHIP_ID_REG;
	if (bma250_i2c_read(client, &tempvalue, 1) > 0)
	{
		if ((tempvalue & 0xFF) == BMA250_CHIP_ID)
		{
			printk(KERN_INFO "Bosch Sensortec Device detected!\n"
			   "BMA250 registered I2C driver!\n");
		}
		else
		{
			printk(KERN_INFO "Bosch Sensortec Device not found, \
					i2c error %d \n", tempvalue);
			//err = -1;
			//goto kfree_exit;
		}

	}

	i2c_set_clientdata(client, data);
	data->bma250_client = client;
	mutex_init(&data->value_mutex);
	mutex_init(&data->mode_mutex);
	mutex_init(&data->enable_mutex);
	bma250_set_bandwidth(client, BMA250_BW_SET);
	bma250_set_range(client, BMA250_RANGE_SET);

	INIT_DELAYED_WORK(&data->work, bma250_work_func);
	atomic_set(&data->delay, BMA250_MAX_DELAY);
	//atomic_set(&data->enable, 0);
	atomic_set(&data->enable, 1);
	err = bma250_input_init(data);
	if (err < 0)
		goto kfree_exit;

	err = sysfs_create_group(&data->input->dev.kobj,
				 &bma250_attribute_group);
	if (err < 0)
		goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = bma250_early_suspend;
	data->early_suspend.resume = bma250_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	/* try to reduce the power comsuption */
	bma250_set_mode(data->bma250_client, BMA250_MODE_NORMAL);
	printk("[bma250_0323]:%s end\n",__func__);
	/*work func begin*/
	bma250_set_mode(data->bma250_client,BMA250_MODE_NORMAL);
	schedule_delayed_work(&data->work,msecs_to_jiffies(atomic_read(&data->delay)));

	g_bma250_data = data;

	return 0;

error_sysfs:
	printk("[bma250_0323]:error_sysfs %s\n",__func__);
	bma250_input_delete(data);

kfree_exit:
	printk("[bma250_0323]:kfree_exit %s\n",__func__);
	kfree(data);
exit:
	printk("[bma250_0323]:exit %s\n",__func__);
	return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h)
{
	struct bma250_data *data =
	    container_of(h, struct bma250_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_SUSPEND);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);
}

static void bma250_late_resume(struct early_suspend *h)
{
	struct bma250_data *data =
	    container_of(h, struct bma250_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_NORMAL);
		schedule_delayed_work(&data->work,
				      msecs_to_jiffies(atomic_read
						       (&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);
}
#endif

static int bma250_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct bma250_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_SUSPEND);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);

	return 0;
}

static int bma250_resume(struct i2c_client *client)
{
	struct bma250_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		bma250_set_mode(data->bma250_client, BMA250_MODE_NORMAL);
		schedule_delayed_work(&data->work,
				      msecs_to_jiffies(atomic_read
						       (&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);

	return 0;
}

void disable_gsensor(void)
{
	if (g_bma250_data){
		__cancel_delayed_work(&g_bma250_data->work);
	}
	return 0;
}


void enable_gsensor(void)
{
	if (g_bma250_data){

		schedule_delayed_work(&g_bma250_data->work, msecs_to_jiffies(atomic_read(&g_bma250_data->delay)));
	}
	return 0;
}

static int bma250_remove(struct i2c_client *client)
{
	struct bma250_data *data = i2c_get_clientdata(client);

	bma250_set_enable(&client->dev, 0);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif

	sysfs_remove_group(&data->input->dev.kobj, &bma250_attribute_group);
	bma250_input_delete(data);
	kfree(data);
	g_bma250_data = NULL;
	return 0;
}

static const struct i2c_device_id bma250_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, bma250_id);

static struct i2c_driver bma250_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.suspend = bma250_suspend,
	.resume = bma250_resume,
	.id_table = bma250_id,
	.probe = bma250_probe,
	.remove = bma250_remove,

};

static int __init BMA250_init(void)
{
	return i2c_add_driver(&bma250_driver);
}

static void __exit BMA250_exit(void)
{
	i2c_del_driver(&bma250_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA250 driver");
MODULE_LICENSE("GPL");

module_init(BMA250_init);
module_exit(BMA250_exit);
