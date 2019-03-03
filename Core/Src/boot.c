#include	"boot.h"

//STM32F0 series chip code correspond page size & ram size
const uint8_t chip_cpr_tbl[][3] = {
//CODE		PAGE(KB)		RAM(KB)
	0x44, 		1, 					4,				//STM32F030x4/STM32F030x6/STM32F03x
	0x45, 		1,  				6,				//STM32F070x6/STM32F04x
	0x40, 		1, 					8,				//STM32F030x8/STM32F05x
	0x48, 		2, 					16,				//STM32F070xB/STM32F07x
	0x42, 		2, 					32				//STM32F030xC/STM32F09x
};

//read chip related informations
static s_chip_info *read_chip(void) {
static s_chip_info chip_info;
	
	if(chip_info.code == 0) {
		//chip 96bit UID
		for(uint8_t i=0; i<3; i++) {
			chip_info.uid[i] = *((uint32_t *)UID_BASE + i);
		}
		//chip device id code
		chip_info.code = *(uint32_t *)DBGMCU_BASE & ~0xF000;
		//chip rom size
		chip_info.rom = *(uint16_t *)FLASHSIZE_BASE * 1024;
		//find & calculate chip page & ram size in mem_tbl
		for(uint8_t i=0; i<sizeof(chip_cpr_tbl)/sizeof(chip_cpr_tbl[0]); i++) {
			if((chip_info.code & 0xFF) == chip_cpr_tbl[i][0]) {
				chip_info.page = chip_cpr_tbl[i][1] * 1024;
				chip_info.ram = chip_cpr_tbl[i][2] * 1024;
			}
		}	
	}
	
	return &chip_info;
}

//move source code to destination address.
//the source & destination address must be page alignment.
static int8_t move_code(uint32_t src, uint32_t dest, uint32_t size) {
s_chip_info *chip = read_chip();

	//check SP pointer range
	if((*(uint32_t *)src >= SRAM_BASE) && (*(uint32_t *)src < (SRAM_BASE + chip->ram))) {
	static FLASH_EraseInitTypeDef eraseType;
	uint32_t pageError;
	
		//erase destination pages.
		HAL_FLASH_Unlock();
		eraseType.TypeErase = FLASH_TYPEERASE_PAGES;
		eraseType.PageAddress = dest;
		eraseType.NbPages = (size + chip->page -1) / chip->page;
		if(HAL_FLASHEx_Erase(&eraseType, &pageError) == HAL_OK) {
			//copy code form source address to destination address.
			size = (size + sizeof(uint32_t) - 1)/sizeof(uint32_t);
			for(uint32_t i=0; i<size; i++) {
				if(((uint32_t *)src)[i] == 0xFFFFFF) {
					continue;
				} else if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest+i*4, ((uint32_t *)src)[i]) != HAL_OK) {
					HAL_FLASH_Lock();
					return -1;
				}
			}
			
			//erase source code after successful copy.
			eraseType.PageAddress = src;
			HAL_FLASHEx_Erase(&eraseType, &pageError);
			HAL_FLASH_Lock();			
		}
	}
	
	return 0;
}

//Initialization SP value, set PC to app start address & run app.
void load_app(void) {
s_chip_info *chip = read_chip();
uint32_t app_addr = FLASH_BASE + BOOTLOADER_SIZE;
uint32_t *src = (uint32_t *)app_addr;
uint32_t *dest = (uint32_t *)SRAM_BASE;
	
	//move code if needed.
	move_code(FLASH_BASE+chip->rom/2, app_addr, chip->rom/2 - BOOTLOADER_SIZE);

	//check SP point to SRAM address range.
	if((*src >= SRAM_BASE) && (*src < SRAM_BASE+chip->ram)) {
		//disable irq to avoid program corrupt when use RTOS.
		__disable_irq();		
		//copy vectors table to SRAM.
		for(uint8_t i=0; i<VECTOR_TBL_SIZE; i++) {
			*dest++ = *src++;
		}
		//reallocate SRAM to address 0x00000000
		__HAL_RCC_SYSCFG_CLK_ENABLE();
		__HAL_SYSCFG_REMAPMEMORY_SRAM();
		//update SP & PC value.
		__set_MSP(*(uint32_t *)app_addr);
		((pfunc)(*((uint32_t *)app_addr + 1)))();
	}
}

