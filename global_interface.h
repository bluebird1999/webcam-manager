/*
 * main_interface.h
 *
 *  Created on: Aug 28, 2020
 *      Author: ning
 */

#ifndef MANAGER_GLOBAL_INTERFACE_H_
#define MANAGER_GLOBAL_INTERFACE_H_

/*
 * header
 */
#include "../tools/tools_interface.h"

/*
 * define
 */
#define				APPLICATION_VERSION_STRING	"beta-8.2"

#define 			MAX_SYSTEM_STRING_SIZE 		32
#define				MAX_SOCKET_TRY				3
#define				FILE_FLUSH_TIME				1000
#define				SERVER_RESTART_PAUSE		5
#define				SERVER_HEARTBEAT_INTERVAL	10
#define				MESSAGE_RESENT				10
#define				MAX_EQUIRY_TIMES			30
#define				MESSAGE_RESENT_SLEEP		1000*500

#define				ELR_USE_THREAD				1
//#define			MEMORY_POOL				1
#define				DMALLOC_ENABLE				1

/*
 * structure
 */
typedef enum manager_memory_mode_t {
	MEMORY_MODE_SHARED = 0,
	MEMORY_MODE_DYNAMIC,
};

typedef enum manager_running_mode_t {
	RUNNING_MODE_NONE,			//	0
	RUNNING_MODE_SLEEP,			//		1
	RUNNING_MODE_NORMAL,		//			2
	RUNNING_MODE_TESTING,		//		3
	RUNNING_MODE_SCANNER,		// 4
	RUNNING_MODE_DEEP_SLEEP,
	RUNNING_MODE_EXIT,
} manager_running_mode_t;

typedef enum manager_debug_level_t {
	DEBUG_NONE,					//0
	DEBUG_SERIOUS,
	DEBUG_WARNING,
	DEBUG_INFO,
	DEBUG_VERBOSE,
} manager_debug_level_t;

typedef enum manager_watchdog_level_t {
	WATCHDOG_NONE,					//0
	WATCHDOG_SOFT,
	WATCHDOG_HARD,
} manager_watchdog_level_t;

typedef struct manager_sleep_t {
	unsigned char 					enable;
	unsigned char					repeat;
	unsigned char					weekday;				//0~7;
	unsigned char					start[MAX_SYSTEM_STRING_SIZE];
	unsigned char					stop[MAX_SYSTEM_STRING_SIZE];
} manager_sleep_t;

typedef struct manager_config_t {
	unsigned char			running_mode;
	unsigned int			server_start;
	unsigned int			server_sleep;
	unsigned char			qcy_path[MAX_SYSTEM_STRING_SIZE*2];
	unsigned char			miio_path[MAX_SYSTEM_STRING_SIZE*2];
	unsigned char			fail_restart;
	unsigned char			fail_restart_interval;
	unsigned char			debug_level;
	unsigned char			heartbeat_enable;
	unsigned char			heartbeat_interval;
	unsigned char			watchdog_level;
	int						timezone;
	unsigned char			condition_limit;
	unsigned char			memory_mode;
	unsigned char			cache_clean;
	unsigned char			msg_overrun;
	unsigned char			miss_avmsg_overrun;
	unsigned char			recorder_avmsg_overrun;
	unsigned char			av_buff_overrun;
	unsigned char			overrun;
//sleep
	manager_sleep_t			sleep;
} manager_config_t;

/*
 * function
 */
/*
 * global variables
 */
extern	manager_config_t	_config_;
extern	elr_mpl_t 			_pool_;

#endif /* MANAGER_GLOBAL_INTERFACE_H_ */
