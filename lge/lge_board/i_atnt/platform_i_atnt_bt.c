/* lge\lge_board\i_atnt\platform_i_atnt_bt.c
 * Copyright (C) 2009 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <asm/setup.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#if 0
#include <mach/board_lge.h>
#include "board-gelato.h"
#else
#include "board_i_atnt.h"
#include "board_lge.h"
#include "devices_i_atnt.h"
#include <mach/dma.h>
#include <mach/gpiomux.h>
//#include "gpiomux.h"
#include "gpiomux_i_atnt.h"
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_hs_lite.h>
#endif
#include <linux/delay.h>
#include <linux/rfkill.h>

/*                                  */
#define BT_RESET_N 138
enum {
	BT_HOST_WAKE = 127,
	BT_WAKE = 137,		
};


static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= BT_HOST_WAKE,
		.end	= BT_HOST_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= BT_WAKE,
		.end	= BT_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(BT_HOST_WAKE),
		.end	= MSM_GPIO_TO_INT(BT_HOST_WAKE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct bluesleep_platform_data bluesleep_data = {
	.bluetooth_port_num = 0,
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
	.dev = {
		.platform_data = &bluesleep_data,
	},	
};



#if (defined(CONFIG_MARIMBA_CORE)) && \
	(defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))

static int bluetooth_power(int);
static int lge_bluetooth_toggle_radio(void *data, bool state);

#if 0
static struct platform_device msm_bt_power_device = {
	.name	 = "bt_power",
	.id	 = -1,
	.dev	 = {
		.platform_data = &bluetooth_power,
	},
};
#else
static struct bluetooth_platform_data lge_bluetooth_data = {
	.bluetooth_power = bluetooth_power,
	.bluetooth_toggle_radio = lge_bluetooth_toggle_radio,
};

static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
	.id	 = -1,		
	.dev = {
		.platform_data = &lge_bluetooth_data,
	},		
};
#endif
#endif


#ifdef CONFIG_SERIAL_MSM_HS

static int configure_uart_gpios(int on)
{
	int ret = 0, i;
	int uart_gpios[] = {53, 54, 55, 56};
	for (i = 0; i < ARRAY_SIZE(uart_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(uart_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(uart_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(uart_gpios[i]);
	return ret;
}
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
       .gpio_config = configure_uart_gpios,
};
#endif


static unsigned bt_config_power_on[] = {
	GPIO_CFG(137, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(127, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* BT_RESET */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(137, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(127, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* BT_RESET */
}; 

static int configure_pcm_gpios(int on)
{
	int ret = 0, i;
	int pcm_gpios[] = {111, 112, 113, 114};
	for (i = 0; i < ARRAY_SIZE(pcm_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(pcm_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(pcm_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(pcm_gpios[i]);
	return ret;
}


static int lge_bluetooth_toggle_radio(void *data, bool state)
{
	int ret;
	int (*power_control)(int enable);

    power_control = ((struct bluetooth_platform_data *)data)->bluetooth_power;
	ret = (*power_control)((state == RFKILL_USER_STATE_SOFT_BLOCKED) ? 1 : 0);
	return ret;
}


static int bluetooth_power(int on)
{
  int ret, pin;

  if(on)
    {
 /*   	
      gpio_direction_output(BT_RESET_N,1);
      mdelay(100);
*/
      if(configure_uart_gpios(1))
        {
          printk(KERN_ERR"bluetooth_power on fail");
          return -EIO;
        }

      if(configure_pcm_gpios(1))
        {
          printk(KERN_ERR "bluetooth_power on fail");
          return -EIO;
        }

      for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++)
        {
          ret = gpio_tlmm_config(bt_config_power_on[pin],GPIO_CFG_ENABLE);
          if (ret) 
            {
              printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=fg%d\n",__func__, bt_config_power_on[pin], ret);
              return -EIO;
            }

		gpio_direction_output(BT_RESET_N, 0);
		mdelay(100);
		gpio_direction_output(BT_RESET_N, 1);
		mdelay(100);
        }
    }
  else
    {
      gpio_direction_output(BT_RESET_N,0);

      if(configure_uart_gpios(0))
        {
          printk(KERN_ERR"bluetooth_power on fail");
          return -EIO;
        }

      if(configure_pcm_gpios(0))
        {
          printk(KERN_ERR "bluetooth_power on fail");
          return -EIO;
        }

      for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++)
        {
          ret = gpio_tlmm_config(bt_config_power_off[pin],GPIO_CFG_ENABLE);
          if (ret) 
            {
              printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, bt_config_power_off[pin], ret);
              return -EIO;
            }
        }
    }
  return 0;
}

static void __init bt_power_init(void)
{
  int rc;
	
  printk(KERN_ERR"----------------mwKim bt Power Init ");  	 	
  rc = gpio_request(BT_RESET_N, "bt_reset_n");
  if (rc)
   {
     printk(KERN_ERR "%s: bt reset  %d request failed\n",__func__,BT_RESET_N );
     return;
   }
}


void __init lge_add_btpower_devices(void)
{
	bt_power_init();

#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(54); /* GSBI6(2) */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

	platform_device_register(&msm_device_uart_dm1);

	platform_device_register(&msm_bt_power_device);
	platform_device_register(&msm_bluesleep_device);
}

#if 0

#ifdef CONFIG_BT
static unsigned bt_config_power_on[] = {
	GPIO_CFG(BT_WAKE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(BT_RFR, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* RFR */
	GPIO_CFG(BT_CTS, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* CTS */
	GPIO_CFG(BT_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Rx */
	GPIO_CFG(BT_TX, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Tx */
	GPIO_CFG(BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(BT_PCM_DIN, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DIN */
	GPIO_CFG(BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(BT_PCM_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_CLK */
	GPIO_CFG(BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* RESET_N */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(BT_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(BT_RFR, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* RFR */
	GPIO_CFG(BT_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CTS */
	GPIO_CFG(BT_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Rx */
	GPIO_CFG(BT_TX, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Tx */
	GPIO_CFG(BT_PCM_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(BT_PCM_DIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DIN */
	GPIO_CFG(BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(BT_PCM_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_CLK */
	GPIO_CFG(BT_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* RESET_N */	
};

static int gelato_bluetooth_toggle_radio(void *data, bool state)
{
	int ret;
	int (*power_control)(int enable);

    power_control = ((struct bluetooth_platform_data *)data)->bluetooth_power;
	ret = (*power_control)((state == RFKILL_USER_STATE_SOFT_BLOCKED) ? 1 : 0);
	return ret;
}

static int gelato_bluetooth_power(int on)
{
	int pin, rc;
	
	printk(KERN_DEBUG "%s\n", __func__);
	printk( "%s %d\n", __func__, on);

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}
        //Turn Bluetooth Power On if and only if not turned on by WLAN yet.
/*                                                                 */
        if (!gpio_get_value(CONFIG_BCM4330_GPIO_WL_REGON)) //#23
		    gpio_set_value(CONFIG_BCM4330_GPIO_WL_REGON, 1); //#23
/*                                                                 */
		mdelay(100);
		gpio_set_value(BT_RESET_N, 0);
		mdelay(100);
		gpio_set_value(BT_RESET_N, 1);
		mdelay(100);

	} else {
        //Turn Bluetooth Power Off if and only if not used by WLAN anymore.
/*                                                                 */
        if (!gpio_get_value(CONFIG_BCM4330_GPIO_WL_RESET)) //#93
         gpio_set_value(CONFIG_BCM4330_GPIO_WL_REGON, 0); //#23
/*                                                                 */

		gpio_set_value(BT_RESET_N, 0);
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
	}
	return 0;
}

static struct bluetooth_platform_data gelato_bluetooth_data = {
	.bluetooth_power = gelato_bluetooth_power,
	.bluetooth_toggle_radio = gelato_bluetooth_toggle_radio,
};

static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
	.dev = {
		.platform_data = &gelato_bluetooth_data,
	},		
};

static void __init bt_power_init(void)
{
	/* TODO: should be fixed */
#if 0
	gpio_set_value(23, 1);
	ssleep(1); /* 1 sec */
	gpio_set_value(23, 0);
#endif
}
#else
#define bt_power_init(x) do {} while (0)
#endif

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= BT_HOST_WAKE,
		.end	= BT_HOST_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= BT_WAKE,
		.end	= BT_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(BT_HOST_WAKE),
		.end	= MSM_GPIO_TO_INT(BT_HOST_WAKE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct bluesleep_platform_data gelato_bluesleep_data = {
	.bluetooth_port_num = 0,
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
	.dev = {
		.platform_data = &gelato_bluesleep_data,
	},	
};

void __init lge_add_btpower_devices(void)
{
	bt_power_init();
#ifdef CONFIG_BT
	platform_device_register(&msm_bt_power_device);
#endif
	platform_device_register(&msm_bluesleep_device);
}
#endif
