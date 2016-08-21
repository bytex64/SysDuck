/**
 * \file
 *
 * \brief Empty user application template
 *
 */

#include <stdio.h>
#include <asf.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "terminal.h"
#include "led.h"

static bool cdc_enabled;

int readline(char *, int);
void init_chibios(void);
void main_loop(void);

struct command {
	char *name;
	lua_CFunction func;
};
#define COMMAND(n) {#n, n}
extern struct command commands[];

static char *exception_types[] = {
	"Unrecoverable Exception",
	"TLB Multiple Hit",
	"Bus Error Data Fetch",
	"Bus Error Instruction Fetch",
	"NMI",
	"Instruction Address",
	NULL, /* ITLB Protection */
	NULL, /* Breakpoint */
	"Illegal Opcode",
	"Unimplemented Instruction",
	"Privilege Violation",
	NULL, /* Floating Point */
	NULL, /* Coprocessor Absent */
	"Data Address (Read)",
	"Data Address (Write)",
	"DTLB Protection (Read)",
	"DTLB Protection (Write)",
	NULL, /* DTLB Modified */
	NULL, /* ITLB Miss */
	NULL, /* DTLB Miss (Read) */
	NULL, /* DTLB Miss (Write) */
	NULL, /* Supervisor Call */
};

__attribute__((__interrupt__))
void ohshit(uint32_t ecr, uint32_t pc, uint32_t sr, uint32_t sp) {
	cpu_irq_enable();
	printf("\x07\x07 BLARGH! %s at %08x\r\n", exception_types[ecr], (unsigned int)pc);
	printf("SR %08x  SP %08x\r\n", (unsigned int)sr, (unsigned int)sp);
	for (int i = 0; i < 2; i++) {
		LED_On(LED0);
		LED_Off(LED1);
		delay_ms(400);
		LED_Off(LED0);
		LED_On(LED1);
		delay_ms(400);
	}
	printf("Press R to reset or any other key to continue\r\n");
	int c = getchar();
	if (c == 'R') {
		reset_do_soft_reset();
	}
	LED_Off(LED0);
	LED_Off(LED1);
}

void init_chibios() {
	LED_On(LED1);
	
	while (!cdc_enabled) {
		LED_Set_Intensity(LED1, 0x80);
		delay_ms(100);
		LED_Set_Intensity(LED1, 0x20);
		delay_ms(100);
	}
	term_init();
	LED_On(LED0);
	term_clear();
	term_goto(1, 1);
	printf("ChiBiOS 1.0\r\n");
	printf("MAIN: %d MHz  CPU %d MHz  PBA %d MHz  PBB %d MHz\r\n",
		(int)(sysclk_get_main_hz() / 1000000),
		(int)(sysclk_get_cpu_hz() / 1000000),
	    (int)(sysclk_get_pba_hz() / 1000000),
		(int)(sysclk_get_pbb_hz() / 1000000));
	
	sd_mmc_init();
	printf("SD slot 0 status: %d\r\n", sd_mmc_check(0));

	if (mem_test_unit_ready(LUN_ID_SD_MMC_0_MEM) == CTRL_GOOD) {
		U32 sectors;
		mem_read_capacity(LUN_ID_SD_MMC_0_MEM, &sectors);
		printf("SD Card Found: %0.2f MiB (%d sectors)\r\n", (float)sectors / 2048.0, (int)sectors);

		nav_reset();
		nav_drive_set(0);
		if (!nav_partition_mount()) {
			printf("Couldn't mount drive: %d\r\n", fs_g_status - FAIL);
		} else {
			printf("Drive mounted\r\n");
		}
	} else {
		printf("SD Card Not Found\r\n");
	}
	nav_reset();
	printf("\r\n");
	LED_Off(LED0);
	LED_Off(LED1);
}

int parse_cmd(char *buf, int n, char *args[]) {
	int i = 0;
	int arg = 0;
	
	while (i < n && arg < 16) {
		/* Trim heading whitespace */
		while (buf[i] == ' ' || buf[i] == '\t') {
			i++;
		}
		args[arg] = buf + i;
		
		/* Find word */
		while (buf[i] != ' ' && buf[i] != '\t' && buf[i] != 0) {
			i++;
		}
		/* Terminate word */
		buf[i] = 0;
		arg++;
		i++;
	}
	
	return arg;
}

int help(lua_State *L) {
	printf("Available commands:\r\n");
	for (int i = 0; commands[i].name != NULL; i++) {
		printf("  %s\r\n", commands[i].name);
	}
	return 0;
}

int led_status(lua_State *L) {
	U32 state = LED_Read_Display();
	printf("LED 0 %s\r\n", state & LED0 ? "on" : "off");
	printf("LED 1 %s\r\n", state & LED1 ? "on" : "off");
	return 0;
}
int led(lua_State *L) {
	int n_args = lua_gettop(L);
	if (n_args < 2) {
		printf("led(led_no, brightness)\r\n");
		printf("brightness is between 0 and 1\r\n");
		return 0;
	}
	
	int led = lua_tointeger(L, 1);
	if (led > 1) {
		printf("Invalid LED: %d\r\n", led);
	}
	led = LED0 + led;
	
	float brightness = lua_tonumber(L, 2);
	if (brightness == 1) {
		LED_On(led);
	} else if (brightness == 0) {
		LED_Off(led);
	} else {
		int intensity = brightness * 255.0;
		LED_Set_Intensity(led, intensity);
	}
	
	return 0;
}

int flash(lua_State *L) {
	printf("%dKiB Flash\r\n", flashc_get_flash_size() / 1024);
	uint32_t fuses = flashc_read_all_gp_fuses();
	printf("Fuses: %08x\r\n", (unsigned int)fuses);
	
	return 0;
}

int dfu(lua_State *L) {
	flashc_set_gp_fuse_bit(31, true);
	printf("ISP_FORCE set; resetting\r\n");
	delay_ms(100);
	reset_do_soft_reset();
	
	return 0;
}

int mem(lua_State *L) {
	int n_args = lua_gettop(L);
	if (n_args < 1) {
		printf("mem <addr> [size]\r\n");
		printf("If size is not specified, it defaults to 256 bytes");
		return 0;
	}
	unsigned int addr = lua_tointeger(L, 1);
	unsigned int size = 256;
	if (n_args == 2) {
		size = lua_tointeger(L, 2);
	}
	
	printf("Dumping %d bytes at %06x\r\n", size, addr);
	for (int i = 0; i < size; i += 32) {
		uint8_t *p = (uint8_t *) addr + i;
		printf("%06x:", (unsigned int)p);
		for (int j = 0; j < 16; j += 2) {
			printf(" %02x%02x", *(p + j), *(p + j + 1));
		}
		printf("\r\n");
	}
	
	return 0;
}

int keydump(lua_State *L) {
	int n = 0;
	while (1) {
		int c = getchar();
		printf("%02x ", c);
		if (c == 4) {
			break;
		}
		if (n++ == 16) {
			printf("\r\n");
			n = 0;
		}
	}
	
	return 0;
}

/*
void fs(int n_args, char *args[]) {
	char filename[256];
	if (n_args == 1) {
		printf("fs ls|format\r\n");
		return;
	}
	if (strncmp(args[1],"ls",3) == 0) {
		bool r = nav_dir_name(filename, 256);
		if (!r) {
			printf("Error getting dir name: %d\r\n", fs_g_status - FAIL);
		}
		printf("Dir name: %s\r\n", filename);

		nav_filelist_first(FS_DIR);
		nav_filelist_reset();

		while (nav_filelist_set(0, FS_FIND_NEXT)) {
			bool isdir = nav_file_isdir();
			int size = nav_file_lgt();
			nav_file_name(filename, 256, FS_NAME_GET, true);
			printf("%s%s%s\t%d\r\n",
				isdir ? "[" : "",
				filename,
				isdir ? "]" : "",
				size);
		}
		printf("%d dirs, %d files\r\n", nav_filelist_nb(FS_DIR), nav_filelist_nb(FS_FILE));
	} else if (strncmp(args[1], "format", 7) == 0) {
		if (!nav_drive_format(FS_FORMAT_DEFAULT)) {
			printf("Format failed: %d\r\n", fs_g_status - FAIL);
		} else {
			printf("Format OK\r\n");
		}
	} else if (strncmp(args[1], "cd", 3) == 0) {
		if (!nav_setcwd(args[2], false, false)) {
			printf("cd failed: %d\r\n", fs_g_status - FAIL);
		} else {
			printf("OK\r\n");
		}
	} else {
		printf("Unknown fs command\r\n");
	}
}
*/

int dd(lua_State *L) {
	int n_args = lua_gettop(L);
	if (n_args < 1) {
		printf("dd <sector>\r\n");
		U32 sectors;
		mem_read_capacity(LUN_ID_SD_MMC_0_MEM, &sectors);
		printf("%lu sectors, %0.2f MiB\r\n", sectors, (float)sectors / 2048.0);
		return 0;
	}
	
	uint32_t sector = lua_tointeger(L, 1);
	uint8_t block[512];
	Ctrl_status status = memory_2_ram(LUN_ID_SD_MMC_0_MEM, sector, block);
	if (status != CTRL_GOOD) {
		printf("Ctrl_status: %d\r\n", status);
		return 0;
	}
	for (int i = 0; i < 512; i += 16) {
		printf("%08lx: ", sector * 512 + i);
		for (int j = 0; j < 16; j += 2) {
			printf("%02x%02x ", block[i + j], block[i + j + 1]);
		}
		printf(" ");
		for (int j = 0; j < 16; j++) {
			char c = block[i + j];
			if (c < 32 || c > 127) {
				c = '.';
			}
			printf("%c", c);
		}
		printf("\r\n");
	}
	
	return 0;
}

int argdump(lua_State *L) {
	int n = lua_gettop(L);
	printf("%d args\r\n", n);
	for (int i = 1; i <= n; i++) {
		printf("Type ");
		switch (lua_type(L, i)) {
			case LUA_TNIL:
				printf("nil");
				break;
			case LUA_TNUMBER:
				printf("Number, %f", lua_tonumber(L, i));
				break;
			case LUA_TBOOLEAN:
				printf("Boolean, %s", lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TSTRING:
				printf("String, \"%s\"", lua_tostring(L, i));
				break;
			case LUA_TTABLE:
				printf("Table");
				break;
			case LUA_TFUNCTION:
				printf("Function");
			default:
				printf("Unknown");
		}
		printf("\r\n");
	}
	return 0;
}

struct command commands[] = {
	COMMAND(help),
	COMMAND(led),
	COMMAND(flash),
	COMMAND(mem),
	COMMAND(dfu),
	COMMAND(keydump),
//	COMMAND(fs),
	COMMAND(dd),
	COMMAND(argdump),
	{NULL, NULL}
};

void main_loop() {
	char inbuf[128];
	int inlen;
	lua_State *L;
	
	L = luaL_newstate();
	if (L == NULL) {
		printf("Could not allocate lua state\r\n");
		return;
	}
	luaL_openlibs(L);
	struct command *c = commands;
	while (c->name != NULL) {
		lua_pushcfunction(L, c->func);
		lua_setglobal(L, c->name);
		c++;
	}
	printf(LUA_VERSION " ready\r\n");

	while (1) {
		printf("\r\n>>> ");
		inlen = term_readline(inbuf, 128);

		if (luaL_dostring(L, inbuf)) {
			int len = lua_gettop(L);
			for (int i = 1; i <= len; i++) {
				printf("%s\r\n", luaL_tolstring(L, i, NULL));
			}
		}
		
		lua_settop(L, 0);
	}

	lua_close(L);
}

int main (void) {
	cdc_enabled = false;
	sysclk_init();
	irq_initialize_vectors();
	cpu_irq_enable();
	board_init();
	sleepmgr_init();
	udc_start();
	stdio_usb_init();
	stdio_usb_enable();
	
	init_chibios();
	main_loop();
}

bool chibi_cdc_enable() {
	cdc_enabled = true;
	return true;
}

void chibi_cdc_disable() {
	cdc_enabled = false;
}