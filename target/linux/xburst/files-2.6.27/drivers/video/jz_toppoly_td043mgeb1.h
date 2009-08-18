#ifndef __JZ_TOPPOLY_TD043MGEB1_H__
#define __JZ_TOPPOLY_TD043MGEB1_H__

#include <asm/jzsoc.h>

#if defined(CONFIG_JZ4750_LCD_TOPPOLY_TD043MGEB1) || defined(CONFIG_JZ4750_ANDROID_LCD_TOPPOLY_TD043MGEB1)
#if defined(CONFIG_JZ4750_APUS) /* board FuWa */
	#define SPEN		(32*3+29)       /*LCD_CS*/
	#define SPCK		(32*3+26)       /*LCD_SCL*/
	#define SPDA		(32*3+27)       /*LCD_SDA*/
	#define LCD_RET 	(32*4+23)       /*LCD_DISP_N use for lcd reset*/
	#define LCD_STBY 	(32*4+25)       /*LCD_STBY, use for lcd standby*/
#else
#error "driver/video/jz_toppoly_td043mgeb1.h, please define SPI pins on your board."
#endif

#define __spi_write_reg(reg, val)		\
	do {					\
		unsigned char no;		\
		unsigned short value;		\
		unsigned char a=0;		\
		unsigned char b=0;		\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		a=reg;				\
		b=val;				\
		__gpio_set_pin(SPEN);		\
		__gpio_clear_pin(SPCK);		\
		udelay(500);			\
		__gpio_clear_pin(SPDA);		\
		__gpio_clear_pin(SPEN);		\
		udelay(500);			\
		value=((a<<10)|(b&0xFF));	\
		for(no=0;no<16;no++)		\
		{				\
			if((value&0x8000)==0x8000){	      \
				__gpio_set_pin(SPDA);}	      \
			else{				      \
				__gpio_clear_pin(SPDA); }     \
			udelay(500);			\
			__gpio_set_pin(SPCK);		\
			value=(value<<1);		\
			udelay(500);			\
			__gpio_clear_pin(SPCK);		\
		}					\
		__gpio_set_pin(SPEN);			\
		udelay(4000);				\
	} while (0)
#define __spi_read_reg(reg,val)			\
	do{					\
		unsigned char no;		\
		unsigned short value;			\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		value = ((reg << 2) | (1 << 1));	\
		val = 0;				\
		__gpio_as_output(SPDA);			\
		__gpio_set_pin(SPEN);			\
		__gpio_clear_pin(SPCK);			\
		udelay(50);				\
		__gpio_clear_pin(SPDA);			\
		__gpio_clear_pin(SPEN);			\
		udelay(50);				\
		for (no = 0; no < 16; no++ ) {		\
			udelay(50);			\
			if(no < 8)			\
			{						\
				if (value & 0x80) /* send data */	\
					__gpio_set_pin(SPDA);		\
				else					\
					__gpio_clear_pin(SPDA);		\
				udelay(50);				\
				__gpio_set_pin(SPCK);			\
				value = (value << 1);			\
				udelay(50);				\
				__gpio_clear_pin(SPCK);			\
				if(no == 7)				\
					__gpio_as_input(SPDA);		\
			}						\
			else						\
			{						\
				udelay(100);				\
				__gpio_set_pin(SPCK);			\
				udelay(50);				\
				val = (val << 1);			\
				val |= __gpio_get_pin(SPDA);		\
				__gpio_clear_pin(SPCK);			\
			}						\
		}							\
		__gpio_as_output(SPDA);					\
		__gpio_set_pin(SPEN);					\
		udelay(400);						\
	} while(0)
	
#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		__gpio_as_output(LCD_STBY);		\
		__gpio_as_output(LCD_RET);		\
		udelay(500);			\
		__gpio_clear_pin(LCD_RET);	\
		udelay(1000);			\
		__gpio_set_pin(LCD_RET);	\
		udelay(1000);			\
	} while (0)

#define __lcd_special_on()			\
	do {					\
		__gpio_set_pin(LCD_STBY);	\
		udelay(1000);			\
		__spi_write_reg(0x02, 0x07);	\
		__spi_write_reg(0x03, 0x5F);	\
		__spi_write_reg(0x04, 0x17);	\
		__spi_write_reg(0x05, 0x20);	\
		__spi_write_reg(0x06, 0x08);	\
		__spi_write_reg(0x07, 0x20);	\
		__spi_write_reg(0x08, 0x20);	\
		__spi_write_reg(0x09, 0x20);	\
		__spi_write_reg(0x0A, 0x20);	\
		__spi_write_reg(0x0B, 0x20);	\
		__spi_write_reg(0x0C, 0x20);	\
		__spi_write_reg(0x0D, 0x22);	\
		__spi_write_reg(0x0E, 0x2F);	\
		__spi_write_reg(0x0F, 0x2f);	\
		__spi_write_reg(0x10, 0x2F);	\
		__spi_write_reg(0x11, 0x15);	\
		__spi_write_reg(0x12, 0xaa);	\
		__spi_write_reg(0x13, 0xFF);	\
		__spi_write_reg(0x14, 0x86);	\
		__spi_write_reg(0x15, 0x8e);	\
		__spi_write_reg(0x16, 0xd6);	\
		__spi_write_reg(0x17, 0xfe);	\
		__spi_write_reg(0x18, 0x28);	\
		__spi_write_reg(0x19, 0x52);	\
		__spi_write_reg(0x1A, 0x7c);	\
		__spi_write_reg(0x1B, 0xe9);	\
		__spi_write_reg(0x1C, 0x42);	\
		__spi_write_reg(0x1D, 0x88);	\
		__spi_write_reg(0x1E, 0xb8);	\
		__spi_write_reg(0x1F, 0xff);	\
		__spi_write_reg(0x20, 0xf0);	\
		__spi_write_reg(0x21, 0xf0);	\
		__spi_write_reg(0x22, 0x08);	\
	} while (0)

#define __lcd_special_off() \
	do {					\
		__gpio_clear_pin(LCD_STBY);	\
	} while (0)

#endif	/* LCD_TOPPOLY_TD043MGEB1 */

#endif  /* __JZ_TOPPOLY_TD043MGEB1_H__ */
