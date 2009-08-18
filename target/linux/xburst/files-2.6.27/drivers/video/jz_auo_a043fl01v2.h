/*
 * linux/drivers/video/jz_auo_a043fl01v2.h -- Ingenic LCD driver
 */

#ifndef __JZ_AUO_A043FL01V2_H__
#define __JZ_AUO_A043FL01V2_H__

#if defined(CONFIG_JZ4750_APUS) /* board pavo */
	#define SPEN		(32*3+29)       /*LCD_CS*/
	#define SPCK		(32*3+26)       /*LCD_SCL*/
	#define SPDA		(32*3+27)       /*LCD_SDA*/
	#define LCD_DISP_N 	(32*4+25)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4750_FUWA) /* board fuwa */
	#define SPEN		(32*3+29)       /*LCD_CS*/
	#define SPCK		(32*3+26)       /*LCD_SCL*/
	#define SPDA		(32*3+27)       /*LCD_SDA*/
	#define LCD_DISP_N 	(32*5+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4750D_CETUS) /* board cetus */
	#define SPEN		(32*5+13)       /*LCD_CS*/
	#define SPCK		(32*5+10)       /*LCD_SCL*/
	#define SPDA		(32*5+11)       /*LCD_SDA*/
	#define LCD_DISP_N 	(32*4+18)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/Jzlcd.h, please define SPI pins on your board."
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
		udelay(50);			\
		__gpio_clear_pin(SPDA);		\
		__gpio_clear_pin(SPEN);		\
		udelay(50);			\
		value=((a<<8)|(b&0xFF));	\
		for(no=0;no<16;no++)		\
		{				\
			if((value&0x8000)==0x8000){	      \
				__gpio_set_pin(SPDA);}	      \
			else{				      \
				__gpio_clear_pin(SPDA); }     \
			udelay(50);			\
			__gpio_set_pin(SPCK);		\
			value=(value<<1);		\
			udelay(50);			\
			__gpio_clear_pin(SPCK);		\
		}					\
		__gpio_set_pin(SPEN);			\
		udelay(400);				\
	} while (0)
#define __spi_read_reg(reg,val)			\
	do{					\
		unsigned char no;		\
		unsigned short value;			\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		value = ((reg << 0) | (1 << 7));	\
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
		__gpio_as_output(LCD_DISP_N);		\
		__gpio_clear_pin(LCD_DISP_N);	     \
	} while (0)
#define __lcd_special_on()			     \
	do {					     \
		udelay(50);\
		__gpio_clear_pin(LCD_DISP_N);	     \
		udelay(100);			     \
		__gpio_set_pin(LCD_DISP_N);	     \
} while (0)

	#define __lcd_special_off() \
	do { \
		__gpio_clear_pin(LCD_DISP_N);		\
	} while (0)

#endif	/* __JZ_AUO_A043FL01V2_H__ */

