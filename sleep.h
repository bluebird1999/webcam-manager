/*
 * sleep.h
 *
 *  Created on: Sep 17, 2020
 *      Author: ning
 */

#ifndef MANAGER_SLEEP_INTERFACE_H_
#define MANAGER_SLEEP_INTERFACE_H_

/*
 * header
 */
#include "../manager/manager_interface.h"

/*
 * define
 */

/*
 * structure
 */

/*
 * function
 */
int sleep_init(int n, char *start, char*stop);
int sleep_release(void);
int sleep_scheduler_init(int n, char *start, char*stop);

#endif /* MANAGER_SLEEP_INTERFACE_H_ */
