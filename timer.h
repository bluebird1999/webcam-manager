/*
 * timer.h
 *
 *  Created on: Sep 17, 2020
 *      Author: ning
 */

#ifndef MANAGER_TIMER_INTERFACE_H_
#define MANAGER_TIMER_INTERFACE_H_

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
int timer_init(void);
int timer_proc(void);
int timer_release(void);
int timer_remove(HANDLER const task_handler);
int timer_add(HANDLER const task_handler, int interval, int delay, int oneshot, int sender);

#endif /* MANAGER_TIMER_INTERFACE_H_ */
