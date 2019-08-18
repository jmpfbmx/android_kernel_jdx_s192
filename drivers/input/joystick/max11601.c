#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include "../map.h"

#define MAX1160X_NAME			"max1160x"
#define MAX1160X_DELAY			msecs_to_jiffies(10)
#define MAX1160X_TIMEOUT			msecs_to_jiffies(1000)

#define MAX1160X_SETUP_RST			(0x00 << 1)
#define MAX1160X_SETUP_BIP			(0x01 << 2)
#define MAX1160X_SETUP_UNI			(0x00 << 2)
#define MAX1160X_SETUP_CLK_EXT		(0x01 << 3)
#define MAX1160X_SETUP_CLK_INT		(0x00 << 3)
#define MAX1160X_SETUP_SEL(x)		(x << 4)
#define MAX1160X_SETUP_REG			(0x01 << 7)

#define MAX1160X_CONFIG_SGL			(0x01 << 0)
#define MAX1160X_CONFIG_DIF			(0x00 << 0)
#define MAX1160X_CONFIG_CS(x)		(x << 1)
#define MAX1160X_CONFIG_SCAN(x)		(x << 5)
#define MAX1160X_CONFIG_REG			(0x00 << 7)

#define JOY_EDGE_OFFSET			0x30
#define JOY_MID_OFFSET			0x10

#define JOYSTICK_VDD_3V3_REF_VOL	0 //  1: alps  joystick 3.3V supply  and max11601 use vdd(3.3v) as reference voltage ; 0: alps joystick 1.8v supply and max11601 adc use 2.048v internal reference voltage

#define CENTER_VALUE			0X80
extern char setting_mode;
struct input_dev *global_input;
extern int key_init_flag;

static const unsigned int joy_report_value[4] = { ABS_Z, ABS_RZ,ABS_X, ABS_Y};

struct max1160x_config
{
};

struct max1160x_data
{
	struct i2c_client *client;
	struct input_dev *input;
	struct max1160x_config *pconfig;
	struct workqueue_struct *wq;
	struct delayed_work delayed_work;

	unsigned char sys_setup;
	unsigned char sys_config;
	int init_flag;
	struct regulator *avdd;
	int err_count;
};

unsigned char rawdata[4];
unsigned char rawdata2[4];
unsigned char rawdata3[4];

static ssize_t max1160x_raw_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d %d %d %d\n", rawdata[0], rawdata[1], rawdata[2],rawdata[3]);
}

static ssize_t max1160x_process_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d %d %d %d\n", rawdata2[0], rawdata2[1], rawdata2[2],rawdata2[3]);
}

static ssize_t max1160x_invert_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d %d %d %d\n", rawdata3[0], rawdata3[1], rawdata3[2],rawdata3[3]);
}

static DEVICE_ATTR(raw, S_IRUGO, max1160x_raw_show, NULL);
static DEVICE_ATTR(process, S_IRUGO, max1160x_process_show, NULL);
static DEVICE_ATTR(invert, S_IRUGO, max1160x_invert_show, NULL);

static int max1160x_i2c_read(struct i2c_client *client, char *databuff, int len)
	{
		int ret;

		struct i2c_msg msgs[] = {
			{
				.addr	= client->addr,
				.flags	= I2C_M_RD,
				.len	= len,
				.buf	= databuff,
			},
		};

		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "i2c read error: %d\n", ret);

		return ret;
}

static int max1160x_i2c_write(struct i2c_client *client, char *databuff, int len)
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

static int max1160x_hw_init(struct i2c_client *client)
{
	int ref_vol = 0;
	struct max1160x_data *pdata;
	pdata = i2c_get_clientdata(client);

#if	JOYSTICK_VDD_3V3_REF_VOL
	ref_vol = 0;
#else
	ref_vol = 5;
#endif

	pdata->sys_setup = MAX1160X_SETUP_REG | MAX1160X_SETUP_SEL(ref_vol) | MAX1160X_SETUP_CLK_EXT | MAX1160X_SETUP_UNI | MAX1160X_SETUP_RST;
	pdata->sys_config = MAX1160X_CONFIG_REG | MAX1160X_CONFIG_SCAN(0) | MAX1160X_CONFIG_CS(3) | MAX1160X_CONFIG_SGL;

	if (max1160x_i2c_write(client, &pdata->sys_setup, 1) < 0)
	{
		dev_err(&client->dev, "sys_setup error\n");
		return 1;
	}

	if (max1160x_i2c_write(client, &pdata->sys_config, 1) < 0)
	{
		dev_err(&client->dev, "sys_config error\n");
		return 1;
	}
	
	return 0;
}

static void max1160x_delayed_work(struct work_struct *work)
{
	struct max1160x_data *pdata;
	unsigned char udata[4] = {0};
	int temp = 0, i = 0;

	if (key_init_flag != 1)
		return;
	pdata = container_of(work, struct max1160x_data, delayed_work.work);
	if (pdata->init_flag)
	{
		pdata->init_flag = max1160x_hw_init(pdata->client);
	}

	if (max1160x_i2c_read(pdata->client, udata, 4) > 0)
	{
		pdata->err_count = 0;
		for (i = 3; i >= 0; i--)
		{
			rawdata[i] = temp = udata[i];

		#if	JOYSTICK_VDD_3V3_REF_VOL
			temp = temp & 0xFF;
		#else
			temp = (temp * 20 / 18) & 0xFF;
		#endif

		#if 0 //liao process
			if (temp < JOY_EDGE_OFFSET)// temp < 48
			{
				temp = 0x00;
			}
			else if ((temp >= JOY_EDGE_OFFSET) && (temp < (128 - JOY_MID_OFFSET)))//    48 <=  temp < 112
			{
				temp = (temp - JOY_EDGE_OFFSET) * 127 / (128 - JOY_EDGE_OFFSET - JOY_MID_OFFSET);//   (temp - 48)*127 / 64
			}
			else if ((temp >= (128 - JOY_MID_OFFSET)) && (temp < (128 + JOY_MID_OFFSET)))//  112 <= temp < 144
			{
				temp = CENTER_VALUE;  // temp = 128
			}
			else if ((temp >= (128 + JOY_MID_OFFSET)) && (temp < (255 - JOY_EDGE_OFFSET))) //  144 <=  temp  < 207
			{
				temp = (temp - 128 - JOY_MID_OFFSET) * 126 / (127 - JOY_EDGE_OFFSET - JOY_MID_OFFSET) + 128; //  (temp - 144) * 126 /63 + 128 
			}
			else if (temp >= (255 - JOY_EDGE_OFFSET))// temp >= 207
			{
				temp = 255; // temp = 255
			}

		#else


			if (temp < JOY_EDGE_OFFSET)// temp < 48
			{
				temp = 0x00;
			}
			else if ((temp >= JOY_EDGE_OFFSET) && (temp < (128 - JOY_MID_OFFSET)))//	48 <=  temp <= 112
			{
				temp = (temp - JOY_EDGE_OFFSET - 5) * 127 / (128 - JOY_EDGE_OFFSET - JOY_MID_OFFSET);//   (temp - 53)*127 / 64
				if(temp < 0)
					temp = 0;
			}
			else if ((temp >= (128 - JOY_MID_OFFSET)) && (temp < (128 + JOY_MID_OFFSET)))//  112 <= temp < 144
			{
				temp = CENTER_VALUE;  // temp = 128
			}
			else if ((temp >= (128 + JOY_MID_OFFSET)) && (temp < (255 - JOY_EDGE_OFFSET))) //  144 <=  temp  < 207
			{
				temp = (temp - 128 - JOY_MID_OFFSET) * 126 / (127 - JOY_EDGE_OFFSET - JOY_MID_OFFSET - 5) + 128; //  (temp - 144) * 126 /58 + 128 
				if(temp >= 255 )
					temp = 255;
			}
			else if (temp >= (255 - JOY_EDGE_OFFSET))// temp >= 207
			{
				temp = 255; // temp = 255
			}

		#endif

			rawdata2[i] = temp;

			if ((i == 3 || i == 2 || i == 1) && temp!=CENTER_VALUE)
				temp = 255 - temp;

			rawdata3[i] = temp;
		//	printk(".. value= %d ",temp);
			//input_report_abs(pdata->input, joy_report_value[i], temp & 0xFF);
			process_map(pdata->input,EV_ABS,joy_report_value[i], temp & 0xFF);
		}
	}
	else
	{
		printk("max1160: read error %d \n",pdata->err_count);
		pdata->err_count++;
	}
	
//	input_sync(pdata->input);
	if (setting_mode != SETTING)              // if in gamap setting , event sync sent in mapping function()
		process_sync(pdata->input);


	schedule_delayed_work(&pdata->delayed_work, (pdata->err_count < 30)?MAX1160X_DELAY:MAX1160X_TIMEOUT);


	
}


struct max1160x_data *global_data;

void suspend_max1160(void)
{
	if (global_data){
		__cancel_delayed_work(&global_data->delayed_work);
		if (global_data->avdd)
			regulator_disable(global_data->avdd);
	}
	return 0;
}

void resume_max1160(void)
{
	if (global_data){
		if (global_data->avdd)
			regulator_enable(global_data->avdd);
		global_data->init_flag = max1160x_hw_init(global_data->client);
		global_data->err_count = 0;
		schedule_delayed_work(&global_data->delayed_work, MAX1160X_DELAY);
	}
	return 0;
}



static struct max1160x_config *max1160x_parse_dt(struct i2c_client *client)
{
	struct max1160x_config *pconfig;
//	struct device_node *np = client->dev.of_node;

	pconfig = devm_kzalloc(&client->dev, sizeof(struct max1160x_config), GFP_KERNEL);
	if (!pconfig)
	{
		dev_err(&client->dev, "Can't allocate platform data\n");
		return ERR_PTR(-ENOMEM);
	}

	return pconfig;
#if 0
	char const *pchar;
	u8 config;
	int len;

	pchar = of_get_property(np, "orientation", &len);
	if (!pchar || len != sizeof(pdata->orientation)) {
		dev_err(&client->dev, "Cannot read orientation property\n");
		return ERR_PTR(-EINVAL);
	}

	memcpy(pdata->orientation, pchar, len);
	if (of_property_read_string(np, "config", &pchar)) {
		dev_err(&client->dev, "Cannot read config property\n");
		return ERR_PTR(-EINVAL);
	}
#endif
}



static int max1160x_probe(struct i2c_client *client, const struct i2c_device_id *devid)
{
	int err = 0;
	struct max1160x_data *pdata;
	struct max1160x_config *pconfig;

	dev_info(&client->dev, "%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		dev_err(&client->dev, "%s: i2c_check_functionality\n", __func__);
		err = -ENODEV;
		goto err_exit;
	}

	pdata = (struct max1160x_data *)devm_kzalloc(&client->dev, sizeof(struct max1160x_data), GFP_KERNEL);
	if (!pdata)
	{
		dev_err(&client->dev, "%s kzalloc ERR\n", __func__);
		err = -ENOMEM;
		goto err_exit;
	}
	global_data = pdata;

	pdata->avdd = regulator_get(&client->dev, "adc_avdd");
	if (unlikely(IS_ERR(pdata->avdd)))
	{
		dev_err(&client->dev, "%s adc_avdd ERR: %d\n", __func__, (int)pdata->avdd);
		err = PTR_ERR(pdata->avdd);
		goto fail_regulator_get;
	}
	else
		dev_err(&client->dev, "%s: regulator_get adc_avdd\n", __func__);

	err = regulator_enable(pdata->avdd);
	if (err)
	{
		dev_err(&client->dev, "%s: regulator_enable adc_avdd\n", __func__);
		goto fail_regulator_enable;
	}
	
	pdata->client = client;
	i2c_set_clientdata(client, pdata);
	if (client->dev.of_node)
	{
		pconfig = max1160x_parse_dt(client);
		if (IS_ERR(pconfig))
		{
			dev_err(&client->dev, "%s max1160x_parse_dt\n", __func__);
			err = -EINVAL;
			goto err_get_platdata;
		}
	}
	else
	{
		pconfig = (struct max1160x_config *)dev_get_platdata(&client->dev);
		if (!pconfig)
		{
			dev_err(&client->dev, "%s dev_get_platdata\n", __func__);
			err = -EINVAL;
			goto err_get_platdata;
		}
	}

	pdata->pconfig = pconfig;
	pdata->init_flag = 1;
	pdata->err_count = 0;

	pdata->input = input_allocate_device();
	if (!pdata->input)
	{
		dev_err(&client->dev, "%s input_allocate_device\n", __func__);
		err = -ENOMEM;
		goto err_input_alloc;
	}
	set_bit(EV_KEY, pdata->input->evbit);
	set_bit(EV_ABS, pdata->input->evbit);
	set_bit(BTN_JOYSTICK, pdata->input->keybit);

	input_set_abs_params(pdata->input, ABS_X, 0, 255, 0, 0);
	input_set_abs_params(pdata->input, ABS_Y, 0, 255, 0, 0);
	input_set_abs_params(pdata->input, ABS_Z, 0, 255, 0, 0);
	input_set_abs_params(pdata->input, ABS_RZ, 0, 255, 0, 0);
	
	INIT_DELAYED_WORK(&pdata->delayed_work, max1160x_delayed_work);
	global_input= pdata->input;
	schedule_delayed_work(&pdata->delayed_work, msecs_to_jiffies(15000));

	err = device_create_file(&client->dev, &dev_attr_raw);
	if(err < 0)
	{
		printk(KERN_ALERT"Failed to create attribute raw.");
	}

	err = device_create_file(&client->dev, &dev_attr_process);
	if(err < 0)
	{
		printk(KERN_ALERT"Failed to create attribute process.");
	}

	err = device_create_file(&client->dev, &dev_attr_invert);
	if(err < 0)
	{
		printk(KERN_ALERT"Failed to create attribute invert.");
	}

	return 0;

exit_create_singlethread:
	input_unregister_device(pdata->input);
err_input_register:
	input_free_device(pdata->input);
err_input_alloc:
	if (client->dev.of_node)
		devm_kfree(&client->dev, pdata->pconfig);
err_get_platdata:
	regulator_disable(pdata->avdd);
fail_regulator_enable:
	regulator_put(pdata->avdd);
fail_regulator_get:
	devm_kfree(&client->dev, pdata);
	global_data = NULL;
err_exit:
	return err;
}

static int max1160x_remove(struct i2c_client *client)
{
	struct max1160x_data *pdata;
	pdata = i2c_get_clientdata(client);

	if (pdata)
	{
		cancel_delayed_work(&pdata->delayed_work);


		devm_kfree(&client->dev, pdata);
		pdata = NULL;
	}

	return 0;
}

static void max1160x_shutdown(struct i2c_client *client)
{
	struct max1160x_data *pdata;
	pdata = i2c_get_clientdata(client);
	if (pdata)
	{

		__cancel_delayed_work(&pdata->delayed_work);
		max1160x_remove(client);
		global_data = NULL;

	}
}

static const struct i2c_device_id max1160x_i2c_device_id[] = {
	{MAX1160X_NAME, 0},
	{"max11601", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, max1160x_i2c_device_id);

static const struct of_device_id max1160x_of_match[] = {
	{ .compatible = "maxim,max11601", },
	{ },
};

MODULE_DEVICE_TABLE(of, max1160x_of_match);

static struct i2c_driver max1160x_driver = {
	.class		= I2C_CLASS_HWMON,
	.probe		= max1160x_probe,
	.remove		= max1160x_remove,
	.driver = {
		.name		= MAX1160X_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(max1160x_of_match),
	},
	.id_table	= max1160x_i2c_device_id,
	.shutdown	= max1160x_shutdown,

};


static int __init max1160x_init(void)
{
	global_data = NULL;

	return i2c_add_driver(&max1160x_driver);
}

static void __exit max1160x_exit(void)
{
	i2c_del_driver(&max1160x_driver);
}

module_init(max1160x_init);
module_exit(max1160x_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AKM driver");

