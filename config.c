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
//program header
#include "../tools/tools_interface.h"
//server header
#include "config.h"
#include "manager_interface.h"

/*
 * static
 */
//variable
static pthread_rwlock_t			lock;
static int						dirty;
static manager_config_t			manager_config;
static config_map_t manager_config_map[] = {
	{"running_mode",     		&(manager_config.running_mode),      	cfg_u32, 	0,0,0,100,  	},
	{"server_start",     		&(manager_config.server_start),      	cfg_u32, 	0,0,0,100000000,  	},
	{"qcy_path",      			&(manager_config.qcy_path),       		cfg_string,	"0",0,0,64,},
	{"miio_path",      			&(manager_config.miio_path),   			cfg_string,	"0",0,0,64,},
	{"fail_restart",     		&(manager_config.fail_restart),      	cfg_u32, 	0,0,0,10,  	},
	{"fail_restart_interval",  	&(manager_config.fail_restart_interval),cfg_u32, 	0,0,0,1000,  	},
	{"debug_level",      		&(manager_config.debug_level),       	cfg_u32, 	0,0,0,100,},
	{"heartbeat_enable",		&(manager_config.heartbeat_enable),  	cfg_u32, 	0,0,0,10,},
	{"heartbeat_interval",		&(manager_config.heartbeat_interval),	cfg_u32, 	0,0,0,1000,},
	{"watchdog_level",     		&(manager_config.watchdog_level),      	cfg_u32, 	0,0,0,100,  	},
    {NULL,},
};
//function
static int manager_config_save(void);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * interface
 */
int config_manager_read(manager_config_t *rconfig)
{
	int ret,ret1=0;
	pthread_rwlock_init(&lock, NULL);
	ret = pthread_rwlock_wrlock(&lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = read_config_file(&manager_config_map, CONFIG_MANAGER_CONFIG_PATH);
	ret1 = pthread_rwlock_unlock(&lock);
	if (ret1)
		log_qcy(DEBUG_SERIOUS, "add unlock fail, ret = %d\n", ret);
	memcpy(rconfig,&manager_config,sizeof(manager_config_t));
	return ret;
}
