/*
 * timer.c
 *
 *  Created on: Sep 17, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
//program header
#include "../tools/tools_interface.h"
//server header
#include "time.h"
#include "manager_interface.h"
/*
 * static
 */
//variable
static int timer_run = 0;
static timer_struct_t timers[TIMER_NUMBER];
static pthread_rwlock_t timer_lock ;
//function;
static unsigned int timer_get_ms(void);
static int send_message(int sender, message_t *msg);

//specific

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static unsigned int timer_get_ms(void)
{
    struct timeval tv;
    unsigned int time;
    gettimeofday(&tv, NULL);
    time = tv.tv_sec*1000  + tv.tv_usec / 1000;
    return time;
}

static int send_message(int receiver, message_t *msg)
{
	int st = 0;
	switch(receiver) {
		case SERVER_DEVICE:
			st = server_device_message(msg);
			break;
		case SERVER_KERNEL:
	//		st = server_kernel_message(msg);
			break;
		case SERVER_REALTEK:
			st = server_realtek_message(msg);
			break;
		case SERVER_MIIO:
			st = server_miio_message(msg);
			break;
		case SERVER_MISS:
			st = server_miss_message(msg);
			break;
		case SERVER_MICLOUD:
	//		st = server_micloud_message(msg);
			break;
		case SERVER_VIDEO:
			st = server_video_message(msg);
			break;
		case SERVER_AUDIO:
			st = server_audio_message(msg);
			break;
		case SERVER_RECORDER:
			st = server_recorder_message(msg);
			break;
		case SERVER_PLAYER:
			st = server_player_message(msg);
			break;
		case SERVER_SPEAKER:
			st = server_speaker_message(msg);
			break;
		case SERVER_VIDEO2:
			st = server_video2_message(msg);
			break;
		case SERVER_SCANNER:
//			st = server_scanner_message(msg);
			break;
		case SERVER_MANAGER:
			st = manager_message(msg);
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "unknown message target! %d", receiver);
			break;
	}
	return st;
}

/*
 * interface
 */
int timer_add(HANDLER const task_handler, int interval, int delay, int oneshot, int sender)
{
    int i;
    int ret = 0;
    if ((timer_run == 0) || (task_handler == NULL)) {
        return -1;
    }
    ret = pthread_rwlock_wrlock(&timer_lock);
    if (ret!=0) {
    	log_qcy( DEBUG_SERIOUS, "write lock obatain failed!");
    	pthread_rwlock_unlock(&timer_lock);
    	return -1;
    }

    int usable_id = 1110;
    for (i = 0; i < TIMER_NUMBER; i++) {
        if (timers[i].tid == 0 && (usable_id == 1110)) {
            usable_id = i;
        }
        if (timers[i].tid != 0 && task_handler == timers[i].fpcallback) {
            usable_id = 1110;
            break;
        }
    }
    if (i == TIMER_NUMBER) {
        if(usable_id == 1110) {
            log_qcy(DEBUG_SERIOUS, "timer_add_task: timer list full");
            ret = -1;
            goto out;
        } else {
            i = usable_id;
        }
    }
    memset(&timers[i],0,sizeof(timer_struct_t));
    timers[i].fpcallback = task_handler;

    timers[i].tid = i+1;
    timers[i].interval = interval;
    timers[i].oneshot = oneshot;
    timers[i].delay = delay;
    timers[i].sender = sender;

    if( timers[i].delay == -1 )
    	timers[i].tick = 0;
    else
    	timers[i].tick = timer_get_ms() + delay;
out:
    ret = pthread_rwlock_unlock(&timer_lock);
    if (ret!=0) {
    	log_qcy(DEBUG_SERIOUS, "un-write lock obatain failed!");
    }
    return ret;
}

int timer_remove(HANDLER const task_handler)
{
    int i;
    if(timer_run == 0 || task_handler==NULL) {
        return -1;
    }
    for(i = 0; i < TIMER_NUMBER; i++) {
        if(timers[i].tid != 0 &&  task_handler == timers[i].fpcallback) {
            break;
        }
    }
    if(i != TIMER_NUMBER)
    {
        pthread_rwlock_wrlock(&timer_lock);
        memset(&timers[i],0,sizeof(timer_struct_t));
        pthread_rwlock_unlock(&timer_lock);
    }
    return 0;
}

/*
 * interface internal
 */
int timer_init(void)
{
	int i,ret=0;
	for(i = 0; i < TIMER_NUMBER; i++) {
		memset(&timers[i],0,sizeof(timer_struct_t));
	}
	timer_run = 1;
	return ret;
}

int timer_proc(void)
{
    int i, ret=0;
    timer_struct_t *timer;
    timer_struct_t extimer;
    unsigned int now ;

	for(i = 0; i < TIMER_NUMBER; i++) {
		if( timers[i].tid != 0 ) {
			pthread_rwlock_wrlock(&timer_lock);
			timer = &timers[i];
			now = timer_get_ms();
			if( ( now < timer->tick) || (now > (timer->tick + timer->interval)) ) {
				memcpy(&extimer,timer,sizeof(timer_struct_t));
				if(timer->oneshot == 1) {
					memset(timer,0,sizeof(timer_struct_t));
				}
				else {
					timer->tick = timer_get_ms();
				}
				pthread_rwlock_unlock(&timer_lock);
//				extimer.fpcallback();
				message_t msg;
			    /********message body********/
				msg_init(&msg);
				msg.message = MSG_MANAGER_TIMER_ACK;
				msg.sender = SERVER_MANAGER;
				msg.receiver = extimer.sender;
				msg.arg_in.handler = (void*)extimer.fpcallback;
				/****************************/
				send_message(extimer.sender, &msg);
			}
			else {
				pthread_rwlock_unlock(&timer_lock);
			}
		}

	}
	usleep(1000);
    return ret;
}

int timer_release(void)
{
	int i,ret=0;
	for(i = 0; i < TIMER_NUMBER; i++) {
		memset(&timers[i],0,sizeof(timer_struct_t));
	}
	timer_run = 0;
	return ret;
}
