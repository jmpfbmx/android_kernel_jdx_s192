#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/input/edt-ft5x06.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


#define FT5x06_REG_DEVICE_MODE			0x00

//TP mode
#define FT5x06_WORKMODE_VALUE			0x00
#define FT5x06_REG_TEST_MODE			0x04
#define FT5x06_REG_TEST_MODE_2			0x05
#define FT5x06_RX_TEST_MODE_1			0x1E
#define FT5x06_TX_TEST_MODE_1			0x28
#define FT5x06_FACTORYMODE_VALUE		0x40

/*factory mode register*/
#define FT5x06_REG_TX_NUM				0x03
#define FT5x06_REG_RX_NUM				0x04
#define FT5x06_REG_VOLTAGE				0x05
#define FT5x06_REG_START_RX				0x06
#define FT5x06_REG_GAIN					0x07
#define FT5x06_REG_ADC_TARGET_HIGH		0x08
#define FT5x06_REG_ADC_TARGET_LOW		0x09
#define FT5x06_REG_SCAN_SELECT			0x4E
#define FT5x06_REG_TX_ORDER_START		0x50
#define FT5x06_REG_TX_CAP_START			0x78
#define FT5x06_REG_RX_CAP_START			0xA0
#define FT5x06_REG_TX_OFFSET_START		0xBF
#define FT5x06_REG_RX_OFFSET_START		0xD3
#define FT5x06_REG_RX_ORDER_START		0xea


/*work mode register*/
#define FT5x06_REG_THGROUP				(0x00 + 0x80)
#define FT5x06_REG_THPEAK 				(0x01 + 0x80)
#define FT5x06_REG_THCAL 				(0x02 + 0x80)
#define FT5x06_REG_THWATER 				(0x03 + 0x80)
#define FT5x06_REG_THFALSE_TOUCH_PEAK 	(0x04 + 0x80)
#define FT5x06_REG_THDIFF				(0x05 + 0x80)
#define FT5x06_REG_PWMODE_CTRL			(0x06 + 0x80)
#define FT5x06_REG_TIME_ENTER_MONITOR 	(0x07 + 0x80)
#define FT5x06_REG_PERIOD_ACTIVE		(0x08 + 0x80)
#define FT5x06_REG_PERIOD_MONITOR 		(0x09 + 0x80)
#define FT5x06_REG_POINTS_SUPPORTED		(0x0a + 0x80)
#define FT5x06_REG_FILTER_FRAME_NOISE	(0x0b + 0x80)	
#define FT5x06_REG_KX_LR_H				(0x0c + 0x80)	
#define FT5x06_REG_KX_LR_L				(0x0d + 0x80)	
#define FT5x06_REG_KY_UD_H				(0x0E + 0x80)	
#define FT5x06_REG_KY_UD_L				(0x0F + 0x80)	
#define FT5x06_REG_POWERNOISE_FILTER_TH	(0x10 + 0x80)
#define FT5x06_REG_ESD_FILTER_FRAME		(0x11 + 0x80)

#define FT5x06_REG_RESOLUTION_X_H		(0x18 + 0x80)
#define FT5x06_REG_RESOLUTION_X_L		(0x19 + 0x80)
#define FT5x06_REG_RESOLUTION_Y_H		(0x1a + 0x80)
#define FT5x06_REG_RESOLUTION_Y_L		(0x1b + 0x80)
#define FT5x06_REG_KX_H					(0x1c + 0x80)
#define FT5x06_REG_KX_L					(0x1d + 0x80)
#define FT5x06_REG_KY_H					(0x1e + 0x80)
#define FT5x06_REG_KY_L					(0x1f + 0x80)
#define FT5x06_REG_AUTO_CLB_MODE		(0x20 + 0x80)
#define FT5x06_REG_LIB_VERSION_H		(0x21 + 0x80)
#define FT5x06_REG_LIB_VERSION_L		(0x22 + 0x80)
#define FT5x06_REG_CIPHER_ID			(0x23 + 0x80)
#define FT5x06_REG_MODE					(0x24 + 0x80)
#define FT5x06_REG_PMODE				(0x25 + 0x80)
#define FT5x06_REG_FIRMWARE_ID			(0x26 + 0x80)
#define FT5x06_REG_STATE				(0x27 + 0x80)
#define FT5x06_REG_CUSTOMER_ID			(0x28 + 0x80)

#define FT5x06_REG_OTP_PARAM_ID			(0x2a + 0x80)
#define FT5x06_REG_STATIC_TH			(0x2b + 0x80)
#define FT5x06_REG_MID_SPEED_TH			(0x2c + 0x80)
#define FT5x06_REG_HIGH_SPEED_TH		(0x2d + 0x80)
#define FT5x06_REG_DRAW_LINE_TH			(0x2e + 0x80)

#define FT5x06_REG_FACE_DETECT_MODE		(0x33 + 0x80)
#define FT5x06_REG_MAX_TOUCH_VALUE_HIGH	(0x34 + 0x80)
#define FT5x06_REG_MAX_TOUCH_VALUE_LOW	(0x35 + 0x80)
#define FT5x06_REG_MOVSTH_I				(0x36 + 0x80)
#define FT5x06_REG_MOVSTH_N				(0x37 + 0x80)

#define FT5x06_REG_DIRECTION			(0x40 + 0x80)
#define FT5x06_REG_LEMDA_X				(0x41 + 0x80)
#define FT5x06_REG_LEMDA_Y				(0x42 + 0x80)
#define FT5x06_REG_FACE_DETECT_STATISTICS_TX_NUM	(0x43 + 0x80)
#define FT5x06_REG_FACE_DETECT_PRE_VALUE			(0x44 + 0x80)
#define FT5x06_REG_FACE_DETECT_NUM					(0x45 + 0x80)
#define FT5x06_REG_FACE_DETECT_LAST_TIME_H			(0x46 + 0x80)
#define FT5x06_REG_FACE_DETECT_LAST_TIME_L			(0x47 + 0x80)
#define FT5x06_REG_FACE_DETECT_ON					(0x48 + 0x80)
#define FT5x06_REG_FACE_DETECT_OFF					(0x49 + 0x80)
#define FT5x06_REG_BIGAREA_PEAK_VALUE_MIN			(0x4a + 0x80)
#define FT5x06_REG_BIGAREA_DIFF_VALUE_OVER_NUM		(0x4b + 0x80)	
#define FT5x06_REG_BIGAREA_POINT_AUTO_CLEAR_TIME_H	(0x4c + 0x80)
#define FT5x06_REG_BIGAREA_POINT_AUTO_CLEAR_TIME_L	(0x4d + 0x80)
#define FT5x06_REG_DIFFDATA_HADDLE_VALUE			(0x4e + 0x80)
#define FT5x06_REG_ABNORMAL_DIFF_VALUE				(0x4f + 0x80)
#define FT5x06_REG_ABNORMAL_DIFF_NUM				(0x50 + 0x80)
#define FT5x06_REG_ABNORMAL_DIFF_LAST_FRAME			(0x51 + 0x80)

#define FT5x06_KX									204
#define FT5x06_KY									207
#define FT5x06_LEMDA_X								46
#define FT5x06_LEMDA_Y								45
#define FT5x06_RESOLUTION_X							1920
#define FT5x06_RESOLUTION_Y							1200
#define FT5x06_DIRECTION							0
#define FT5x06_FACE_DETECT_PRE_VALUE				20
#define FT5x06_FACE_DETECT_NUM						10
#define FT5x06_BIGAREA_PEAK_VALUE_MIN				255
#define FT5x06_BIGAREA_DIFF_VALUE_OVER_NUM			30
#define FT5x06_BIGAREA_POINT_AUTO_CLEAR_TIME		3000
#define FT5x06_FACE_DETECT_LAST_TIME				1000
#define FT5x06_MODE									1
#define FT5x06_PMODE								0
#define FT5x06_FIRMWARE_ID							14
#define FT5x06_STATE								1
#define FT5x06_CUSTOMER_ID							121
#define FT5x06_PERIOD_ACTIVE						14
#define FT5x06_FACE_DETECT_STATISTICS_TX_NUM		3
#define FT5x06_THGROUP								100
#define FT5x06_THPEAK								60
#define FT5x06_FACE_DETECT_MODE						0
#define FT5x06_MAX_TOUCH_VALUE						1200
#define FT5x06_THFALSE_TOUCH_PEAK					135
#define FT5x06_THDIFF								2560
#define FT5x06_PWMODE_CTRL							1
#define FT5x06_TIME_ENTER_MONITOR					10
#define FT5x06_PERIOD_MONITOR						40
#define FT5x06_AUTO_CLB_MODE						255
#define FT5x06_DRAW_LINE_TH							250
#define FT5x06_DIFFDATA_HADDLE_VALUE				-60
#define FT5x06_ABNORMAL_DIFF_VALUE					60
#define FT5x06_ABNORMAL_DIFF_NUM					15
#define FT5x06_ABNORMAL_DIFF_LAST_FRAME				30
#define FT5x06_POINTS_SUPPORTED						5
#define FT5x06_STATIC_TH							232
#define FT5x06_MID_SPEED_TH							17
#define FT5x06_HIGH_SPEED_TH						200
#define FT5x06_START_RX								0
#define FT5x06_ADC_TARGET							8500
#define FT5x06_FILTER_FRAME_NOISE					3
#define FT5x06_POWERNOISE_FILTER_TH					0
#define FT5x06_KX_LR								366
#define FT5x06_KY_UD								362
#define FT5x06_ESD_FILTER_FRAME						0
#define FT5x06_MOVSTH_I								4
#define FT5x06_MOVSTH_N								3

#define MAX_CONTACTS								5
#define SCREEN_MAX_X								FT5x06_RESOLUTION_X
#define SCREEN_MAX_Y								FT5x06_RESOLUTION_Y
#define PRESS_MAX									255
#define FT5X06_TS_NAME								"ft5x06-touch"
#define IDX_PACKET_SIZE								(6 * MAX_CONTACTS + 1)

static struct ft5x06_data
{
	struct i2c_client *client;
	struct input_dev *input;
	int irq;
	int reset_pin;
	int irq_pin;
	struct workqueue_struct *wq;
	struct work_struct work;
	struct delayed_work	delayed_work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int use_irq;
	struct regulator *vdd_ts_en; 
}ft5x06_touch_data;

struct point_data
{
	unsigned char status;
	unsigned short x;
	unsigned short y;
}ft5x06_touch_point[MAX_CONTACTS];

struct Struct_Param_FT5x06
{
	unsigned short ft5x06_KX;
	unsigned short ft5x06_KY;
	unsigned char ft5x06_LEMDA_X;
	unsigned char ft5x06_LEMDA_Y;
	unsigned short ft5x06_RESOLUTION_X;
	unsigned short ft5x06_RESOLUTION_Y;
	unsigned char ft5x06_DIRECTION;
	unsigned char ft5x06_FACE_DETECT_PRE_VALUE;
	unsigned char ft5x06_FACE_DETECT_NUM;

	unsigned char ft5x06_BIGAREA_PEAK_VALUE_MIN;
	unsigned char ft5x06_BIGAREA_DIFF_VALUE_OVER_NUM;
	unsigned short ft5x06_BIGAREA_POINT_AUTO_CLEAR_TIME;
	unsigned short ft5x06_FACE_DETECT_LAST_TIME;
	unsigned char ft5x06_MODE;
	unsigned char ft5x06_PMODE;
	unsigned char ft5x06_FIRMWARE_ID;
	unsigned char ft5x06_STATE;
	unsigned char ft5x06_CUSTOMER_ID;
	unsigned char ft5x06_PERIOD_ACTIVE;
	unsigned char  ft5x06_FACE_DETECT_STATISTICS_TX_NUM;

	unsigned short ft5x06_THGROUP;
	unsigned char ft5x06_THPEAK;
	unsigned char ft5x06_FACE_DETECT_MODE;
	unsigned short ft5x06_MAX_TOUCH_VALUE;

	unsigned char ft5x06_THFALSE_TOUCH_PEAK;
	unsigned short ft5x06_THDIFF;
	unsigned char ft5x06_PWMODE_CTRL;
	unsigned char ft5x06_TIME_ENTER_MONITOR;
	unsigned char ft5x06_PERIOD_MONITOR;
	unsigned char ft5x06_AUTO_CLB_MODE;
	unsigned char ft5x06_DRAW_LINE_TH;
	unsigned char ft5x06_DIFFDATA_HADDLE_VALUE;

	unsigned char ft5x06_ABNORMAL_DIFF_VALUE;
	unsigned char ft5x06_ABNORMAL_DIFF_NUM;
	unsigned char ft5x06_ABNORMAL_DIFF_LAST_FRAME;
	unsigned char ft5x06_POINTS_SUPPORTED;
		
	unsigned char ft5x06_STATIC_TH;
	unsigned char ft5x06_MID_SPEED_TH;
	unsigned char ft5x06_HIGH_SPEED_TH;
	unsigned char ft5x06_START_RX;
	unsigned short ft5x06_ADC_TARGET;
	unsigned char ft5x06_FILTER_FRAME_NOISE;
	unsigned char ft5x06_POWERNOISE_FILTER_TH;
	unsigned short ft5x06_KX_LR;
	unsigned short ft5x06_KY_UD;
	unsigned char ft5x06_ESD_FILTER_FRAME;
	unsigned char ft5x06_MOVSTH_I;
	unsigned char ft5x06_MOVSTH_N;
};

struct Struct_Param_FT5x06 ft5x06_hw_param = {
	FT5x06_KX,
	FT5x06_KY,
	FT5x06_LEMDA_X,
	FT5x06_LEMDA_Y,
	FT5x06_RESOLUTION_X,
	FT5x06_RESOLUTION_Y,
	FT5x06_DIRECTION,

	FT5x06_FACE_DETECT_PRE_VALUE,
	FT5x06_FACE_DETECT_NUM,
	FT5x06_BIGAREA_PEAK_VALUE_MIN,
	FT5x06_BIGAREA_DIFF_VALUE_OVER_NUM,
	FT5x06_BIGAREA_POINT_AUTO_CLEAR_TIME,
	FT5x06_FACE_DETECT_LAST_TIME,
	FT5x06_MODE,
	FT5x06_PMODE,
	FT5x06_FIRMWARE_ID,
	FT5x06_STATE,
	FT5x06_CUSTOMER_ID,
	FT5x06_PERIOD_ACTIVE,
	FT5x06_FACE_DETECT_STATISTICS_TX_NUM,

	FT5x06_THGROUP,
	FT5x06_THPEAK,
	FT5x06_FACE_DETECT_MODE,
	FT5x06_MAX_TOUCH_VALUE,

	FT5x06_THFALSE_TOUCH_PEAK,
	FT5x06_THDIFF,
	FT5x06_PWMODE_CTRL,
	FT5x06_TIME_ENTER_MONITOR,
	FT5x06_PERIOD_MONITOR,
	FT5x06_AUTO_CLB_MODE,
	FT5x06_DRAW_LINE_TH,
	FT5x06_DIFFDATA_HADDLE_VALUE,
	FT5x06_ABNORMAL_DIFF_VALUE, 
	FT5x06_ABNORMAL_DIFF_NUM,
	FT5x06_ABNORMAL_DIFF_LAST_FRAME,
	FT5x06_POINTS_SUPPORTED,

	FT5x06_STATIC_TH,
	FT5x06_MID_SPEED_TH,
	FT5x06_HIGH_SPEED_TH,
	FT5x06_START_RX,
	FT5x06_ADC_TARGET,
	FT5x06_FILTER_FRAME_NOISE,
	FT5x06_POWERNOISE_FILTER_TH,
	FT5x06_KX_LR,
	FT5x06_KY_UD,
	FT5x06_ESD_FILTER_FRAME,
	FT5x06_MOVSTH_I,
	FT5x06_MOVSTH_N,
};

//unsigned char ft5x06_tx_order[] = {19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
unsigned char ft5x06_tx_order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
unsigned char ft5x06_tx_cap[] = {42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42};
unsigned char ft5x06_tx_offset = 1;

unsigned char ft5x06_rx_order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
unsigned char ft5x06_rx_cap[] = {84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84, 84};
unsigned char ft5x06_rx_offset[] = {187, 221, 203, 186, 188, 221};

unsigned char ft5x06_gain = 15;
unsigned char ft5x06_voltage = 2;
unsigned char ft5x06_scanselect = 7; // µçÔ´¸ÉÈÅ£¬4, 5, 6, 7, 8ÊÔ

#define SABRESD_FT5x06_WAKE	IMX_GPIO_NR(1, 3)

static int ft5x06_i2c_read(struct i2c_client *client, char *databuff, int len)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= databuff,
		},
		{
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

static int ft5x06_i2c_write(struct i2c_client *client, char *databuff, int len)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
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

static int ft5x06_write_reg(struct i2c_client *client, unsigned char regaddr, unsigned char regvalue)
{
	unsigned char databuff[2] = {0};

	databuff[0] = regaddr;
	databuff[1] = regvalue;
	
	return ft5x06_i2c_write(client, databuff, 2);
}

static int ft5x06_read_reg(struct i2c_client *client, unsigned char regaddr, unsigned char *regvalue)
{
	regvalue[0] = regaddr;
	return ft5x06_i2c_read(client, regvalue, 1);
}


/*************************************************************/
/****************                    Hardware Init              *****************/
/*************************************************************/
#if 0
static int ft5x06_set_tx_order(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	int ret = 0;
	
	if (offset < FT5x06_TX_TEST_MODE_1)
		ret = ft5x06_write_reg(client, FT5x06_REG_TX_ORDER_START + offset, value);
	else {
		ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE_2 << 4);
		if (ret >= 0)
			ret = ft5x06_write_reg(client, FT5x06_REG_TX_ORDER_START + offset - FT5x06_TX_TEST_MODE_1, value);
		ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE << 4);
	}
	
	return ret;
}

static int ft5x06_set_tx_cap(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	int ret = 0;
	
	if (offset < FT5x06_TX_TEST_MODE_1)
		ret = ft5x06_write_reg(client, FT5x06_REG_TX_CAP_START + offset, value);
	else {
		ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE_2 << 4);
		if (ret >= 0)
			ret = ft5x06_write_reg(client, FT5x06_REG_TX_CAP_START + offset - FT5x06_TX_TEST_MODE_1, value);
		ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE << 4);
	}
	
	return ret;
}

static int ft5x06_set_tx_offset(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	int ret = 0;
	unsigned char temp = 0;

	if (offset < FT5x06_TX_TEST_MODE_1)
	{
		ret = ft5x06_read_reg(client, FT5x06_REG_TX_OFFSET_START + (offset >> 1), &temp);
		if (ret > 0)
		{
			if (ret % 2 == 0)
				ret = ft5x06_write_reg(client, FT5x06_REG_TX_OFFSET_START + (offset >> 1), (temp & 0xf0) + (value & 0x0f));
			else
				ret = ft5x06_write_reg(client, FT5x06_REG_TX_OFFSET_START + (offset >> 1), (temp & 0xf0) + (value << 4));
		}
	}
	else {
		ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE_2 << 4);
		if (ret >= 0)
		{
			ret = ft5x06_read_reg(client, FT5x06_REG_DEVICE_MODE + ((offset - FT5x06_TX_TEST_MODE_1) >> 1), &temp);
			if (ret > 0)
			{
				if (ret % 2 == 0)
					ret = ft5x06_write_reg(client, FT5x06_REG_TX_OFFSET_START + ((offset - FT5x06_TX_TEST_MODE_1) >> 1), (temp & 0xf0) + (value & 0x0f));
				else
					ret = ft5x06_write_reg(client, FT5x06_REG_TX_OFFSET_START + ((offset - FT5x06_TX_TEST_MODE_1) >> 1), (temp & 0xf0) + (value << 4));
			}
		}
		ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE << 4);
	}

	return ret;
}

static int ft5x06_set_rx_order(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_RX_ORDER_START + offset, value);
}

static int ft5x06_set_rx_cap(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	int ret = 0;
	
	if (offset < FT5x06_RX_TEST_MODE_1)
		ret = ft5x06_write_reg(client, FT5x06_REG_RX_CAP_START + offset, value);
	else {
		ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE_2 << 4);
		if (ret >= 0)
			ret = ft5x06_write_reg(client, FT5x06_REG_RX_CAP_START + offset - FT5x06_TX_TEST_MODE_1, value);
		ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE << 4);
	}
	
	return ret;
}

static int ft5x06_set_rx_offset(struct i2c_client * client, unsigned char offset, unsigned char value)
{
	int ret = 0;
	unsigned char temp = 0;

	if (offset < FT5x06_RX_TEST_MODE_1)
	{
		ret = ft5x06_read_reg(client, FT5x06_REG_RX_OFFSET_START + (offset >> 1), &temp);
		if (ret > 0)
		{
			if (ret % 2 == 0)
				ret = ft5x06_write_reg(client, FT5x06_REG_RX_OFFSET_START + (offset >> 1), (temp & 0xf0) + (value & 0x0f));
			else
				ret = ft5x06_write_reg(client, FT5x06_REG_RX_OFFSET_START + (offset >> 1), (temp & 0xf0) + (value << 4));
		}
	}
	else {
		ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE_2 << 4);
		if (ret >= 0)
		{
			ret = ft5x06_read_reg(client, FT5x06_REG_DEVICE_MODE + ((offset - FT5x06_RX_TEST_MODE_1) >> 1), &temp);
			if (ret > 0)
			{
				if (ret % 2 == 0)
					ret = ft5x06_write_reg(client, FT5x06_REG_RX_OFFSET_START + ((offset - FT5x06_RX_TEST_MODE_1) >> 1), (temp & 0xf0) + (value & 0x0f));
				else
					ret = ft5x06_write_reg(client, FT5x06_REG_RX_OFFSET_START + ((offset - FT5x06_RX_TEST_MODE_1) >> 1), (temp & 0xf0) + (value << 4));
			}
		}
		ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_REG_TEST_MODE << 4);
	}

	return ret;
}

static int ft5x06_set_scan_select(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_SCAN_SELECT, value);
}

static int ft5x06_set_tx_num(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_TX_NUM, value);
}

static int ft5x06_set_rx_num(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_RX_NUM, value);
}

static int ft5x06_set_gain(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_GAIN, value);
}

static int ft5x06_set_vol(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_VOLTAGE, value);
}

static int ft5x06_set_Resolution(struct i2c_client *client, unsigned short x, unsigned short y)
{
	int ret = 0;
	
	ret = ft5x06_write_reg(client, FT5x06_REG_RESOLUTION_X_H, ((unsigned char)(x >> 8)));
	if (ret < 0)
		return ret;
	
	ret = ft5x06_write_reg(client, FT5x06_REG_RESOLUTION_X_L, ((unsigned char)(x & 0xff)));
	if (ret < 0)
		return ret;
	
	ret = ft5x06_write_reg(client, FT5x06_REG_RESOLUTION_Y_H, ((unsigned char)(y >> 8)));
	if (ret < 0)
		return ret;
	
	ret = ft5x06_write_reg(client, FT5x06_REG_RESOLUTION_Y_L, ((unsigned char)(y & 0xff)));
	if (ret < 0)
		return ret;

	return ret;
}

static int ft5x06_set_face_detect_pre_value(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_PRE_VALUE, value);
}

static int ft5x06_set_face_detect_num(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_NUM, value);
}

static int ft5x06_set_face_detect_last_time(struct i2c_client *client, unsigned short value)
{
	int ret = 0;
	unsigned char temp[2] = {0};

	temp[0] = value >> 8;
	temp[1] = value & 0xff;

	ret = ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_LAST_TIME_H, temp[0]);
	if (ret < 0)
		return ret;

	ret = ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_LAST_TIME_L, temp[1]);
	if (ret < 0)
		return ret;

	return ret;
}

static int ft5x06_set_peak_value_min(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_BIGAREA_PEAK_VALUE_MIN, value);
}

static int ft5x06_set_diff_value_over_num(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_BIGAREA_DIFF_VALUE_OVER_NUM, value);
}

static int ft5x06_set_point_auto_clear_time(struct i2c_client *client, unsigned short value)
{
	int ret = 0;
	unsigned char temp[2] = {0};

	temp[0] = value >> 8;
	temp[1] = value & 0xff;

	ret = ft5x06_write_reg(client, FT5x06_REG_BIGAREA_POINT_AUTO_CLEAR_TIME_H, temp[0]);
	if (ret < 0)
		return ret;

	ret = ft5x06_write_reg(client, FT5x06_REG_BIGAREA_POINT_AUTO_CLEAR_TIME_L, temp[1]);
	if (ret < 0)
		return ret;

	return ret;
}

static int ft5x06_set_kx(struct i2c_client *client, unsigned short value)
{
	int ret = 0;
	unsigned char temp[2] = {0};

	temp[0] = value >> 8;
	temp[1] = value & 0xff;

	ret = ft5x06_write_reg(client, FT5x06_REG_KX_H, temp[0]);
	if (ret < 0)
		return ret;

	ret = ft5x06_write_reg(client, FT5x06_REG_KX_L, temp[1]);
	if (ret < 0)
		return ret;

	return ret;
}

static int ft5x06_set_ky(struct i2c_client *client, unsigned short value)
{
	int ret = 0;
	unsigned char temp[2] = {0};

	temp[0] = value >> 8;
	temp[1] = value & 0xff;

	ret = ft5x06_write_reg(client, FT5x06_REG_KY_H, temp[0]);
	if (ret < 0)
		return ret;

	ret = ft5x06_write_reg(client, FT5x06_REG_KY_L, temp[1]);
	if (ret < 0)
		return ret;

	return ret;
}

static int ft5x06_set_lemda_x(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_LEMDA_X, value);
}

static int ft5x06_set_lemda_y(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_LEMDA_Y, value);
}

static int ft5x06_set_pos_x(struct i2c_client *client, unsigned char value)
{
	return ft5x06_write_reg(client, FT5x06_REG_DIRECTION, value);
}

static int ft5x06_set_other_param(struct i2c_client *client)
{
	int ret = 0;

	ret = ft5x06_write_reg(client, FT5x06_REG_THGROUP, ft5x06_hw_param.ft5x06_THGROUP >> 2);
	if (ret < 0)
	{
		dev_err(&client->dev, "write THGROUP failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_THPEAK, ft5x06_hw_param.ft5x06_THPEAK);
	if (ret < 0)
	{
		dev_err(&client->dev, "write THPEAK failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_THFALSE_TOUCH_PEAK, ft5x06_hw_param.ft5x06_THFALSE_TOUCH_PEAK);
	if (ret < 0)
	{
		dev_err(&client->dev, "write THFALSE_TOUCH_PEAK failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_THDIFF, ft5x06_hw_param.ft5x06_THDIFF >> 4);
	if (ret < 0)
	{
		dev_err(&client->dev, "write THDIFF failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_PWMODE_CTRL, ft5x06_hw_param.ft5x06_PWMODE_CTRL);
	if (ret < 0)
	{
		dev_err(&client->dev, "write PERIOD_CTRL failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_TIME_ENTER_MONITOR, ft5x06_hw_param.ft5x06_TIME_ENTER_MONITOR);
	if (ret < 0)
	{
		dev_err(&client->dev, "write TIME_ENTER_MONITOR failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_PERIOD_ACTIVE, ft5x06_hw_param.ft5x06_PERIOD_ACTIVE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write PERIOD_ACTIVE failed\n");
		return ret;
	}
	
	ret = ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_STATISTICS_TX_NUM, ft5x06_hw_param.ft5x06_FACE_DETECT_STATISTICS_TX_NUM);
	if (ret < 0)
	{
		dev_err(&client->dev, "write FACE_DETECT_STATISTICS_TX_NUM failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_PERIOD_MONITOR, ft5x06_hw_param.ft5x06_PERIOD_MONITOR);
	if (ret < 0)
	{
		dev_err(&client->dev, "write PERIOD_MONITOR failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_AUTO_CLB_MODE, ft5x06_hw_param.ft5x06_AUTO_CLB_MODE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write AUTO_CLB_MODE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MODE, ft5x06_hw_param.ft5x06_MODE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MODE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_PMODE, ft5x06_hw_param.ft5x06_PMODE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write PMODE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_FIRMWARE_ID, ft5x06_hw_param.ft5x06_FIRMWARE_ID);
	if (ret < 0)
	{
		dev_err(&client->dev, "write FIRMWARE_ID failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_STATE, ft5x06_hw_param.ft5x06_STATE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write STATE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MAX_TOUCH_VALUE_HIGH, ft5x06_hw_param.ft5x06_MAX_TOUCH_VALUE >> 8);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MAX_TOUCH_VALUE_HIGH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MAX_TOUCH_VALUE_LOW, ft5x06_hw_param.ft5x06_MAX_TOUCH_VALUE & 0xff);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MAX_TOUCH_VALUE_LOW failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_FACE_DETECT_MODE, ft5x06_hw_param.ft5x06_FACE_DETECT_MODE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write FACE_DETECT_MODE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_DRAW_LINE_TH, ft5x06_hw_param.ft5x06_DRAW_LINE_TH);
	if (ret < 0)
	{
		dev_err(&client->dev, "write DRAW_LINE_TH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_DIFFDATA_HADDLE_VALUE, ft5x06_hw_param.ft5x06_DIFFDATA_HADDLE_VALUE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write DIFFDATA_HADDLE_VALUE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ABNORMAL_DIFF_VALUE, ft5x06_hw_param.ft5x06_ABNORMAL_DIFF_VALUE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ABNORMAL_DIFF_VALUE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ABNORMAL_DIFF_NUM, ft5x06_hw_param.ft5x06_ABNORMAL_DIFF_NUM);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ABNORMAL_DIFF_NUM failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ABNORMAL_DIFF_LAST_FRAME, ft5x06_hw_param.ft5x06_ABNORMAL_DIFF_LAST_FRAME);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ABNORMAL_DIFF_LAST_FRAME failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_POINTS_SUPPORTED, ft5x06_hw_param.ft5x06_POINTS_SUPPORTED);
	if (ret < 0)
	{
		dev_err(&client->dev, "write POINTS_SUPPORTED failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_STATIC_TH, ft5x06_hw_param.ft5x06_STATIC_TH);
	if (ret < 0)
	{
		dev_err(&client->dev, "write STATIC_TH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MID_SPEED_TH, ft5x06_hw_param.ft5x06_MID_SPEED_TH);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MID_SPEED_TH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_HIGH_SPEED_TH, ft5x06_hw_param.ft5x06_HIGH_SPEED_TH);
	if (ret < 0)
	{
		dev_err(&client->dev, "write HIGH_SPEED_TH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_FILTER_FRAME_NOISE, ft5x06_hw_param.ft5x06_FILTER_FRAME_NOISE);
	if (ret < 0)
	{
		dev_err(&client->dev, "write FILTER_FRAME_NOISE failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_KX_LR_H, ft5x06_hw_param.ft5x06_KX_LR >> 8);
	if (ret < 0)
	{
		dev_err(&client->dev, "write REG_KX_LR_H failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_KX_LR_L, ft5x06_hw_param.ft5x06_KX_LR);
	if (ret < 0)
	{
		dev_err(&client->dev, "write REG_KX_LR_L failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_KY_UD_H, ft5x06_hw_param.ft5x06_KY_UD >> 8);
	if (ret < 0)
	{
		dev_err(&client->dev, "write CCOFFSET_X failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_KY_UD_L, ft5x06_hw_param.ft5x06_KY_UD);
	if (ret < 0)
	{
		dev_err(&client->dev, "write CCOFFSET_Y failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_POWERNOISE_FILTER_TH, ft5x06_hw_param.ft5x06_POWERNOISE_FILTER_TH);
	if (ret < 0)
	{
		dev_err(&client->dev, "write POWERNOISE_FILTER_TH failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ESD_FILTER_FRAME, ft5x06_hw_param.ft5x06_ESD_FILTER_FRAME);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ESD_FILTER_FRAME failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MOVSTH_I, ft5x06_hw_param.ft5x06_MOVSTH_I);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MOVSTH_I failed\n");
		return ret;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_MOVSTH_N, ft5x06_hw_param.ft5x06_MOVSTH_N);
	if (ret < 0)
	{
		dev_err(&client->dev, "write MOVSTH_N failed\n");
		return ret;
	}

	return ret;
}

static int ft5x06_touch_hw_init(struct i2c_client *client)
{
	int ret = 0;
	int i = 0;
	int count = 0;

	ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_FACTORYMODE_VALUE);
	if (ret < 0)
	{
		dev_err(&client->dev, "enter factory mode failed\n");
		goto enter_work;
	}

	count = sizeof(ft5x06_tx_order);
	for (i = 0; i < count; i++)
	{
		if (ft5x06_tx_order[i] != 0xFF)
		{
			ret = ft5x06_set_tx_order(client, i, ft5x06_tx_order[i]);
			if (ret < 0)
			{
				dev_err(&client->dev, "could not set tx%d order\n", i);
				goto enter_work;
			}
		}

		ret = ft5x06_set_tx_cap(client, i, ft5x06_tx_cap[i]);
		if (ret < 0)
		{
			dev_err(&client->dev, "could not set tx%d cap\n", i);
			goto enter_work;
		}
	}

	ret = ft5x06_set_tx_offset(client, 0, ft5x06_tx_offset);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set tx 0 offset\n");
		goto enter_work;
	}

	count = sizeof(ft5x06_rx_order);
	for (i = 0; i < count; i++)
	{
		ret = ft5x06_set_rx_order(client, i, ft5x06_rx_order[i]);
		if (ret < 0)
		{
			dev_err(&client->dev, "could not set rx%d order\n", i);
			goto enter_work;
		}

		ret = ft5x06_set_rx_cap(client, i, ft5x06_rx_cap[i]);
		if (ret < 0)
		{
			dev_err(&client->dev, "could not set rx%d cap\n", i);
			goto enter_work;
		}
	}

	count = count >> 2;
	for (i = 0; i < count; i++)
	{
		ret = ft5x06_set_rx_offset(client, i * 2, ft5x06_rx_offset[i] >> 4);
		if (ret < 0)
		{
			dev_err(&client->dev, "could not set rx order\n");
			goto enter_work;
		}

		ret = ft5x06_set_rx_offset(client, i * 2 + 1, ft5x06_rx_offset[i] & 0xff);
		if (ret < 0)
		{
			dev_err(&client->dev, "could not set rx cap\n");
			goto enter_work;
		}
	}
	
	ret = ft5x06_set_scan_select(client, ft5x06_scanselect);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set scan select\n");
		goto enter_work;
	}

	ret = ft5x06_set_scan_select(client, ft5x06_scanselect);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set scan select\n");
		goto enter_work;
	}

	count = sizeof(ft5x06_tx_order);
	ret = ft5x06_set_tx_num(client, count);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set tx num\n");
		goto enter_work;
	}

	count = sizeof(ft5x06_rx_order);
	ret = ft5x06_set_rx_num(client, count);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set rx num\n");
		goto enter_work;
	}

	ret = ft5x06_set_gain(client, ft5x06_gain);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set gain\n");
		goto enter_work;
	}

	ret = ft5x06_set_vol(client, ft5x06_voltage);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set voltage\n");
		goto enter_work;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ADC_TARGET_HIGH, ft5x06_hw_param.ft5x06_ADC_TARGET >> 8);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ADC_TARGET_HIGH failed\n");
		goto enter_work;
	}

	ret = ft5x06_write_reg(client, FT5x06_REG_ADC_TARGET_LOW, ft5x06_hw_param.ft5x06_ADC_TARGET & 0xff);
	if (ret < 0)
	{
		dev_err(&client->dev, "write ADC_TARGET_LOW failed\n");
		goto enter_work;
	}
	
enter_work:
	ret = ft5x06_write_reg(client, FT5x06_REG_DEVICE_MODE, FT5x06_WORKMODE_VALUE);
	if (ret < 0)
	{
		dev_err(&client->dev, "enter work mode failed\n");
		goto ret_exit;
	}

	ret = ft5x06_set_Resolution(client, ft5x06_hw_param.ft5x06_RESOLUTION_X, ft5x06_hw_param.ft5x06_RESOLUTION_Y);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set resolution\n");
		goto ret_exit;
	}

	ret = ft5x06_set_face_detect_pre_value(client, ft5x06_hw_param.ft5x06_FACE_DETECT_PRE_VALUE);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set face detect pre value\n");
		goto ret_exit;
	}

	ret = ft5x06_set_face_detect_num(client, ft5x06_hw_param.ft5x06_FACE_DETECT_NUM);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set face detect num\n");
		goto ret_exit;
	}

	ret = ft5x06_set_face_detect_last_time(client, ft5x06_hw_param.ft5x06_FACE_DETECT_LAST_TIME);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set face detect last time\n");
		goto ret_exit;
	}

	ret = ft5x06_set_peak_value_min(client, ft5x06_hw_param.ft5x06_BIGAREA_PEAK_VALUE_MIN);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set min peak value\n");
		goto ret_exit;
	}

	ret = ft5x06_set_diff_value_over_num(client, ft5x06_hw_param.ft5x06_BIGAREA_DIFF_VALUE_OVER_NUM);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set diff value over num\n");
		goto ret_exit;
	}

	ret = ft5x06_set_point_auto_clear_time(client, ft5x06_hw_param.ft5x06_BIGAREA_POINT_AUTO_CLEAR_TIME);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set point auto clear time\n");
		goto ret_exit;
	}

	ret = ft5x06_set_kx(client, ft5x06_hw_param.ft5x06_KX);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set kx\n");
		goto ret_exit;
	}

	ret = ft5x06_set_ky(client, ft5x06_hw_param.ft5x06_KY);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set ky\n");
		goto ret_exit;
	}

	ret = ft5x06_set_lemda_x(client, ft5x06_hw_param.ft5x06_LEMDA_X);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set lemda x\n");
		goto ret_exit;
	}
	
	ret = ft5x06_set_lemda_y(client, ft5x06_hw_param.ft5x06_LEMDA_Y);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set lemda y\n");
		goto ret_exit;
	}

	ret = ft5x06_set_pos_x(client, ft5x06_hw_param.ft5x06_DIRECTION);
	if (ret < 0)
	{
		dev_err(&client->dev, "could not set pos x\n");
		goto ret_exit;
	}
	
	ret = ft5x06_set_other_param(client);
	if (ret < 0)
	{
		dev_err(&client->dev, "ft5x06_set_other_param fail\n");
		goto ret_exit;
	}

	return 0;
ret_exit:
	return ret;
}
#endif

/*************************************************************/
/****************                  TP  Driver                       *****************/
/*************************************************************/
static int ft5x06_touch_recv_data(struct i2c_client *client)
{
	unsigned char databuff[IDX_PACKET_SIZE] = {0};
	int ret = 0, count = 0, i = 0, index = 0;
	int temp = 0;

	ret = ft5x06_i2c_read( client, databuff, IDX_PACKET_SIZE);
	if (ret < 0)
	{
		dev_err(&client->dev, "read_data i2c_rxdata failed\n");
		return ret;
	}

	count = databuff[2] & 0x07;
	if (count > MAX_CONTACTS)
		return -1;
	for (i = 0; i < count; i++)
	{
		index = (databuff[i * 6 + 5] & 0xf0) >> 4;
		if (index >= MAX_CONTACTS)
			continue;
		ft5x06_touch_point[index].status = (databuff[i * 6 + 3] & 0xc0) >> 6;
		ft5x06_touch_point[index].x = databuff[i * 6 + 3] & 0x07;
		ft5x06_touch_point[index].x <<= 8;
		ft5x06_touch_point[index].x |= databuff[i * 6 + 4];
		ft5x06_touch_point[index].y = databuff[i * 6 + 5] & 0x07;
		ft5x06_touch_point[index].y <<= 8;
		ft5x06_touch_point[index].y |= databuff[i * 6 + 6];
	//	printk("id: %d   x: %d   y: %d\n", index, ft5x06_touch_point[index].x, ft5x06_touch_point[index].y);




		

		// swap x,y
		temp = ft5x06_touch_point[index].x;
		ft5x06_touch_point[index].x = ft5x06_touch_point[index].y;
		ft5x06_touch_point[index].y = SCREEN_MAX_Y-temp;
/*
		// extend from 600x1024 to 1200x1920
		temp = ft5x06_touch_point[index].x * SCREEN_MAX_X / 600;
		ft5x06_touch_point[index].x = temp;
		temp = ft5x06_touch_point[index].y * SCREEN_MAX_Y / 1024;
		ft5x06_touch_point[index].y = temp;
*/
//		printk("fix data id: %d   x: %d   y: %d\n", index, ft5x06_touch_point[index].x, ft5x06_touch_point[index].y);
	}

	return ret;
}

static void ft5x0x_report_value(struct input_dev *input)
{
	int i = 0;
	
	for (i = 0; i < MAX_CONTACTS; i++)
	{
		if (ft5x06_touch_point[i].status == 0x02)
		{
			input_mt_slot(input, i);
			input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
			//input_report_abs(input, ABS_MT_TRACKING_ID, ft5x06_touch_point[i].id);
			input_report_abs(input, ABS_MT_TOUCH_MAJOR, 100);
			input_report_abs(input, ABS_MT_POSITION_X, ft5x06_touch_point[i].x);
			input_report_abs(input, ABS_MT_POSITION_Y, ft5x06_touch_point[i].y);
			input_report_abs(input, ABS_MT_WIDTH_MAJOR, 100);
			ft5x06_touch_point[i].status = 0x00;
		//	printk("x=%d, y=%d \n",ft5x06_touch_point[i].x,ft5x06_touch_point[i].y);
		}
		else
		{
			input_mt_slot(input, i);
			//input_report_abs(input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
		}
	}

	input_sync(input);
}

static void ft5x06_touch_work_func(struct work_struct *work)
{
	
	if (ft5x06_touch_recv_data(ft5x06_touch_data.client) > 0)
		ft5x0x_report_value(ft5x06_touch_data.input);
}

static irqreturn_t ft5x06_touch_ts_intretupt(int irq, void *dev_id)
{
	queue_work(ft5x06_touch_data.wq, &ft5x06_touch_data.work);

	return IRQ_HANDLED;
}

static void ft5x06_touch_timer_func(struct work_struct *work)
{
	queue_work(ft5x06_touch_data.wq, &ft5x06_touch_data.work);
	schedule_delayed_work(&ft5x06_touch_data.delayed_work, msecs_to_jiffies(100));
}

static int ft5x06_touch_regulator_enable(void)
{
	int ret = 0;
	
	ret = regulator_enable(ft5x06_touch_data.vdd_ts_en);
	if (ret == 0)
		dev_err(&ft5x06_touch_data.client->dev, "vdd_ts_en enable\n");

	return ret;
}

static int ft5x06_touch_regulator_disable(void)
{
	int ret = 0;

	ret = regulator_disable(ft5x06_touch_data.vdd_ts_en);
	if (ret == 0)
		dev_err(&ft5x06_touch_data.client->dev, "vdd_ts_en disable\n");

	return ret;
}


static int ft5x06_touch_suspend(struct i2c_client *client, pm_message_t mesg)
{
	if (ft5x06_touch_data.use_irq)
	{
		disable_irq_nosync(ft5x06_touch_data.irq);
	}
	else
	{
		cancel_delayed_work(&ft5x06_touch_data.delayed_work);
	}

	ft5x06_touch_regulator_disable();
	
	return 0;
}

static int ft5x06_touch_resume(struct i2c_client *client)
{
	ft5x06_touch_regulator_enable();
	
	if (ft5x06_touch_data.use_irq)
	{
		
		enable_irq(ft5x06_touch_data.irq);
	}
	else
	{
		schedule_delayed_work(&ft5x06_touch_data.delayed_work, msecs_to_jiffies(100));
	}
	return 0;
}

struct ft5x06_data *p_ft5x06_data;

void disable_ft5x06(void)
{
	if(p_ft5x06_data)
	{
		if (p_ft5x06_data->use_irq)
		{
			disable_irq_nosync(p_ft5x06_data->irq);
		}
		else
		{
			cancel_delayed_work(&p_ft5x06_data->delayed_work);
		}

		//ft5x06_touch_regulator_disable();
	}

	return 0;
}

void enable_ft5x06(void)
{
	if(p_ft5x06_data)
	{
		//ft5x06_touch_regulator_enable();

		if (p_ft5x06_data->use_irq)
		{

			enable_irq(p_ft5x06_data->irq);
		}
		else
		{
			schedule_delayed_work(&p_ft5x06_data->delayed_work, msecs_to_jiffies(100));
		}
	}
	return 0;
}


static int ft5x06_touch_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct edt_ft5x06_platform_data *pdata = client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		dev_err(&client->dev, "i2c_check_functionality\n");
		ret = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (pdata)
	{
		if (pdata->irq_pin > 0)
		{
			if (gpio_request(pdata->irq_pin, "ft5x06-irq"))
			{
				dev_err(&client->dev, "fail to request irq GPIO-%d", pdata->irq_pin);
				ft5x06_touch_data.irq_pin = -1;
			}
			else
			{
				gpio_direction_input(pdata->irq_pin);
				ft5x06_touch_data.irq_pin = pdata->irq_pin;
			}
		}

		if (pdata->reset_pin > 0)
		{
		/*
			if (gpio_request(pdata->reset_pin, "ft5x06-reset"))
			{
				dev_err(&client->dev, "fail to request GPIO-%d", pdata->reset_pin);
				ft5x06_touch_data.reset_pin = -1;
			}
			else*/
			{
				gpio_direction_output(pdata->reset_pin, 1);
				ft5x06_touch_data.reset_pin = pdata->reset_pin;
			
			}
		}
	}

	ft5x06_touch_data.client = client;
	strlcpy(client->name, FT5X06_TS_NAME, I2C_NAME_SIZE);
	
	ft5x06_touch_data.irq = client->irq;
	dev_err(&client->dev, "irq %d, gpio %d\n", ft5x06_touch_data.irq, ft5x06_touch_data.irq_pin);

	ft5x06_touch_data.vdd_ts_en = regulator_get(&client->dev, "fix_avdd_tp");
	if (unlikely(IS_ERR(ft5x06_touch_data.vdd_ts_en)))
	{
		dev_err(&client->dev, "vdd_ts_en regulator get failed\n");
		ft5x06_touch_data.vdd_ts_en = NULL;
		ret = PTR_ERR(ft5x06_touch_data.vdd_ts_en);
		goto exit_check_functionality_failed;
	}
	else
	{
		dev_err(&client->dev, "vdd_ts_en regulator get success\n");
	}

	if (ft5x06_touch_regulator_enable())
	{
		dev_err(&client->dev, "%s: regulator_enable fix_avdd_tp\n", __func__);
		goto fail_regulator_enable;
	}

	ft5x06_touch_data.input = input_allocate_device();
	if (ft5x06_touch_data.input == NULL)
	{
		dev_err(&client->dev, "input_allocate_device\n");
		ret = -ENOMEM;
		goto exit_input_dev_alloc_failed;
	}
	
	ft5x06_touch_data.input->name = FT5X06_TS_NAME;
	ft5x06_touch_data.input->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, ft5x06_touch_data.input->evbit);
	__set_bit(EV_KEY, ft5x06_touch_data.input->evbit);
	__set_bit(EV_REP,  ft5x06_touch_data.input->evbit);
	__set_bit(INPUT_PROP_DIRECT, ft5x06_touch_data.input->propbit);
	set_bit(ABS_MT_POSITION_X, ft5x06_touch_data.input->absbit);
	set_bit(ABS_MT_POSITION_Y, ft5x06_touch_data.input->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ft5x06_touch_data.input->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ft5x06_touch_data.input->absbit);

	input_mt_init_slots(ft5x06_touch_data.input, MAX_CONTACTS, 0);
	
	input_set_abs_params(ft5x06_touch_data.input,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ft5x06_touch_data.input,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(ft5x06_touch_data.input,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(ft5x06_touch_data.input,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	memset(ft5x06_touch_point, 0, sizeof(struct point_data) * MAX_CONTACTS);

	ret = input_register_device(ft5x06_touch_data.input);
	if (ret < 0)
	{
		dev_err(&client->dev, "input_register_device\n");
		goto exit_input_register_device_failed;
	}

	ft5x06_touch_data.wq = create_singlethread_workqueue("ft5x06_wq");
	if (!ft5x06_touch_data.wq)
	{
		dev_err(&client->dev, "create_singlethread_workqueue\n");
		ret = -ENOMEM;
		goto exit_create_singlethread;
	}
	
	INIT_WORK(&ft5x06_touch_data.work, ft5x06_touch_work_func);

	if (client->irq)
	{
		ft5x06_touch_data.use_irq = 1;
		ret = request_irq(client->irq, ft5x06_touch_ts_intretupt, IRQF_TRIGGER_FALLING, FT5X06_TS_NAME, &ft5x06_touch_data);
		if (ret < 0)
		{
			dev_err(&client->dev, "request_irq\n");
			ft5x06_touch_data.use_irq = 0;
		}
		
		//enable_irq(client->irq);
	}
	else
	{
		ft5x06_touch_data.use_irq = 0;
		INIT_DELAYED_WORK(&ft5x06_touch_data.delayed_work, ft5x06_touch_timer_func);
		schedule_delayed_work(&ft5x06_touch_data.delayed_work, msecs_to_jiffies(100));
	}


	p_ft5x06_data = &ft5x06_touch_data;
	dev_err(&client->dev, "ft5x06 ts starts in %s mode.\n", ft5x06_touch_data.use_irq == 1 ? "intretupt" : "polling");

	return 0;

exit_create_singlethread:
	input_unregister_device(ft5x06_touch_data.input);
exit_input_register_device_failed:
	input_free_device(ft5x06_touch_data.input);
exit_input_dev_alloc_failed:
	if (ft5x06_touch_data.irq_pin)
		gpio_free(ft5x06_touch_data.irq_pin);

	if (ft5x06_touch_data.reset_pin)
		gpio_free(ft5x06_touch_data.reset_pin);
	
	ft5x06_touch_regulator_disable();
	regulator_put(ft5x06_touch_data.vdd_ts_en);
	ft5x06_touch_data.vdd_ts_en = NULL;
fail_regulator_enable:
	regulator_put(ft5x06_touch_data.vdd_ts_en);
exit_check_functionality_failed:
	return ret;
}

static int ft5x06_touch_remove(struct i2c_client *client)
{
	if (ft5x06_touch_data.use_irq)
		free_irq(client->irq, client);
	else
		cancel_delayed_work_sync(&ft5x06_touch_data.delayed_work);
	
	if (ft5x06_touch_data.wq)
		destroy_workqueue(ft5x06_touch_data.wq);

	input_unregister_device(ft5x06_touch_data.input);
	input_free_device(ft5x06_touch_data.input);

	if (ft5x06_touch_data.irq_pin)
			gpio_free(ft5x06_touch_data.irq_pin);
	
	if (ft5x06_touch_data.reset_pin)
		gpio_free(ft5x06_touch_data.reset_pin);

	ft5x06_touch_regulator_disable();
	regulator_put(ft5x06_touch_data.vdd_ts_en);
	ft5x06_touch_data.vdd_ts_en = NULL;
	p_ft5x06_data = NULL;

	return 0;
}

static const struct i2c_device_id ft5x06_touch_id[] = {
	{FT5X06_TS_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ft5x06_touch_id);

static struct i2c_driver ft5x06_touch_driver = {
	.probe = ft5x06_touch_probe,
	.remove = ft5x06_touch_remove,
	.suspend = ft5x06_touch_suspend,
	.resume = ft5x06_touch_resume,
	.id_table = ft5x06_touch_id,
	.driver = {
		.name = FT5X06_TS_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ft5x06_touch_init(void)
{
	p_ft5x06_data = NULL;
	return i2c_add_driver(&ft5x06_touch_driver);
}

static void __exit ft5x06_touch_exit(void)
{
	i2c_del_driver(&ft5x06_touch_driver);
}

module_init(ft5x06_touch_init);
module_exit(ft5x06_touch_exit);

MODULE_AUTHOR("Baolin Liu <liubaolin@jxd-mobile.com>");
MODULE_DESCRIPTION("FT5302 Touch Screen driver");
MODULE_LICENSE("GPL");

