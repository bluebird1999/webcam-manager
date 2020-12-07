/*
 * manager.h
 *
 *  Created on: Aug 28, 2020
 *      Author: ning
 */

#ifndef MANAGER_MANAGER_H_
#define MANAGER_MANAGER_H_

/*
 * header
 */
#define		MANAGER_INIT_CONDITION_NUM				1
#define		MANAGER_INIT_CONDITION_CONFIG			0

/*
 * define
 */
#define	THREAD_TIMER				0

/*
 * structure
 */

/*
 * function
 */
int manager_proc(void);
int manager_init(void);
void manager_deep_sleep(void);
void manager_wakeup(void);
int manager_osd_init(void);
int manager_osd_release(void);
#endif /* MANAGER_MANAGER_H_ */
