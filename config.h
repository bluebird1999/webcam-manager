/*
 * config_player.h
 *
 *  Created on: Aug 16, 2020
 *      Author: ning
 */

#ifndef MANAGER_CONFIG_H_
#define MANAGER_CONFIG_H_

/*
 * header
 */
#include "manager_interface.h"

/*
 * define
 */
#define 	CONFIG_MANAGER_CONFIG_PATH			"/opt/qcy/config/webcam.config"

/*
 * structure
 */

/*
 * function
 */
int config_manager_read(manager_config_t*);

#endif /* SERVER_MANAGER_CONFIG_H_ */
