/* drivers/video/backlight/lm3533_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>

#include <board_lge.h>
#include <linux/earlysuspend.h>

#define MAX_LEVEL_lm3528			0x7F
#define MAX_LEVEL_lm3533			0xFF
//                                                       
//EJJ_ORG #define MIN_LEVEL 		0x0F
#define MIN_LEVEL 		0x04

//EJJ_E
#define DEFAULT_LEVEL	0x33

#define I2C_BL_NAME "lm3533"

#define BL_ON	1
#define BL_OFF	0

static struct i2c_client *lm3533_i2c_client;

struct backlight_platform_data {
   void (*platform_init)(void);
   int gpio;
   unsigned int mode;
   int max_current;
   int init_on_boot;
   int min_brightness;
//                                                  
   int default_brightness;
   int max_brightness;
   int dimming_brightness;
//                                                
};

struct lm3533_device {
   struct i2c_client *client;
   struct backlight_device *bl_dev;
   int gpio;
   int max_current;
   int min_brightness;
   int default_brightness;
   int max_brightness;
   int dimming_brightness;
   struct mutex bl_mutex;
};

static const struct i2c_device_id lm3533_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static int lm3533_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val);

static int cur_main_lcd_level;
static int saved_main_lcd_level;
static int cur_touchkey_level = 0xFF;


static int backlight_status = BL_OFF;
static struct lm3533_device *main_lm3533_dev = NULL;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;

static int is_early_suspended = false;
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW)
static int requested_in_early_suspend_lcd_level= 0;
#endif
#endif /* CONFIG_HAS_EARLYSUSPEND */
static void lm3533_hw_reset(void)
{
	int gpio = main_lm3533_dev->gpio;

	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(gpio, 1);
	mdelay(1);
}

static int lm3533_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	buf[0] = reg;
	buf[1] = val;

	if ((err = i2c_transfer(client->adapter, &msg, 1)) < 0) {
		dev_err(&client->dev, "i2c write error\n");
	}

	return 0;
}

extern int lge_bd_rev;
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW)
extern int usb_cable_info;
#endif
static void lm3533_set_main_current_level(struct i2c_client *client, int level)
{
	struct lm3533_device *dev;
	int cal_value = 0;
	int min_brightness 		= main_lm3533_dev->min_brightness;
	int max_brightness 		= main_lm3533_dev->max_brightness;
	//int dimming_brightness  = main_lm3533_dev->dimming_brightness;
	//int default_brightness  = main_lm3533_dev->default_brightness;

   dev = (struct lm3533_device *)i2c_get_clientdata(client);
	dev->bl_dev->props.brightness = cur_main_lcd_level = level;

	mutex_lock(&main_lm3533_dev->bl_mutex);
	if(level!= 0){
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
			if(lge_bd_rev >= LGE_REV_C) {
#else
			if(lge_bd_rev >= LGE_REV_B) {
#endif
			if(level >= 1 && level <= 255)
			{
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW)
				// make 50% UI brightness current to 5.38mA
				if( level>=129 && level<=131 ) {
					cal_value = 68;
				} else if( level<129 ) {
					cal_value = ((level-2)*640 + 4*1260)/1260;
				} else {
					cal_value = ((level-132)*1870 + 68*1230)/1230;
				}

				if((usb_cable_info==6) || (usb_cable_info==7) || (usb_cable_info==11))
				{
					if(cal_value > 1)
						cal_value = 1;
				}
#else
				// make 50% UI brightness current to 6.18mA
				//                                                     
				if( level>=129 && level<=131 ) {
					cal_value = 74;		// 78 -> 74
				} else if( level<129 ) {
					cal_value = ((level-2)*700 + 4*1260)/1260;	// 740 -> 700
				} else {
					cal_value = ((level-132)*1810 + 74*1230)/1230;	// 1770 -> 1810, 78 -> 74
				}
#endif
			}
			else{
				mdelay(1);
				return;
			}

			if (cal_value < MIN_LEVEL){
				cal_value = min_brightness;
				cal_value = MIN_LEVEL;
			}
			else if (cal_value > MAX_LEVEL_lm3533)
			{
				cal_value = max_brightness;
				cal_value = MAX_LEVEL_lm3533;
			}

			printk ("................... level=%d,cal_value1=%d\n",level,cal_value);

//                                               
			//printk ("ch.han==!!!!!cal_value=%d\n",cal_value),
			lm3533_write_reg(main_lm3533_dev->client, 0x40, cal_value);
		}
		else{
			cal_value =level;
			//printk ("ch.han=========level=%d,cal_value1=%d\n",level,cal_value);
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
			lm3533_write_reg(client, 0xA0, cal_value);
#else
			lm3533_write_reg(client, 0x40, cal_value);
#endif
		}
	}
	else{
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
		if(lge_bd_rev >= LGE_REV_C){
#else
		if(lge_bd_rev >= LGE_REV_A){
#endif
			printk ("................... level=%d,cal_value1=%d\n",level,cal_value);
			lm3533_write_reg(client, 0x27, 0x00); //Control Bank is disabled
		}
		else
		lm3533_write_reg(client, 0x10, 0x00);
	}

	mutex_unlock(&main_lm3533_dev->bl_mutex);
}

void lm3533_backlight_on(int level)
{

#if defined(CONFIG_HAS_EARLYSUSPEND)
		if(is_early_suspended)
		{
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW)	
			requested_in_early_suspend_lcd_level = level;
#endif
			return;
		}
#endif /* CONFIG_HAS_EARLYSUSPEND */

	if(backlight_status == BL_OFF){
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
		if(lge_bd_rev >= LGE_REV_C){
#else
		if(lge_bd_rev >= LGE_REV_B){
#endif
			lm3533_hw_reset();
			lm3533_write_reg(main_lm3533_dev->client, 0x10, 0x0); //HVLED 1 & 2 are controlled by Bank A
			//lm3533_write_reg(main_lm3533_dev->client, 0x14, 0x1); //PWM input is enabled
			lm3533_write_reg(main_lm3533_dev->client, 0x12, 0x2); // Shut down Transition Time control (524ms)
			lm3533_write_reg(main_lm3533_dev->client, 0x1A, 0x2); //Linear & Control Bank A is configured for register Current control
			lm3533_write_reg(main_lm3533_dev->client, 0x1F, 0x13); //Full-Scale Current (20.2mA)
			lm3533_write_reg(main_lm3533_dev->client, 0x27, 0x1); //Control Bank A is enable
			lm3533_write_reg(main_lm3533_dev->client, 0x2C, 0xA); //Active High, OVP(40V), Boost Frequency(1Mhz)
			msleep(10);
		}
		else{
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
			lm3533_hw_reset();
			lm3533_write_reg(main_lm3533_dev->client, 0x10, 0x5);
			msleep(10);
#else
			lm3533_hw_reset();
			lm3533_write_reg(main_lm3533_dev->client, 0x10, 0x0); //HVLED 1 & 2 are controlled by Bank A and LVLED 1,2,3 by Bank C
			lm3533_write_reg(main_lm3533_dev->client, 0x11, 0x0); //LVLED 4 controlled by Bacnk c
			// lm3533_write_reg(main_lm3533_dev->client, 0x14, 0x1); //PWM input is enabled
			lm3533_write_reg(main_lm3533_dev->client, 0x12, 0x2); // Shut down Transition Time control (524ms)
			lm3533_write_reg(main_lm3533_dev->client, 0x1A, 0x2); //Linear & Control Bank A is configured for register Current control
			lm3533_write_reg(main_lm3533_dev->client, 0x1B, 0x4); //Linear & Control Bank C is configured for register Current control
			lm3533_write_reg(main_lm3533_dev->client, 0x1F, 0x13); //Full-Scale Current (20.2mA) for Bacnk A
			lm3533_write_reg(main_lm3533_dev->client, 0x21, 0x0); //Full-Scale Current (5 mA) for Bank C
			lm3533_write_reg(main_lm3533_dev->client, 0x25, 0x3F); //ANODE Connect Register
			lm3533_write_reg(main_lm3533_dev->client, 0x26, 0x0); //Charge pump enable
			lm3533_write_reg(main_lm3533_dev->client, 0x27, 0x5); //Control Bank A & C is enable
			lm3533_write_reg(main_lm3533_dev->client, 0x2C, 0xA); //Active High, OVP(40V), Boost Frequency(1Mhz)
			lm3533_write_reg(main_lm3533_dev->client, 0x42, 0xFF); //Brightness Register for BANK B
			msleep(10);
#endif
		}
	}

	//printk("%s received (prev backlight_status: %s)\n", __func__, backlight_status?"ON":"OFF");
	lm3533_set_main_current_level(main_lm3533_dev->client, level);
	backlight_status = BL_ON;

	return;
}

void lm3533_backlight_off(void)
{
	int gpio = main_lm3533_dev->gpio;

	if (backlight_status == BL_OFF) return;
	saved_main_lcd_level = cur_main_lcd_level;
	lm3533_set_main_current_level(main_lm3533_dev->client, 0);
	lm3533_write_reg(main_lm3533_dev->client, 0x42, 0x0); //Key led off
	backlight_status = BL_OFF;

	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_direction_output(gpio, 0);
	msleep(6);
	return;
}

void lm3533_lcd_backlight_set_level( int level)
{
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	if(lge_bd_rev >= LGE_REV_C){
#else
	if(lge_bd_rev >= LGE_REV_A){
#endif
		if (level > MAX_LEVEL_lm3533)
			level = MAX_LEVEL_lm3533;
	}
	else{
		if (level > MAX_LEVEL_lm3528)
			level = MAX_LEVEL_lm3528;
	}

	if(lm3533_i2c_client!=NULL )
	{
		if(level == 0) {
			lm3533_backlight_off();
		} else {
			lm3533_backlight_on(level);
		}

		//printk("%s() : level is : %d\n", __func__, level);
	}else{
		printk("%s(): No client\n",__func__);
	}
}
EXPORT_SYMBOL(lm3533_lcd_backlight_set_level);

#if defined(CONFIG_HAS_EARLYSUSPEND)
void lm3533_early_suspend(struct early_suspend * h)
{
        is_early_suspended = true;

        pr_info("%s[Start] backlight_status: %d\n", __func__, backlight_status);
        if (backlight_status == BL_OFF)
                return;

        lm3533_lcd_backlight_set_level(0);
}

void lm3533_late_resume(struct early_suspend * h)
{
        is_early_suspended = false;

        pr_info("%s[Start] backlight_status: %d\n", __func__, backlight_status);
        if (backlight_status == BL_ON)
                return;
#if defined(CONFIG_MACH_LGE_325_BOARD_VZW)

        lm3533_lcd_backlight_set_level(requested_in_early_suspend_lcd_level);
#endif		
        return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int bl_set_intensity(struct backlight_device *bd)
{
	lm3533_lcd_backlight_set_level(bd->props.brightness);
	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
    return cur_main_lcd_level;
}

static ssize_t lcd_backlight_show_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n", cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int level;

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);
	lm3533_lcd_backlight_set_level(level);

	return count;
}

static int lm3533_bl_resume(struct i2c_client *client)
{
    lm3533_backlight_on(saved_main_lcd_level);

    return 0;
}

static int lm3533_bl_suspend(struct i2c_client *client, pm_message_t state)
{
    printk(KERN_INFO"%s: new state: %d\n",__func__, state.event);

    lm3533_lcd_backlight_set_level(saved_main_lcd_level);

    return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s received (prev backlight_status: %s)\n", __func__, backlight_status?"ON":"OFF");
	return 0;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	printk("%s received (prev backlight_status: %s)\n", __func__, backlight_status?"ON":"OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "%d",on_off);

	if(on_off == 1){
		lm3533_bl_resume(client);
	}else if(on_off == 0)
		lm3533_bl_suspend(client, PMSG_SUSPEND);

	return count;

}

static ssize_t touchkey_backlight_show_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;
	r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n", cur_touchkey_level);
	return r;
}

static ssize_t touchkey_backlight_store_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (!count)
		return -EINVAL;

	cur_touchkey_level = simple_strtoul(buf, NULL, 10);
	if (cur_touchkey_level >= 255)
		cur_touchkey_level = 255;
	lm3533_write_reg(main_lm3533_dev->client, 0x42, cur_touchkey_level); //Key led brightness control

	return count;

}

DEVICE_ATTR(lm3533_level, 0644, lcd_backlight_show_level, lcd_backlight_store_level);
DEVICE_ATTR(lm3533_backlight_on_off, 0644, lcd_backlight_show_on_off, lcd_backlight_store_on_off);
DEVICE_ATTR(lm3533_touchkeyled_control, 0664, touchkey_backlight_show_level, touchkey_backlight_store_level);


static struct backlight_ops lm3533_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int lm3533_probe(struct i2c_client *i2c_dev, const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct lm3533_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	printk(KERN_INFO"%s: i2c probe start\n", __func__);

	pdata = i2c_dev->dev.platform_data;
	lm3533_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct lm3533_device), GFP_KERNEL);
	if(dev == NULL) {
		dev_err(&i2c_dev->dev,"fail alloc for lm3533_device\n");
		return 0;
	}

	main_lm3533_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	if(lge_bd_rev >= LGE_REV_C){
#else
	if(lge_bd_rev >= LGE_REV_A){
#endif
		props.max_brightness = MAX_LEVEL_lm3533;	
		props.type = BACKLIGHT_PLATFORM;
		bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL, &lm3533_bl_ops, &props);
		bl_dev->props.max_brightness = MAX_LEVEL_lm3533;
		bl_dev->props.brightness = DEFAULT_LEVEL;
		bl_dev->props.power = FB_BLANK_UNBLANK;
	}
	else {
		props.max_brightness = MAX_LEVEL_lm3528;	
#if 1 /*                                               */
		props.type = BACKLIGHT_PLATFORM;
#endif
		bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL, &lm3533_bl_ops, &props);
		bl_dev->props.max_brightness = MAX_LEVEL_lm3528;
		bl_dev->props.brightness = DEFAULT_LEVEL;
		bl_dev->props.power = FB_BLANK_UNBLANK;
	}
	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;
	dev->gpio = pdata->gpio;
	dev->max_current = pdata->max_current;
	dev->min_brightness = pdata->min_brightness;
	//                                                  
	dev->default_brightness = pdata->default_brightness;
	dev->max_brightness = pdata->max_brightness;
	dev->dimming_brightness = pdata->dimming_brightness;
	//                                                
	i2c_set_clientdata(i2c_dev, dev);

	if(dev->gpio && gpio_request(dev->gpio, "lm3533 reset") != 0) {
		return -ENODEV;
	}
		mutex_init(&dev->bl_mutex);


	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3533_level);
	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3533_backlight_on_off);
	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3533_touchkeyled_control);

#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.suspend = lm3533_early_suspend;
	early_suspend.resume = lm3533_late_resume;
	register_early_suspend(&early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	return 0;
}

static int lm3533_remove(struct i2c_client *i2c_dev)
{
	struct lm3533_device *dev;
	int gpio = main_lm3533_dev->gpio;

	device_remove_file(&i2c_dev->dev, &dev_attr_lm3533_level);
	device_remove_file(&i2c_dev->dev, &dev_attr_lm3533_backlight_on_off);
	device_remove_file(&i2c_dev->dev, &dev_attr_lm3533_touchkeyled_control);
	dev = (struct lm3533_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);
	return 0;
}

static struct i2c_driver main_lm3533_driver = {
	.probe = lm3533_probe,
	.remove = lm3533_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = lm3533_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};


static int __init lcd_backlight_init(void)
{
	static int err=0;

	err = i2c_add_driver(&main_lm3533_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("LM3533 Backlight Control");
MODULE_AUTHOR("Jaeseong Gim <jaeseong.gim@lge.com>");
MODULE_LICENSE("GPL");
