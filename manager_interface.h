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
#include "global_interface.h"
#include "../tools/tools_interface.h"

/*
 * define
 */
#define	SERVER_MANAGER_VERSION_STRING		"alpha-8.0"

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
#define SERVER_VIDEO2		12
#define SERVER_SCANNER		13
#define SERVER_VIDEO3		14
#define	SERVER_MANAGER		32

#define TIMER_NUMBER  		32

/*
const char server_name[][MAX_SYSTEM_STRING_SIZE] =
{
		"CONFIG", "DEVICE", "KERNEL", "REALTEK", "MIIO", "MISS", "MICLOUD",
		"VIDEO", "AUDIO", "RECORDER", "SPEAKER", "MANAGER",
};
*/

/*
 * ------------message------------------
 */
#define		MSG_TYPE_GET					0
#define		MSG_TYPE_SET					1
#define		MSG_TYPE_SET_CARE				2

//common message
#define	MSG_MANAGER_BASE						(SERVER_MANAGER<<16)
#define	MSG_MANAGER_SIGINT						(MSG_MANAGER_BASE | 0x0000)
#define	MSG_MANAGER_SIGINT_ACK					(MSG_MANAGER_BASE | 0x1000)
#define	MSG_MANAGER_EXIT						(MSG_MANAGER_BASE | 0x0001)
#define	MSG_MANAGER_EXIT_ACK					(MSG_MANAGER_BASE | 0x1001)
#define	MSG_MANAGER_DUMMY						(MSG_MANAGER_BASE | 0x0002)
#define	MSG_MANAGER_WAKEUP						(MSG_MANAGER_BASE | 0x0003)
#define	MSG_MANAGER_HEARTBEAT					(MSG_MANAGER_BASE | 0x0010)
#define	MSG_MANAGER_HEARTBEAT_ACK				(MSG_MANAGER_BASE | 0x1010)
#define	MSG_MANAGER_TIMER_ADD					(MSG_MANAGER_BASE | 0x0011)
#define	MSG_MANAGER_TIMER_ACK					(MSG_MANAGER_BASE | 0x0012)
#define	MSG_MANAGER_TIMER_REMOVE				(MSG_MANAGER_BASE | 0x0013)
#define	MSG_MANAGER_TIMER_REMOVE_ACK			(MSG_MANAGER_BASE | 0x1013)
#define	MSG_MANAGER_PROPERTY_GET				(MSG_MANAGER_BASE | 0x0020)
#define	MSG_MANAGER_PROPERTY_GET_ACK			(MSG_MANAGER_BASE | 0x1020)
#define	MSG_MANAGER_PROPERTY_SET				(MSG_MANAGER_BASE | 0x0021)
#define	MSG_MANAGER_PROPERTY_SET_ACK			(MSG_MANAGER_BASE | 0x1021)


#define	MANAGER_PROPERTY_SLEEP					(0x000 | PROPERTY_TYPE_GET)
#define	MANAGER_PROPERTY_DEEP_SLEEP_REPEAT		(0x002 | PROPERTY_TYPE_GET)
#define	MANAGER_PROPERTY_DEEP_SLEEP_WEEKDAY		(0x003 | PROPERTY_TYPE_GET)
#define	MANAGER_PROPERTY_DEEP_SLEEP_START		(0x004 | PROPERTY_TYPE_GET)
#define	MANAGER_PROPERTY_DEEP_SLEEP_END			(0x005 | PROPERTY_TYPE_GET)
#define	MANAGER_PROPERTY_TIMEZONE				(0x006 | PROPERTY_TYPE_GET)



typedef void (*HANDLER)(void);
typedef void (*AHANDLER)(void *arg);
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
#define	STATUS_TYPE_ERROR					7

/*
 *  property type
 */
#define	PROPERTY_TYPE_GET					(1 << 12)
#define	PROPERTY_TYPE_SET					(1 << 13)
#define	PROPERTY_TYPE_NOTIFY				(1 << 14)

/*
 * structure
 */
//sever status for local state machine
typedef enum server_status_t {
    STATUS_NONE = 0,		//initial status = 0;
	STATUS_WAIT,			//wait state;
	STATUS_SETUP,			//setup action;
	STATUS_IDLE,			//idle state;
	STATUS_START,			//start action;
	STATUS_RUN,				//running state;
	STATUS_STOP,			//stop action;
	STATUS_RESTART,			//restart action;
	STATUS_ERROR,			//error action;

	TASK_INIT = 100,		//general task
	TASK_WAIT,
	TASK_SETUP,
	TASK_IDLE,
	TASK_RUN,
	TASK_FINISH,
	TASK_FINISH2,

	EXIT_INIT = 1000,		//quit start
	EXIT_SERVER,
	EXIT_STAGE1,
	EXIT_THREAD,
	EXIT_STAGE2,
	EXIT_FINISH,
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
	server_status_t		old_status;
	pthread_t			id;
	int					init;
	int					error;
	int					msg_lock;
	int					status2;
	int					exit;
	task_t				task;
	int					thread_start;
	int					thread_exit;
	int					init_status;
	unsigned long long int	tick;
	pthread_rwlock_t	lock;		//deprecated, please use the static one
} server_info_t;

typedef struct timer_struct_t
{
    int       		tid;
    int				sender;
    unsigned int 	tick;
    int				delay;
    unsigned int    interval;
    int		      	oneshot;
    void   			*fpcallback;
    message_arg_t	arg;
} timer_struct_t;

/*
 * function
 */
int manager_message(message_t *msg);


int manager_common_send_message(int receiver, message_t *msg);
void manager_common_send_dummy(int server);
void manager_common_message_do_delivery(int receiver, message_t *msg);

#endif /* MANAGER_MANAGER_INTERFACE_H_ */
