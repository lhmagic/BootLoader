#ifndef		__BOOT_H__
#define		__BOOT_H__

#include "stm32f0xx_hal.h"

#define		VECTOR_TBL_SIZE							(16 + 32)
#define		BOOTLOADER_SIZE							(2048)

#define		BOOTLOADER_VERSION					"0.1.3"

typedef		void (*pfunc)(void);
typedef struct {
	uint32_t uid[3];
	uint32_t code;
	uint32_t rom;
	uint32_t ram;
	uint16_t page;
} s_chip_info;

//public functions prototype
void load_app(void);

//static functions prototype
static s_chip_info *read_chip(void);
static int8_t move_code(uint32_t src, uint32_t dest, uint32_t size);

#endif		//__BOOT_H__
