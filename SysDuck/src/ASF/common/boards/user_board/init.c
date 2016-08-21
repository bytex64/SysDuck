/**
 * \file
 *
 * \brief USB Rubber Duck initialization
 *
 */

#include "compiler.h"
#include "user_board.h"
#include "conf_board.h"
#include "gpio.h"
#include "board.h"

void board_init(void)
{
	gpio_configure_pin(LED0_GPIO,GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED1_GPIO,GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	
	#if defined (CONF_BOARD_SD_MMC_SPI)
		static const gpio_map_t SD_MMC_SPI_GPIO_MAP = {
			{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },
			{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},
			{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},
			{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION},
		};

		// Assign I/Os to SPI.
		gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
			sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

		/*
		// Configure SD/MMC card detect and write protect pins
		#define SD_MMC_CONFIGURE_CD_WP_PIN(slot, unused) \
		  gpio_configure_pin(SD_MMC_##slot##_CD_GPIO, GPIO_DIR_INPUT); \
		  gpio_configure_pin(SD_MMC_##slot##_WP_GPIO, GPIO_DIR_INPUT);
		MREPEAT(SD_MMC_SPI_MEM_CNT, SD_MMC_CONFIGURE_CD_WP_PIN, ~)
		#undef SD_MMC_CONFIGURE_CD_WP_PIN
		*/

	#endif
}
