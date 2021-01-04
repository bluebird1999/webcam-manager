/*
 * main.c
 *
 *  Created on: Aug 20, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <malloc.h>
#include <execinfo.h>
#ifdef DMALLOC_ENABLE
#include <dmalloc.h>
#endif
//program header
#include "manager_interface.h"
#include "../server/audio/audio_interface.h"
#include "../server/miio/miio_interface.h"
#include "../server/miss/miss_interface.h"
#include "../server/micloud/micloud_interface.h"
#include "../server/realtek/realtek_interface.h"
#include "../server/device/device_interface.h"
#include "../server/kernel/kernel_interface.h"
#include "../server/recorder/recorder_interface.h"
#include "../server/player/player_interface.h"
#include "../server/speaker/speaker_interface.h"
#include "../tools/tools_interface.h"
#include "../server/scanner/scanner_interface.h"
#include "../server/video/video_interface.h"
#include "../server/video2/video2_interface.h"
//server header
#include "main.h"
#include "watchdog_interface.h"
//server header

/*
 * static
 */
//variable
static FILE *tempFd = NULL;
static FILE *logFd = NULL;
static char file_c = 1;
static char *log_path = NULL;

//function;
static void log_redirect(void);
static void log_check(void);


/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */



/*
 * helper
 */
void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGSEGV:
        case SIGFPE:
        case SIGBUS: {
			void*   array[20];
			size_t  size;
			char**  strings;
			size_t  i;
			size = backtrace(array, 20);
			strings = backtrace_symbols(array, size);
			for (i=0; i< size; i++) {
				printf("%s\n", strings[i]);
			}
			free(strings);
			exit(EXIT_FAILURE);
			break;
        }
        default:
            break;
    }
}

void log_redirect_close(void)
{
	if(logFd != NULL)
		   fclose(logFd);

	system("sync");
}

static void log_redirect(void)
{
	char *p = NULL;
	p = getenv("LOG_REDIRECT_PATH");

	if(p == NULL)
	   return;

	log_path = calloc(32, 1);
	memcpy(log_path, p, strlen(p));
	if(logFd == NULL && log_path != NULL) {
		file_c = 1;
		logFd = fopen(log_path, "w+");
		if(logFd != NULL) {
			stdout = logFd;
		}
	}

	fflush(stdout);

	return;
}

static void log_check(void)
{
	long file_size = 0;
	char log2_path[32] = {0};

	if(logFd == NULL)
		   return;

	file_size = ftell(logFd);

	if(file_size <= (512 * 1024))
		   return;

	if(file_c == 1) {
		   file_c = 2;
		   snprintf(log2_path, sizeof(log2_path), "%s%d", log_path, 1);
		   tempFd = fopen(log2_path, "w+");
	}
	else if(file_c == 2) {
		   file_c = 1;
		   tempFd = fopen(log_path, "w+");
	}

	if(tempFd != NULL) {
		   stdout = tempFd;
	}
	if(logFd != NULL) {
		   fclose(logFd);
	}
	logFd = tempFd;

	return;
}
/*
 * 	Main function, entry point
 *
 * 		NING,  2020-08-13
 *
 */
int main(int argc, char *argv[])
{
    	signal(SIGSEGV, signal_handler);
    	signal(SIGFPE,  signal_handler);
    	signal(SIGBUS,  signal_handler);
	
	log_redirect();

	printf("++++++++++++++++++++++++++++++++++++++++++\r\n");
	printf("   webcam started\r\n");
	printf("---version---\r\n");
	printf("%10s: %s\r\n", "program",APPLICATION_VERSION_STRING);
	printf("   servers\r\n");
	printf("%10s: %s\r\n", "manager",SERVER_MANAGER_VERSION_STRING);
	printf("%10s: %s\r\n", "tools",TOOLS_VERSION_STRING);
	printf("%10s: %s\r\n", "device",SERVER_DEVICE_VERSION_STRING);
	printf("%10s: %s\r\n", "kernel",SERVER_KERNEL_VERSION_STRING);
	printf("%10s: %s\r\n", "micloud",SERVER_MICLOUD_VERSION_STRING);
	printf("%10s: %s\r\n", "miio",SERVER_MIIO_VERSION_STRING);
	printf("%10s: %s\r\n", "miss",SERVER_MISS_VERSION_STRING);
	printf("%10s: %s\r\n", "realtek",SERVER_REALTEK_VERSION_STRING);
	printf("%10s: %s\r\n", "video",SERVER_VIDEO_VERSION_STRING);
	printf("%10s: %s\r\n", "audio",SERVER_AUDIO_VERSION_STRING);
	printf("%10s: %s\r\n", "recorder",SERVER_RECORDER_VERSION_STRING);
	printf("%10s: %s\r\n", "player",SERVER_PLAYER_VERSION_STRING);
	printf("%10s: %s\r\n", "speaker",SERVER_SPEAKER_VERSION_STRING);
	printf("%10s: %s\r\n", "video2",SERVER_VIDEO2_VERSION_STRING);
	printf("%10s: %s\r\n", "scanner",SERVER_SCANNER_VERSION_STRING);
	printf("++++++++++++++++++++++++++++++++++++++++++\r\n");
	manager_init();

/*
 * main loop
 */
	while(1) {
		log_check();
	/*
	 * manager proc
	 */
		manager_proc();
	/*
	 *  watch-dog proc
	 */
		watchdog_proc();
	}
//---unexpected catch here---
	log_redirect_close();
	printf("-----------thread exit: main-----------");
	return 1;
}
