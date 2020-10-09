/*
 * manager.h
 *
 *  Created on: Aug 14, 2020
 *      Author: ning
 */

/*
 *	Manager
 *	 Reside in main thread, manage all the servers *
 */

#ifndef MANAGER_MANAGER_INTERFACE_H_
#define MANAGER_MANAGER_INTERFACE_H_

/*
 * header
 */
#include <pthread.h>
#include "../tools/tools_interface.h"

/*
 * define
 */
#define	SERVER_MANAGER_VERSION_STRING		"alpha-3.1"

#define	MAX_SERVER			32

#define	SERVER_CONFIG		0
#define	SERVER_DEVICE		1
#define	SERVER_KERNEL		2
#define	SERVER_REALTEK		3
#define	SERVER_MIIO			4
#define	SERVER_MISS			5
#define	SERVER_MICLOUD		6
#define	SERVER_VIDEO		7
#define SERVER_AUDIO		8
#define	SERVER_RECORDER		9
#define	SERVER_PLAYER		10
#define SERVER_SPEAKER		11
#define	SERVER_MANAGER		32

typedef void (*HANDLER)(void);
typedef void (*AHANDLER)(void *arg);
#define TIMER_NUMBER  		16

/*
 * ------------message------------------
 */
#define		MSG_TYPE_GET					0
#define		MSG_TYPE_SET					1
#define		MSG_TYPE_SET_CARE				2

//common message
#define	MSG_MANAGER_BASE						(SERVER_MANAGER<<16)
#define	MSG_MANAGER_SIGINT						MSG_MANAGER_BASE | 0x0000
#define	MSG_MANAGER_SIGINT_ACK					MSG_MANAGER_BASE | 0x1000
#define	MSG_MANAGER_EXIT						MSG_MANAGER_BASE | 0x0001
#define	MSG_MANAGER_EXIT_ACK					MSG_MANAGER_BASE | 0x1001
#define	MSG_MANAGER_TIMER_ADD					MSG_MANAGER_BASE | 0x0010
#define	MSG_MANAGER_TIMER_ACK					MSG_MANAGER_BASE | 0x1010
#define	MSG_MANAGER_TIMER_REMOVE				MSG_MANAGER_BASE | 0x0011
#define	MSG_MANAGER_TIMER_REMOVE_ACK			MSG_MANAGER_BASE | 0x1011

/*
 * server status
 */
#define	STATUS_TYPE_STATUS					0
#define	STATUS_TYPE_EXIT					1
#define	STATUS_TYPE_CONFIG					2
#define STATUS_TYPE_THREAD_START			3
#define	STATUS_TYPE_THREAD_EXIT				4
#define	STATUS_TYPE_MESSAGE_LOCK			5
#define	STATUS_TYPE_STATUS2					6

/*
 * structure
 */
//sever status for local state machine
typedef enum server_status_t {
    STATUS_NONE,			//initial status = 0;
	STATUS_WAIT,			//wait state;
	STATUS_SETUP,			//setup action;
	STATUS_IDLE,			//idle state;
	STATUS_START,			//start action;
	STATUS_RUN,				//running state;
	STATUS_STOP,			//stop action;
	STATUS_RESTART,			//restart action;
	STATUS_ERROR,			//error action;
} server_status_t;

typedef struct task_t {
	HANDLER   		func;
	message_t		msg;
	server_status_t	start;
	server_status_t	end;
} task_t;

//server info
typedef struct server_info_t {
	server_status_t		status;
	pthread_rwlock_t	lock;
	pthread_t			id;
	int					error;
	int					msg_lock;
	int					status2;
	long int			tick;
	int					exit;
	task_t				task;
	int					thread_start;
	int					thread_exit;
} server_info_t;

typedef struct timer_struct_t
{
    int       		tid;
    int				sender;
    unsigned int 	tick;
    int				delay;
    unsigned int    interval;
    int		      	oneshot;
    HANDLER   		fpcallback;
} timer_struct_t;

/*
 * function
 */
int manager_message(message_t *msg);

#endif /* MANAGER_MANAGER_INTERFACE_H_ */
