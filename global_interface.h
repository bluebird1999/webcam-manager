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


/*
 * define
 */
#define				APPLICATION_VERSION_STRING	"alpha-3.14"

#define 			MAX_SYSTEM_STRING_SIZE 		32
#define				MAX_SOCKET_TRY				3
#define				FILE_FLUSH_TIME				30000
#define				SERVER_RESTART_PAUSE		5
#define				SERVER_HEARTBEAT_INTERVAL	10
#define				MESSAGE_RESENT				3

/*
 * structure
 */
typedef enum manager_running_mode_t {
	RUNNING_MODE_NONE,			//	0
	RUNNING_MODE_SLEEP,			//		1
	RUNNING_MODE_NORMAL,		//			2
	RUNNING_MODE_TESTING,		//		3
	RUNNING_MODE_SCANNER,		// 4
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

typedef struct manager_config_t {
	manager_running_mode_t	running_mode;
	unsigned int			server_start;
	char					qcy_path[MAX_SYSTEM_STRING_SIZE*2];
	char					miio_path[MAX_SYSTEM_STRING_SIZE*2];
	char					fail_restart;
	char					fail_restart_interval;
	manager_debug_level_t	debug_level;
	char					heartbeat_enable;
	char					heartbeat_interval;
	manager_watchdog_level_t watchdog_level;
	int						timezone;
} manager_config_t;

/*
 * function
 */

/*
 * global variables
 */
extern	manager_config_t	_config_;

#endif /* MANAGER_GLOBAL_INTERFACE_H_ */
