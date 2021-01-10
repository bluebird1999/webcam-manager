/*
 * config_manager.c
 *
 *  Created on: Aug 16, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#ifdef DMALLOC_ENABLE
#include <dmalloc.h>
#endif
//program header
#include "../tools/tools_interface.h"
//server header
#include "config.h"
#include "manager_interface.h"

/*
 * static
 */
//variable
static manager_config_t			manager_config;
static config_map_t manager_config_profile_map[] = {
	{"running_mode",     		&(manager_config.running_mode),      	cfg_u8, 	0,0,0,100,  	},
	{"server_start",     		&(manager_config.server_start),      	cfg_u32, 	0,0,0,10000000,  },
	{"server_sleep",     		&(manager_config.server_sleep),      	cfg_u32, 	0,0,0,10000000,  	},
	{"qcy_path",      			&(manager_config.qcy_path),       		cfg_string,	"0",0,0,64,},
	{"miio_path",      			&(manager_config.miio_path),   			cfg_string,	"0",0,0,64,},
	{"fail_restart",     		&(manager_config.fail_restart),      	cfg_u8, 	0,0,0,10,  	},
	{"fail_restart_interval",  	&(manager_config.fail_restart_interval),cfg_u8, 	0,0,0,1000,  	},
	{"debug_level",      		&(manager_config.debug_level),       	cfg_u8, 	0,0,0,100,},
	{"heartbeat_enable",		&(manager_config.heartbeat_enable),  	cfg_u8, 	0,0,0,10,},
	{"heartbeat_interval",		&(manager_config.heartbeat_interval),	cfg_u8, 	0,0,0,255,},
	{"watchdog_level",     		&(manager_config.watchdog_level),      	cfg_u8, 	0,0,0,100,  	},
	{"timezone",     			&(manager_config.timezone),		      	cfg_s32, 	80,0,-1000,1000,  	},
	{"sleep_on",      			&(manager_config.sleep.enable),       	cfg_u8, 	0,0,0,1,},
	{"sleep_repeat",			&(manager_config.sleep.repeat),  		cfg_u8, 	0,0,0,255,},
	{"sleep_weekday",			&(manager_config.sleep.weekday),		cfg_u8, 	0,0,0,7,},
	{"sleep_start",     		&(manager_config.sleep.start),      	cfg_string,	"00:00",0,0,32,  	},
	{"sleep_stop",     			&(manager_config.sleep.stop),      		cfg_string,	"00:00",0,0,32,  	},
	{"memory_mode",     		&(manager_config.memory_mode),     		cfg_u8,		1,0,0,1,  	},
	{"cache_clean",     		&(manager_config.cache_clean),     		cfg_u8,		1,0,0,1,  	},
	{"msg_overrun",    			&(manager_config.msg_overrun), 			cfg_u8,		1,0,0,1,  	},
	{"miss_avmsg_overrun",    	&(manager_config.miss_avmsg_overrun), 	cfg_u8,		1,0,0,1,  	},
	{"recorder_avmsg_overrun",  &(manager_config.recorder_avmsg_overrun), 	cfg_u8,		1,0,0,1,  	},
	{"av_buff_overrun",    		&(manager_config.av_buff_overrun), 		cfg_u8,		1,0,0,1,  	},
    {NULL,},
};

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */


/*
 * interface
 */
int config_manager_read(manager_config_t *aconfig)
{
	int ret;
	ret = read_config_file(&manager_config_profile_map, CONFIG_MANAGER_CONFIG_PATH);
	memcpy(aconfig,&manager_config,sizeof(manager_config_t));
	return ret;
}

int config_manager_set(int module, void *arg)
{
	int ret = 0;
	memcpy( (manager_config_t*)(&manager_config), arg, sizeof(manager_config_t));
	manager_config.running_mode = RUNNING_MODE_NORMAL;
	if( manager_config.sleep.enable != 2)
		manager_config.sleep.enable = 0;
	ret = write_config_file(&manager_config_profile_map, CONFIG_MANAGER_CONFIG_PATH);
	return ret;
}
