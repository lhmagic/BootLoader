#ifndef		__BOOT_H__
#define		__BOOT_H__

#include "stm32f0xx_hal.h"

#define		VECTOR_TBL_SIZE			(16 + 32)
#define		BOOTLOADER_SIZE			(0x800)

#define		BL_VERSION					"0.1.0"

typedef		void (*pfunc)(void);
typedef struct {
	uint32_t uid[3];
	uint16_t dev_id;
	uint16_t rev_id;
	uint16_t rom_size;
	uint16_t page_size;
	uint16_t ram_size;
} s_chip_info;

//public functions prototype
s_chip_info *read_chip(void);
void move_code(void);
void run_app(void);

#endif		//__BOOT_H__
