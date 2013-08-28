#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <asm/io.h>

#include <mach/gpio.h>

#include "fc8101.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8101_regs.h"
#include "fc8101_isr.h"
#include "fci_hal.h"

ISDBT_INIT_INFO_T *hInit;
u32 totalTS=0;
u32 totalErrTS=0;
int overflow_cnt = 0; //taew00k.kang test for checking overflow continiously

//                                                                   
u32 remain_ts_len=0;
u8 remain_ts_buf[INT_THR_SIZE];	// remain buffer size is 16*188 defined in ["fc8101_regs.h"] taew00k.kang 2011-08-26
//                                                                 

#if 0
int isdbt_open (struct inode *inode, struct file *filp);
int isdbt_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int isdbt_release (struct inode *inode, struct file *filp);
ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
#endif

static wait_queue_head_t isdbt_isr_wait;
/* kernel malloc fix on bootup start*/
static ISDBT_OPEN_INFO_T *ghOpen;
/* kernel malloc fix on bootup end*/

#define RING_BUFFER_SIZE	(128 * 1024)  // kmalloc max 128k

//GPIO(RESET & INTRRUPT) Setting
#define FC8101_NAME		"isdbt"

#define GPIO_ISDBT_IRQ 33
#define GPIO_ISDBT_PWR_EN 102
#define GPIO_ISDBT_RST 101

static DEFINE_MUTEX(ringbuffer_lock);


void isdbt_hw_setting(void)
{
	gpio_request(GPIO_ISDBT_PWR_EN, "ISDBT_EN");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_PWR_EN, 1);

	gpio_request(GPIO_ISDBT_RST, "ISDBT_RST");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_RST, 1);

	gpio_request(GPIO_ISDBT_IRQ, "ISDBT_IRQ");
	gpio_direction_output(GPIO_ISDBT_IRQ, false);
}

//POWER_ON & HW_RESET & INTERRUPT_CLEAR
void isdbt_hw_init(void)
{
	PRINTF(0, "isdbt_hw_init \n");
	gpio_direction_input(GPIO_ISDBT_IRQ);
	
	gpio_set_value(GPIO_ISDBT_PWR_EN, 1);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST, 1);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST, 0);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_RST, 1);
}

//POWER_OFF
void isdbt_hw_deinit(void)
{
	gpio_set_value(GPIO_ISDBT_RST, 0);
	mdelay(1);
	gpio_set_value(GPIO_ISDBT_PWR_EN, 0);

	gpio_direction_output(GPIO_ISDBT_IRQ, false);
	gpio_set_value(GPIO_ISDBT_IRQ, 0);
}

//static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

static u8 isdbt_isr_sig=0;
static struct task_struct *isdbt_kthread = NULL;

static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	isdbt_isr_sig++;
	wake_up(&isdbt_isr_wait);
	return IRQ_HANDLED;
}

int data_callback(u32 hDevice, u8 *data, int len)
{
	ISDBT_INIT_INFO_T *hInit;
	struct list_head *temp;
	int i;
	
	//if((*data!=0x47)||((*(data+1)&0x80)==0x80))
	//	return 0;
	totalTS +=(len/188);

	for(i=0;i<len;i+=188)
	{
		if((data[i+1]&0x80)||data[i]!=0x47)
			totalErrTS++;
	}

	hInit = (ISDBT_INIT_INFO_T *)hDevice;

	list_for_each(temp, &(hInit->hHead))
	{
		ISDBT_OPEN_INFO_T *hOpen;

		hOpen = list_entry(temp, ISDBT_OPEN_INFO_T, hList);

		if(hOpen->isdbttype == TS_TYPE)
		{
			mutex_lock(&ringbuffer_lock);
			if(fci_ringbuffer_free(&hOpen->RingBuffer) < (len+2) ) 
			{
				mutex_unlock(&ringbuffer_lock);
				PRINTF(0, "fc8101 data_callback : ring buffer is full\n");
				return 0;
			}

			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer, len >> 8);
			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer, len & 0xff);

			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);

			wake_up_interruptible(&(hOpen->RingBuffer.queue));
			
			mutex_unlock(&ringbuffer_lock);
		}
	}

	return 0;
}

static int isdbt_thread(void *hDevice)
{
	static DEFINE_MUTEX(thread_lock);

	ISDBT_INIT_INFO_T *hInit = (ISDBT_INIT_INFO_T *)hDevice;
	
	set_user_nice(current, -20);// taew00k.kang 2011-08-30 set priority highest
	
	PRINTF(hInit, "isdbt_kthread enter\n");

	BBM_CALLBACK_REGISTER((u32)hInit, data_callback);

	init_waitqueue_head(&isdbt_isr_wait); //0929
	
	fc8101_buffer_init(1);
	
	while(1)
	{
		wait_event_interruptible(isdbt_isr_wait, isdbt_isr_sig || kthread_should_stop());
		
		//isdbt_isr_sig=0;
		
		BBM_ISR(hInit);

		if(isdbt_isr_sig>0)
		{
			isdbt_isr_sig--;
		}
	
		if (kthread_should_stop())
			break;
	}

	fc8101_buffer_init(0);

	BBM_CALLBACK_DEREGISTER(hInit);
	
	PRINTF(hInit, "isdbt_kthread exit\n");

	return 0;
}

int isdbt_open (struct inode *inode, struct file *filp)
{
	ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt open\n");
/* kernel malloc fix on bootup start*/
	hOpen = ghOpen;

	hOpen->isdbttype = 0;
	
	hOpen->hInit = (HANDLE *)hInit;

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);

	filp->private_data = hOpen;
/* kernel malloc fix on bootup end*/
	return 0;
}

 ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    s32 avail;
    s32 non_blocking = filp->f_flags & O_NONBLOCK;
    ISDBT_OPEN_INFO_T *hOpen = (ISDBT_OPEN_INFO_T*)filp->private_data;
    struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
    ssize_t len, total_len = 0;
 
    if (!cibuf->data || !count)
    {
        //PRINTF(hInit, " return 0\n");
        return 0;
    }
    
    if (non_blocking && (fci_ringbuffer_empty(cibuf)))
    {
        //PRINTF(hInit, "return EWOULDBLOCK\n");
        return -EWOULDBLOCK;
    }

	mutex_lock(&ringbuffer_lock);
	
//                                                                   
	if((remain_ts_len != 0) && (remain_ts_len <= INT_THR_SIZE))
	{
		if(count >= remain_ts_len)
		{
			if(copy_to_user(buf, remain_ts_buf, remain_ts_len))
			{
				PRINTF(hInit, "write to user buffer falied\n");
				mutex_unlock(&ringbuffer_lock);
				return 0;
			}

			total_len += remain_ts_len;
			PRINTF(hInit, "remian data copied %d !!!\n",remain_ts_len);
		}

		remain_ts_len = 0;
	}
//                                                                 

    while(1)
    {
        avail = fci_ringbuffer_avail(cibuf);
        
        if (avail < 4)
        {
            //PRINTF(hInit, "avail %d\n", avail);
            break;
        }
        
        len = FCI_RINGBUFFER_PEEK(cibuf, 0) << 8;
        len |= FCI_RINGBUFFER_PEEK(cibuf, 1);

		//                                                                   
		if ((avail < len + 2) || ((count-total_len) == 0))
		{
			PRINTF(hInit, "not enough data or user buffer avail %d, len %d, remain user buffer %d\n",avail, len, count-total_len);
			break;
		}

		if(count-total_len < len)
		{
			int read_size;

			read_size = count-total_len;
			remain_ts_len = len - read_size;			
			
			FCI_RINGBUFFER_SKIP(cibuf, 2); //skip 2 byte for length field
			
			total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, read_size);
			fci_ringbuffer_read(cibuf, remain_ts_buf, remain_ts_len);
			PRINTF(hInit, "user buf not enough, read %d , write %d , remain %d, avail %d\n",len, read_size, remain_ts_len, avail);
			PRINTF(hInit, "total_len %d, count %d\n",total_len, count);
			break;
		}
		//                                                                 
		
		FCI_RINGBUFFER_SKIP(cibuf, 2);

		total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, len);

	}
	mutex_unlock(&ringbuffer_lock);
	
	return total_len;
}
int isdbt_release (struct inode *inode, struct file *filp)
{
	ISDBT_OPEN_INFO_T *hOpen;

/* kernel malloc fix on bootup start*/
	hOpen = filp->private_data;

	hOpen->isdbttype = 0;
/* kernel malloc fix on bootup end*/
	return 0;
}

int fc8101_if_test(void)
{
	int res=0;
	int i;
	u16 wdata=0;
	u32 ldata=0;
	u8 data=0;
	
	for(i=0;i<3000;i++){
		bbm_ext_byte_write(0, 0xa0, i&0xff);
		bbm_ext_byte_read(0, 0xa0, &data);
		if((i&0xff)!=data){
			PRINTF(0, "fc8101_if_btest!   i=0x%x, data=0x%x\n", i&0xff, data);
		res=1;
			}
		}

	
	for(i=0;i<3000;i++){
		bbm_ext_word_write(0, 0xa0, i&0xffff);
		bbm_ext_word_read(0, 0xa0, &wdata);
		if((i&0xffff)!=wdata){
			PRINTF(0, "fc8101_if_wtest!   i=0x%x, data=0x%x\n", i&0xffff, wdata);
		res=1;
			}
		}

	for(i=0;i<10000;i++){
		bbm_ext_long_write(0, 0xa0, i&0xffffffff);
		data=0;
		bbm_ext_long_read(0, 0xa0, &ldata);
		if((i&0xffffffff)!=ldata){
			PRINTF(0, "fc8101_if_ltest!   i=0x%x, data=0x%x\n", i&0xffffffff, ldata);
		res=1;
			}
		}

	return res;
}

unsigned int tspc[8]=
{
  	1<<6,
	1<<8,
	1<<10,
	1<<12,
	1<<14,
	1<<16,
	1<<18,
	1<<20
};

static int isdbt_sw_lock_check(HANDLE hDevice)
{
	int res = BBM_NOK;
	unsigned char lock_data;

	res = BBM_READ(hDevice, BBM_STATEF, &lock_data);

	if(res)
		return res;

	if(lock_data == 0xA)
		res = BBM_OK;
	else
		res = BBM_NOK;

	return res;
}

static int isdbt_lock_check(HANDLE hDevice)
{
	int res = BBM_NOK;
	int i;
	unsigned char lock_data;
	unsigned char first_timeout = 10;
	unsigned char second_timeout = 70;

	for(i=0; i<first_timeout; i++)
	{
		res = BBM_READ(hDevice, BBM_STATEF, &lock_data);

		if(res)
			return res;

		if(lock_data >= 0x4)
		{
			res = BBM_OK;
			break;
		}


		msWait(10);
	}

	PRINTF(0, "  first lock %d\n", lock_data);
    
	if(i == first_timeout)
		return BBM_NOK;

	for(i=0; i<second_timeout; i++)
	{
		res = BBM_READ(hDevice, BBM_STATEF, &lock_data);
		
		if(res)
			return res;

		if(lock_data == 0xA)
		{
			res = BBM_OK;
			break;
		}
		msWait(10);
	}

	PRINTF(0, "  second lock %d\n", lock_data);
    
	if(i == second_timeout)
		return BBM_NOK;

	return res;
}


static int isdbt_cn_measure(HANDLE hDevice, u32 *fNumerator, u32 *fExpression, u32 *mod_mode)
{
	int res = BBM_NOK;
	u32 nReg_data;
	u16 CN_bit;
	u32 Mode_bit;
	
	res = BBM_BULK_READ(hDevice,BBM_ERRPWR1, (u8 *)&CN_bit, 2);
		
	if(res)
	{
		*fNumerator = 0;
		return res;
	}

	res = BBM_BULK_READ(hDevice,BBM_TMCRNT0, (u8 *)&Mode_bit, 3);

	if(res)
	{
		*fNumerator = 0;
		return res;
	}

	//if return data is LSByte first pattern ,  swap cn bit field
	nReg_data = ((CN_bit & 0xFF00)>>8 |(CN_bit & 0x3)<<8);      
	*mod_mode = ((Mode_bit & 0x7000)>>12);
	switch(*mod_mode)
	{
		case 1:     //QPSK
			*fNumerator = 7000 - 25 * nReg_data;
		break;

		case 2:     // 16QAM
			*fNumerator = 7000 - 110 * nReg_data;
		break;

		default:
			*fNumerator = 7000 - 25 * nReg_data;
		break;
	}

	if((nReg_data == 0) || (nReg_data == 1))
	{
		*fExpression=0;
		res = BBM_OK;
		return res;
	}
	
	*fExpression = (nReg_data - 1)*10;

	return BBM_OK;
}

int isdbt_ber_per_measure(HANDLE hDevice, u32 *ui32BER, u32 *ui32PER)
{
	int res = BBM_NOK;
	u32 Rscorbt = 0, Rsngtsp = 0, Tsp = 0;
	//u32 csBerData =0 , csPerData = 0;
	u32 ber_bit = 0;
	u16 per_bit = 0;

	res = BBM_BULK_READ(hDevice, BBM_RSECNT0, (u8 *)&ber_bit, 3);

	//if return data is LSByte first pattern ,  swap ber bit field	
	Rscorbt = ((ber_bit & 0xff)<<16)|(ber_bit & 0xff00)|((ber_bit & 0xff0000)>>16);      

	res |= BBM_BULK_READ(hDevice, BBM_RSTCNT0,(u8 *)&per_bit, 2);

	//if return data is LSByte first pattern ,  swap per bit field	
	Rsngtsp = (((per_bit & 0xff)<<8)|((per_bit & 0xff00)>>8));

	if(res)
	{
		*ui32BER = (u32)0xFFFF;
		*ui32PER = (u32)0xFFFF;

		PRINTF(hDevice, "[1011][No Reset]ISDBT MEASURE BER Read Error\n");
		return res;
	}

	Tsp = BER_PACKET_SIZE;

	// calculation BER
	if(Rsngtsp >= tspc[Tsp])
	{
		*ui32BER = (u32)0xFFFF;
		*ui32PER = (u32)0xFFFF;

		PRINTF(hDevice, "[1011][No Reset]ISDBT MEASURE SW No Reset\n");
		return BBM_NOK;
	}
	else
	{
		*ui32BER =(Rscorbt & 0xFFFFF)*10000/((tspc[Tsp]-Rsngtsp)*204*8/10);
		//csBerData =(float)((float)(Rscorbt & 0xFFFFF)/((float)tspc[Tsp]-(float)Rsngtsp)/204/8);
	}

	// calculation PER
	*ui32PER = (Rsngtsp*100000)/tspc[Tsp];

	PRINTF(hDevice, "ISDBT MEASURE BER : %d, PER : %d, cor : %d, ng : %d\n", *ui32BER, *ui32PER, Rscorbt, Rsngtsp);
	return res;
}

int isdbt_rssi_measure(HANDLE hDevice, s32 *i32RSSI)
{
	int res =BBM_NOK;
	u8 LNA, RFVGA, GSF, PGA;
	//s32 K=-32; // depends on model, rssi adjustment variable
	s32 K=-73;

	res = BBM_TUNER_READ(hDevice, RF_STATE_MONITOR1, 0x01, &LNA, 0x01);
	res |= BBM_TUNER_READ(hDevice, RF_STATE_MONITOR2, 0x01, &RFVGA, 0x01);
	res |= BBM_TUNER_READ(hDevice, RF_STATE_MONITOR3, 0x01, &GSF, 0x01);
	res |= BBM_TUNER_READ(hDevice, RF_STATE_MONITOR4, 0x01, &PGA, 0x01);

	if(res)
	{
		*i32RSSI = (s32)0xFFFF;
		return res;
	}

	//*i32RSSI = (LNA&0x07*6)+(RFVGA)+((GSF&0x07)*6)-(PGA/4)+K;
	*i32RSSI = (LNA*6)+(RFVGA)+((GSF&0x07)*6)-(PGA/4)+K;
	//PRINTF(hDevice, "ISDBT MEASURE RSSI : %d,  LNA : 0x%x, RFVGA : 0x%x, GSF : 0x%x, PGA : 0x%x\n", *i32RSSI, LNA, RFVGA, GSF, PGA);
	return res;
}

int isdbt_agc_measure(HANDLE hDevice, u16 *ui32AGCLvl)
{
	int res =BBM_NOK;
	u8 agc;
	
	res = BBM_READ(hDevice, BBM_AGADAT, &agc);

	if(res)
	{
		*ui32AGCLvl = (u16)0xFFFF;
		return BBM_NOK;
	}

	*ui32AGCLvl = (u16)agc;

	return BBM_OK;
}

long isdbt_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;
	s32 err = 0;
	s32 size = 0;
	int uData=0;
	ISDBT_OPEN_INFO_T *hOpen;
		
	IOCTL_ISDBT_SIGNAL_INFO isdbt_signal_info;
	
	if(_IOC_TYPE(cmd) != ISDBT_IOC_MAGIC)
	{
		return -EINVAL;
	}
	
	if(_IOC_NR(cmd) >= IOCTL_MAXNR)
	{
		return -EINVAL;
	}

	hOpen = filp->private_data;

	size = _IOC_SIZE(cmd);
	PRINTF(0, "isdbt_ioctl  0x%x\n", cmd);	

	switch(cmd)
	{
		case IOCTL_ISDBT_POWER_ON:
			PRINTF(0, "IOCTL_ISDBT_POWER_ON \n");	
			isdbt_hw_init();
			res = BBM_I2C_INIT(hInit, FCI_I2C_TYPE);
			res |= BBM_INIT(hInit);
			res |= BBM_TUNER_SELECT(hInit, FC8101_TUNER, 0);
			if(res)
			PRINTF(0, "IOCTL_ISDBT_POWER_ON FAIL \n");
				
			break;
		case IOCTL_ISDBT_POWER_OFF:
			PRINTF(0, "IOCTL_ISDBT_POWER_OFF \n");
			isdbt_hw_deinit();	
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_SCAN_FREQ:
			PRINTF(0, "IOCTL_ISDBT_SCAN_FREQ \n");	
			err = copy_from_user((void *)&uData, (void *)arg, size);
			res = BBM_TUNER_SET_FREQ(hInit, uData);
			res |= isdbt_lock_check(hInit);
			break;
		case IOCTL_ISDBT_SET_FREQ:
			PRINTF(0, "IOCTL_ISDBT_SET_FREQ \n");	
			totalTS=0;
			totalErrTS=0;
			remain_ts_len=0; //                                                            
			overflow_cnt = 0;

			err = copy_from_user((void *)&uData, (void *)arg, size);
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer); 	// FCI patch ring buffer init 2011-09-02
			mutex_unlock(&ringbuffer_lock);
			res = BBM_TUNER_SET_FREQ(hInit, uData);
			PRINTF(0, "chNum : %d \n", uData);
			//res |= isdbt_lock_check(hInit);
			break;
		case IOCTL_ISDBT_GET_LOCK_STATUS:
			PRINTF(0, "IOCTL_ISDBT_GET_LOCK_STATUS \n");	
			res = isdbt_sw_lock_check(hInit);
			if(res)
				uData=0;
			else
				uData=1;
			err |= copy_to_user((void *)arg, (void *)&uData, size);
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_GET_SIGNAL_INFO:

			if(isdbt_sw_lock_check(hInit))
			{
				isdbt_signal_info.lock=0;
			}
			else 
			{
				isdbt_signal_info.lock=1;
			}
			isdbt_cn_measure(hInit, &isdbt_signal_info.Num, &isdbt_signal_info.Exp, &isdbt_signal_info.mode);	
			isdbt_ber_per_measure(hInit, &isdbt_signal_info.ber, &isdbt_signal_info.per);
			isdbt_agc_measure(hInit, &isdbt_signal_info.agc);
			isdbt_rssi_measure(hInit, &isdbt_signal_info.rssi);
				
			fc8101_check_overflow(hInit);
			isdbt_signal_info.ErrTSP = totalErrTS;
			isdbt_signal_info.TotalTSP = totalTS;
			// when all sync byte is broken with good signal state, reset BB & buffer 2011-09-27 taew00k.kang
			/*
			if(totalErrTS == totalTS)
			{
				if(isdbt_signal_info.ber<=300)
				{
					// BB reset
					BBM_WRITE(0, BBM_SYSRST, 0x02);
					// ext buffer reset
					bbm_ext_write(hInit, BBM_COM_RESET, 0x01); 	//Ext buffer reset 
					bbm_ext_write(hInit, BBM_COM_RESET, 0x03);
					PRINTF(0, "[0927][Reset] All Sync byte is broken !!!!!!!!!!!!\n");
				}
			}
			*/
			
			totalTS=totalErrTS=0;

			err |= copy_to_user((void *)arg, (void *)&isdbt_signal_info, size);
			PRINTF(0, "[0926]LOCK :%d, BER: %d, PER : %d\n",isdbt_signal_info.lock, isdbt_signal_info.ber, isdbt_signal_info.per);
			
			res = BBM_OK;
			
			break;
		case IOCTL_ISDBT_START_TS:
			hOpen->isdbttype = TS_TYPE;
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_STOP_TS:
			hOpen->isdbttype = 0;
			res = BBM_OK;
			break;

		default:
			PRINTF(hInit, "isdbt ioctl error!\n");
			res = BBM_NOK;
			break;
	}
	
	if(err < 0)
	{
		PRINTF(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res; 
}

static struct file_operations isdbt_fops = 
{
	.owner				= THIS_MODULE,
	.unlocked_ioctl		= isdbt_ioctl,
	.open				= isdbt_open,
	.read				= isdbt_read,
	.release			= isdbt_release,
};

static struct miscdevice fc8101_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8101_NAME,
    .fops = &isdbt_fops,
};

int isdbt_init(void)
{
	s32 res;
	ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt_init\n");

	res = misc_register(&fc8101_misc_device);

	if(res < 0)
	{
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}

	isdbt_hw_setting();

	isdbt_hw_init();

	hInit = (ISDBT_INIT_INFO_T *)kmalloc(sizeof(ISDBT_INIT_INFO_T), GFP_KERNEL);

	res = BBM_HOSTIF_SELECT(hInit, BBM_SPI);
	
	if(res)
	{
		PRINTF(hInit, "isdbt host interface select fail!\n");
	}

	isdbt_hw_deinit();
	
#if 1
	res = request_irq(gpio_to_irq(GPIO_ISDBT_IRQ), isdbt_irq, IRQF_DISABLED | IRQF_TRIGGER_FALLING, FC8101_NAME, NULL);
	
	if(res) 
	{
		PRINTF(hInit, "dmb rquest irq fail : %d\n", res);
	}
#endif
	if (!isdbt_kthread)
	{
		
		PRINTF(hInit, "kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread, (void*)hInit, "isdbt_thread");
	}

	INIT_LIST_HEAD(&(hInit->hHead));

	/* kernel malloc fix on bootup start*/
	hOpen = (ISDBT_OPEN_INFO_T *)kmalloc(sizeof(ISDBT_OPEN_INFO_T), GFP_KERNEL);

	if(hOpen == NULL)
	{
		PRINTF(hInit, "ISDBT_OPEN_INFO_T malloc error\n");
		return -ENOMEM;
	}

	hOpen->buf = (u8 *)kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	if(hOpen->buf == NULL)
	{
		PRINTF(hInit, "ring buffer malloc error\n");
		return -ENOMEM;
	}

	ghOpen = hOpen;
	/* kernel malloc fix on bootup end*/
	return 0;
}

void isdbt_exit(void)
{
	PRINTF(hInit, "isdbt isdbt_exit \n");

	free_irq(GPIO_ISDBT_IRQ, NULL);
	
	kthread_stop(isdbt_kthread);
	isdbt_kthread = NULL;

	BBM_HOSTIF_DESELECT(hInit);

	isdbt_hw_deinit();
	
	misc_deregister(&fc8101_misc_device);

	kfree(hInit);

	/* kernel malloc fix on bootup start*/
	list_del(&(ghOpen->hList));
	
	kfree(ghOpen->buf);

	kfree(ghOpen);
	/* kernel malloc fix on bootup end*/
}

void fc8101_check_overflow(HANDLE hDevice)
{
	u8	extOverStatus = 0;
	u8	extIntStatus = 0;

	bbm_ext_read(hDevice, BBM_COM_OVERFLOW, &extOverStatus);
	
	if(extOverStatus) {
		overflow_cnt++;

		// if overflow is occured continuously
		if (!(overflow_cnt % 10)) {
			PRINTF(0, "[INT CLEAR]FC8101 Buffer Overflow %d tiems.isdbt_isr_sig : %d\n",overflow_cnt ,isdbt_isr_sig);
			//BBM_WRITE(0, BBM_SYSRST, 0x02);
			//bbm_ext_write(hDevice, BBM_COM_RESET, 0x01); 	//Ext buffer reset 
			//bbm_ext_write(hDevice, BBM_COM_RESET, 0x03);

			bbm_ext_read(hDevice, BBM_COM_INT_STATUS, &extIntStatus);
			bbm_ext_write(hDevice, BBM_COM_INT_STATUS, extIntStatus);
			bbm_ext_write(hDevice, BBM_COM_INT_STATUS, 0x00);

			//overflow_cnt=0;	
		}
		else
		{
			PRINTF(0, "[INT Not CLEAR]FC8101 Buffer Overflow %d time.isdbt_isr_sig : %d\n",overflow_cnt ,isdbt_isr_sig);
		}
	}
	else // if overflow is not occured continuously
	{
		overflow_cnt = 0;
	}
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");
