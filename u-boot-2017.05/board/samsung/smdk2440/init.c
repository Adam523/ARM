/* NAND FLASH control register */
#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))

void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len);

static int isBootFromNorFlash(void)
{
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if (*p == 0x12345678)
	{
		/* if ture, system booted from NAND Flash */
		*p = val;
		return 0;
	}
	else
	{
		/* system booted from NOR Flash */
		return 1;
	}
}

void copy_code_to_sdram(unsigned char *src, unsigned char *dest, unsigned int len)
{	
	int i = 0;
	
	/* From NOR Flash */
	if (isBootFromNorFlash())
	{
		while (i < len)
		{
			dest[i] = src[i];
			i++;
		}
	}
	else
	{
		//nand_init();
		nand_read_ll((unsigned int)src, dest, len);
	}
}

void clear_bss(void)
{
	extern int __bss_start, __bss_end__;
	int *p = &__bss_start;
	
	for (; p < &__bss_end__; p++)
		*p = 0;
}

void nand_init_ll(void)
{
#define TACLS   0
#define TWRPH0  1
#define TWRPH1  0
	/* setup timing */
	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	/* Enable NAND Flash control register, initialize ECC and disable chip select */
	NFCONT = (1<<4)|(1<<1)|(1<<0);	
}

static void nand_select(void)
{
	NFCONT &= ~(1<<1);	
}

static void nand_deselect(void)
{
	NFCONT |= (1<<1);	
}

static void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}

static void nand_addr(unsigned int addr)
{
	unsigned int col  = addr % 2048;
	unsigned int page = addr / 2048;
	volatile int i;

	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	
	NFADDR  = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR  = (page >> 8) & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR  = (page >> 16) & 0xff;
	for (i = 0; i < 10; i++);	
}

static void nand_wait_ready(void)
{
	while (!(NFSTAT & 1));
}

static unsigned char nand_data(void)
{
	return NFDATA;
}

void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int col = addr % 2048;
	int i = 0;
		
	/* 1. select NAND Flash */
	nand_select();

	while (i < len)
	{
		/* 2. send cmd 00h */
		nand_cmd(0x00);

		/* 3. send addr (5 cycles) */
		nand_addr(addr);

		/* 4. send cmd 30h */
		nand_cmd(0x30);

		/* 5. wait for ready */
		nand_wait_ready();

		/* 6. read data */
		for (; (col < 2048) && (i < len); col++)
		{
			buf[i] = nand_data();
			i++;
			addr++;
		}
		
		col = 0;
	}

	/* 7. deselect */		
	nand_deselect();
}

