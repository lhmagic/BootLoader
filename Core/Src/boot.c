#include	"boot.h"

//const s_chip_info chip_info[] = {
////chip ID	RAM (Kbytes)	Family
//	0x444, 	4, 						"STM32F030x4/STM32F030x6/STM32F03x",
//	0x445, 	6, 						"STM32F070x6/STM32F04x",
//	0x440, 	8, 						"STM32F030x8/STM32F05x",
//	0x448, 	16, 					"STM32F070xB/STM32F07x",
//	0x442, 	32, 					"STM32F030xC/STM32F09x",
//};

const uint8_t mem_tbl[][3] = {
//chipID		PAGE(KB)		RAM(KB)
	0x44, 		1, 					4,
	0x45, 		1,  				6,
	0x40, 		1, 					8,
	0x48, 		2, 					16,
	0x42, 		2, 					32
};

s_chip_info *read_chip(void) {
static s_chip_info chip;
	chip.uid[0] = *(uint32_t *)UID_BASE;
	chip.uid[1] = *((uint32_t *)UID_BASE + 1);
	chip.uid[2] = *((uint32_t *)UID_BASE + 2);
	chip.dev_id = (*(uint32_t *)DBGMCU_BASE) & 0x0FFF;
	chip.rev_id = (*(uint32_t *)DBGMCU_BASE) >> 16;
	chip.rom_size = *(uint16_t *)FLASHSIZE_BASE;

	for(uint8_t i=0; i<sizeof(mem_tbl)/sizeof(mem_tbl[0]); i++) {
		if((chip.dev_id & 0xFF) == mem_tbl[i][0]) {
			chip.page_size = mem_tbl[i][1];
			chip.ram_size = mem_tbl[i][2];
		}
	}
	return &chip;
}

void move_code(void) {
uint32_t src, dest, app_size;
s_chip_info *chip = read_chip();
	
	src = chip->rom_size * 1024 / 2 + FLASH_BASE;
	dest = BOOTLOADER_SIZE + FLASH_BASE;
	app_size = src - dest;
	
	if((*(uint32_t *)src >= SRAM_BASE) && (*(uint32_t *)src < (SRAM_BASE+chip->ram_size * 1024))) {
	FLASH_EraseInitTypeDef eraseType;
	uint32_t pageError;	
	
		HAL_FLASH_Unlock();
		
		//erase destnation pages.
		eraseType.TypeErase = FLASH_TYPEERASE_PAGES;
		eraseType.PageAddress = dest;
		eraseType.NbPages = app_size / (chip->page_size * 1024);
		
		//move code form source to destnation.
		if(HAL_FLASHEx_Erase(&eraseType, &pageError) == HAL_OK) {
			for(uint32_t i=0; i<app_size/sizeof(uint32_t); i++) {
				if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest+i*4, ((uint32_t *)src)[i]) != HAL_OK) {
					HAL_FLASH_Lock();
					return;
				}
			}			
		}
		
		//erase source pages.
		eraseType.PageAddress = src;
		HAL_FLASHEx_Erase(&eraseType, &pageError);
				
		HAL_FLASH_Lock();
	}
}

void run_app(void) {
uint32_t *app_addr = (uint32_t *)(BOOTLOADER_SIZE + FLASH_BASE);
s_chip_info *chip = read_chip();

	//check SP point to SRAM address range.
	if((*app_addr >= SRAM_BASE) && (*app_addr < (SRAM_BASE+chip->ram_size * 1024))) {
		//copy vectors table to SRAM.
	__IO uint32_t *src = app_addr;
	__IO uint32_t *dest = (uint32_t *)SRAM_BASE;
		for(uint8_t i=0; i<VECTOR_TBL_SIZE; i++) {
			*dest++ = *src++;
		}
		//reallocate SRAM to address 0x00000000
		__HAL_RCC_SYSCFG_CLK_ENABLE();
		__HAL_SYSCFG_REMAPMEMORY_SRAM();
		//update SP & PC value.
		__set_MSP(*app_addr);
		((pfunc)(*(app_addr + 1)))();
	}
}

