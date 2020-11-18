/*
 * manager.c
 *
 *  Created on: Aug 14, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <stdio.h>
#include <pthread.h>
#include <syscall.h>
#include <signal.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//program header
#include "../server/audio/audio_interface.h"
#include "../server/video/video_interface.h"
#include "../server/miio/miio_interface.h"
#include "../server/miss/miss_interface.h"
//#include "../server/micloud/micloud_interface.h"
#include "../server/realtek/realtek_interface.h"
#include "../server/device/device_interface.h"
//#include "../server/kernel/kernel_interface.h"
#include "../server/recorder/recorder_interface.h"
#include "../server/player/player_interface.h"
#include "../server/speaker/speaker_interface.h"
#include "../tools/tools_interface.h"
#include "../server/video2/video2_interface.h"
#include "../server/scanner/scanner_interface.h"
//server header
#include "global_interface.h"
#include "manager_interface.h"
#include "timer.h"
#include "config.h"

/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t 	info;
//function;
static int server_message_proc(void);
static int server_release(void);
static int task_error(void);
static int task_none(void);
static int task_sleep(void);
static int task_normal(void);
static int task_testing(void);
static int task_scanner(void);
//specific
static void manager_kill_all(void);
static int manager_server_start(int server);
static int manager_sleep(void);
static int manager_wakeup(void);
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
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
			st = server_scanner_message(msg);
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

static int manager_get_property(message_t *msg)
{
	int ret = 0, st;
	int temp;
	message_t send_msg;
    /********message body********/
	msg_init(&send_msg);
	memcpy(&(send_msg.arg_pass), &(msg->arg_pass),sizeof(message_arg_t));
	send_msg.message = msg->message | 0x1000;
	send_msg.sender = send_msg.receiver = SERVER_MANAGER;
	send_msg.arg_in.cat = msg->arg_in.cat;
	send_msg.result = 0;
	/****************************/
	if( send_msg.arg_in.cat == MANAGER_PROPERTY_SLEEP) {
		if( _config_.running_mode == RUNNING_MODE_SLEEP )
			temp = 0;
		else if( _config_.running_mode == RUNNING_MODE_NORMAL)
			temp = 1;
		else
			log_qcy(DEBUG_WARNING, "----current sleeping status = %d", _config_.running_mode );
		send_msg.arg = (void*)(&temp);
		send_msg.arg_size = sizeof(temp);
	}
	ret = send_message( msg->receiver, &send_msg);
	return ret;
}

static int manager_set_property(message_t *msg)
{
	int ret=0, mode = -1;
	message_t send_msg;
    /********message body********/
	msg_init(&send_msg);
	memcpy(&(send_msg.arg_pass), &(msg->arg_pass),sizeof(message_arg_t));
	send_msg.message = msg->message | 0x1000;
	send_msg.sender = send_msg.receiver = SERVER_MANAGER;
	send_msg.arg_in.cat = msg->arg_in.cat;
	/****************************/
	if( msg->arg_in.cat == MANAGER_PROPERTY_SLEEP ) {
		int temp = *((int*)(msg->arg));
		if( (temp == 0) && ( _config_.running_mode == RUNNING_MODE_SLEEP) ) {
			ret = 0;
		}
		else if( (temp == 1) && ( _config_.running_mode == RUNNING_MODE_NORMAL) ) {
			ret = 0;
		}
		else if( temp == 0 ) {
			if( info.status == STATUS_RUN )
				ret = manager_sleep();
			else
				log_qcy(DEBUG_INFO,"still in previous sleep enter processing");
		}
		else if( temp == 1 ) {
			if( info.status == STATUS_RUN )
				ret = manager_wakeup();
			else
				log_qcy(DEBUG_INFO,"still in previous sleep leaving processing");
		}
	}
	/***************************/
	send_msg.result = ret;
	ret = send_message(msg->receiver, &send_msg);
	/***************************/
	return ret;
}

static int manager_sleep(void)
{
	int ret = 0;
	info.status = STATUS_NONE;
	info.task.func = task_sleep;
	_config_.running_mode = RUNNING_MODE_SLEEP;
	return ret;
}

static int manager_wakeup(void)
{
	int ret = 0;
	info.status = STATUS_NONE;
	info.task.func = task_normal;
	_config_.running_mode = RUNNING_MODE_NORMAL;
	return ret;
}

static int manager_server_start(int server)
{
	int ret=0;
	switch(server) {
		case SERVER_DEVICE:
			if( !server_device_start() )
				misc_set_bit(&info.thread_start, SERVER_DEVICE, 1);
			break;
		case SERVER_KERNEL:
	//		if( !server_kernel_start() )
		//		misc_set_bit(&info.thread_start, SERVER_CONFIG, 1);
			break;
		case SERVER_REALTEK:
			if( !server_realtek_start() )
				misc_set_bit(&info.thread_start, SERVER_REALTEK, 1);
			break;
		case SERVER_MIIO:
			if( !server_miio_start() )
				misc_set_bit(&info.thread_start, SERVER_MIIO, 1);
			break;
		case SERVER_MISS:
			if( !server_miss_start() )
				misc_set_bit(&info.thread_start, SERVER_MISS, 1);
			break;
		case SERVER_MICLOUD:
//			if( !server_micloud_start() )
//				misc_set_bit(&info.thread_start, SERVER_MICLOUD, 1);
			break;
		case SERVER_VIDEO:
			if( !server_video_start() )
				misc_set_bit(&info.thread_start, SERVER_VIDEO, 1);
			break;
		case SERVER_AUDIO:
			if( !server_audio_start() )
				misc_set_bit(&info.thread_start, SERVER_AUDIO, 1);
			break;
		case SERVER_RECORDER:
			if( !server_recorder_start() )
				misc_set_bit(&info.thread_start, SERVER_RECORDER, 1);
			break;
		case SERVER_PLAYER:
			if( !server_player_start() )
				misc_set_bit(&info.thread_start, SERVER_PLAYER, 1);
			break;
		case SERVER_SPEAKER:
			if( !server_speaker_start() )
				misc_set_bit(&info.thread_start, SERVER_SPEAKER, 1);
			break;
		case SERVER_VIDEO2:
			if( !server_video2_start() )
				misc_set_bit(&info.thread_start, SERVER_VIDEO2, 1);
			break;
		case SERVER_SCANNER:
			if( !server_scanner_start() )
				misc_set_bit(&info.thread_start, SERVER_SCANNER, 1);
			break;
	}
	return ret;
}

static int manager_server_stop(int server)
{
	int ret=0;
	message_t msg;
	msg_init(&msg);
	msg.message = MSG_MANAGER_EXIT;
	msg.sender = msg.receiver = SERVER_MANAGER;
	ret = send_message(server, &msg);
	return ret;
}

static void manager_kill_all(void)
{
	log_qcy(DEBUG_SERIOUS, "%%%%%%%%forcefully kill all%%%%%%%%%");
	exit(0);
}

static int server_release(void)
{
	int ret = 0;
	timer_release();
	msg_buffer_release(&message);
	return ret;
}

static int server_clean(void)
{
	int ret = 0;
	timer_release();
	msg_buffer_release(&message);
	return ret;
}

static int server_message_proc(void)
{
	int ret = 0, ret1 = 0, i;
	message_t msg;
	message_t send_msg;
	msg_init(&msg);
	msg_init(&send_msg);
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add lock fail, ret = %d", ret);
		return ret;
	}
	ret = msg_buffer_pop(&message, &msg);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1) {
		log_qcy(DEBUG_SERIOUS, "add message unlock fail, ret = %d", ret1);
	}
	if( ret == -1) {
		msg_free(&msg);
		return -1;
	}
	else if( ret == 1) {
		return 0;
	}
	switch(msg.message){
		case MSG_MANAGER_SIGINT:
		case MSG_DEVICE_SIGINT:
	//	case MSG_KERNEL_SIGINT:
		case MSG_REALTEK_SIGINT:
	//	case MSG_MICLOUD_SIGINT:
		case MSG_MISS_SIGINT:
		case MSG_MIIO_SIGINT:
		case MSG_VIDEO_SIGINT:
		case MSG_AUDIO_SIGINT:
		case MSG_RECORDER_SIGINT:
		case MSG_PLAYER_SIGINT:
		case MSG_SPEAKER_SIGINT:
		case MSG_VIDEO2_SIGINT:
		case MSG_SCANNER_SIGINT:
			info.thread_exit = msg.sender;
			send_msg.message = MSG_MANAGER_EXIT;
			send_msg.sender = send_msg.receiver = SERVER_MANAGER;
			for(i=0;i<MAX_SERVER;i++) {
				if( misc_get_bit( info.thread_start, i) ) {
					if( (i != SERVER_REALTEK) && (i!=info.thread_exit) )
						send_message(i, &send_msg);
				}
			}
			log_qcy(DEBUG_INFO, "sigint request from server %d, exit code = %x", msg.sender, info.thread_start);
			if( !info.status2 ) {
				/********message body********/
				msg_init(&send_msg);
				send_msg.message = MSG_MANAGER_TIMER_ADD;
				send_msg.sender = SERVER_MANAGER;
				send_msg.arg_in.cat = 5000;
				send_msg.arg_in.dog = 0;
				send_msg.arg_in.duck = 1;
				send_msg.arg_in.handler = &manager_kill_all;
				manager_message(&msg);
				/****************************/
				info.status2 = 1;
			}
			break;
		case MSG_MANAGER_EXIT_ACK:
			misc_set_bit(&info.thread_start, msg.sender, 0);
			if( info.status2 ) {
				if( info.thread_start == (1<<SERVER_REALTEK) ) {
					msg_init(&send_msg);
					send_msg.sender = send_msg.receiver = SERVER_MANAGER;
					send_msg.message = MSG_MANAGER_EXIT;
					server_realtek_message(&send_msg);
				}
				else if( info.thread_start == ( (1<<SERVER_REALTEK) | (1<<info.thread_exit) ) ) {
					msg_init(&send_msg);
					send_msg.sender = send_msg.receiver = SERVER_MANAGER;
					send_msg.message = MSG_MANAGER_EXIT;
					send_message(info.thread_exit,&send_msg);
					log_qcy(DEBUG_INFO, "termination process--------exiter quit message sent!---%d", info.thread_exit);
				}
				else if( info.thread_start == 0 ) {	//quit all
					info.exit = 1;
				}
				log_qcy(DEBUG_INFO, "termination process quit status = %x", info.thread_start);
			}
			else if( _config_.running_mode == RUNNING_MODE_SCANNER ) {
				if( info.thread_start == 0 ) {	//quit all
					_config_.running_mode == RUNNING_MODE_NORMAL;
					info.task.func = task_normal;
					info.task.start = STATUS_NONE;
					info.task.end = STATUS_RUN;
					info.status = STATUS_NONE;
					info.status2 = 0;
				}
				else if( info.thread_start == (1<<SERVER_REALTEK) ) {
					msg_init(&send_msg);
					send_msg.sender = send_msg.receiver = SERVER_MANAGER;
					send_msg.message = MSG_MANAGER_EXIT;
					server_realtek_message(&send_msg);
				}
				log_qcy(DEBUG_INFO, "scanner process quit status = %x", info.thread_start);
			}
			else if( _config_.running_mode == RUNNING_MODE_NORMAL ){
				if( _config_.fail_restart ) {
					manager_server_start(msg.sender);
				}
				log_qcy(DEBUG_INFO, "restart process quit status = %x", info.thread_start);
			}
			else if( _config_.running_mode == RUNNING_MODE_SLEEP) {
				if( info.thread_start == (1<<SERVER_MIIO) ) {
					info.status = STATUS_START;
				}
				else if( info.thread_start == ((1<<SERVER_REALTEK)|(1<<SERVER_MIIO)) ) {
					msg_init(&send_msg);
					send_msg.sender = send_msg.receiver = SERVER_MANAGER;
					send_msg.message = MSG_MANAGER_EXIT;
					server_realtek_message(&send_msg);
				}
				log_qcy(DEBUG_INFO, "sleeping process quit status = %x", info.thread_start);
			}
			//to do: other mode
			break;
		case MSG_MANAGER_TIMER_ADD:
			if( timer_add(msg.arg_in.handler, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck, msg.sender) )
				log_qcy(DEBUG_WARNING, "add timer failed!");
			break;
		case MSG_MANAGER_TIMER_ACK:
			((HANDLER)msg.arg_in.handler)();
			break;
		case MSG_MANAGER_TIMER_REMOVE:
			if( timer_remove(msg.arg_in.handler) ) {
				log_qcy(DEBUG_WARNING, "remove timer failed!");
			}
			break;
		case MSG_MANAGER_HEARTBEAT:
			log_qcy(DEBUG_VERBOSE, "---heartbeat---at:%d",time_get_now_stamp());
			log_qcy(DEBUG_VERBOSE, "---from: %d---status: %d---thread: %d---init: %d", msg.sender, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck);
			break;
		case MSG_MIIO_PROPERTY_NOTIFY:
			if( msg.arg_in.cat == MIIO_PROPERTY_CLIENT_STATUS) {
				if( _config_.running_mode == RUNNING_MODE_SCANNER ) {
					if( (msg.arg_in.dog == STATE_WIFI_STA_MODE) ||
						(msg.arg_in.dog == STATE_CLOUD_CONNECTED) ) {
						msg_init(&send_msg);
						send_msg.sender = send_msg.receiver = SERVER_MANAGER;
						send_msg.message = MSG_MANAGER_EXIT;
						server_scanner_message(&send_msg);
						server_speaker_message(&send_msg);
						server_miio_message(&send_msg);
						log_qcy(DEBUG_INFO, "---scanner success!---");
					}
					else {
						log_qcy(DEBUG_INFO, "---scanner error status = %d---", msg.arg_in.dog);
					}

				}
			}
			break;
		case MSG_MANAGER_PROPERTY_GET:
			manager_get_property(&msg);
			break;
		case MSG_MANAGER_PROPERTY_SET:
			manager_set_property(&msg);
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "not processed message = %x", msg.message);
			break;
	}
	msg_free(&msg);
	return ret;
}

/*
 * state machine
 */
static int task_error(void)
{
	switch( info.status )
	{
		default:
			//write kernel out for restart!
			log_qcy(DEBUG_SERIOUS, "!!!!!!error in manager, quit all!!!");
			info.exit = 1;
			break;
	}
	return 0;
}

static int task_none(void)
{
	switch( info.status )
	{
		case STATUS_NONE:
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_none = %d", info.status);
			break;
	}
	return 0;
}

static int task_sleep(void)
{
	int i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<MAX_SERVER;i++) {
				if( misc_get_bit( info.thread_start, i) ) {
					if( (i != SERVER_MIIO) && (i!= SERVER_REALTEK) )
						manager_server_stop(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			break;
		case STATUS_RESTART:
			break;
		case STATUS_ERROR:
			info.task.func = task_error;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_normal = %d", info.status);
			break;
	}
	return 0;
}


static int task_normal(void)
{
	int i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<MAX_SERVER;i++) {
				if( misc_get_bit( _config_.server_start, i) ) {
					if( misc_get_bit( info.thread_start, i) != 1 )
						manager_server_start(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			info.status = STATUS_START;
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			break;
		case STATUS_RESTART:
			break;
		case STATUS_ERROR:
			info.task.func = task_error;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_normal = %d", info.status);
			break;
	}
	return 0;
}

static int task_testing(void)
{
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			break;
		case STATUS_IDLE:
			info.status = STATUS_START;
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			break;
		case STATUS_RESTART:
			break;
		case STATUS_ERROR:
			info.task.func = task_error;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_testing = %d", info.status);
			break;
	}
	return 0;
}

static int task_scanner(void)
{
	int start, i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			start = 10264;//2072; //10100000011000, miio, realtek, speaker, scanner;
			for(i=0;i<MAX_SERVER;i++) {
				if( misc_get_bit( start, i) ) {
					manager_server_start(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			info.status = STATUS_START;
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			break;
		case STATUS_RESTART:
			break;
		case STATUS_ERROR:
			info.task.func = task_error;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_scanner = %d", info.status);
			break;
	}
	return 0;
}

static int main_thread_termination(void)
{
	int ret=0;
    message_t msg;
    /********message body********/
    msg_init(&msg);
    msg.message = MSG_MANAGER_SIGINT;
	msg.sender = msg.receiver = SERVER_MANAGER;
    /****************************/
    manager_message(&msg);
	return ret;
}


/*
 * internal interface
 */
int manager_get_debug_level(int type)
{
	return _config_.debug_level;
}

/*
 * external interface
 */
int manager_init(void)
{
	int ret = 0;
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);
	ret = config_manager_read(&_config_);
	if( ret )
		return -1;
    if( timer_init()!= 0 )
    	return -1;
    _config_.timezone = 8;	//temporarily set to utc+8
	if( !message.init ) {
		msg_buffer_init(&message, MSG_BUFFER_OVERFLOW_NO);
	}
	pthread_rwlock_init(&info.lock, NULL);
	//default task
	if( _config_.running_mode == RUNNING_MODE_NONE) {
		info.task.func = task_none;
		info.task.start = STATUS_NONE;
		info.task.end = STATUS_NONE;
	}
	else if( _config_.running_mode == RUNNING_MODE_SLEEP ) {
		info.task.func = task_sleep;
		info.task.start = STATUS_NONE;
		info.task.end = STATUS_RUN;
	}
	else if( _config_.running_mode == RUNNING_MODE_NORMAL ) {
		info.task.func = task_normal;
		info.task.start = STATUS_NONE;
		info.task.end = STATUS_RUN;
	}
	else if( _config_.running_mode == RUNNING_MODE_TESTING ) {
		info.task.func = task_testing;
		info.task.start = STATUS_NONE;
		info.task.end = STATUS_RUN;
	}
	else if( _config_.running_mode == RUNNING_MODE_SCANNER ) {
		info.task.func = task_scanner;
		info.task.start = STATUS_NONE;
		info.task.end = STATUS_RUN;
	}
	//check scanner
	char fname[MAX_SYSTEM_STRING_SIZE*2];
	memset(fname,0,sizeof(fname));
	sprintf(fname, "%swifi.conf", _config_.miio_path);
	if( (_config_.running_mode == RUNNING_MODE_NORMAL) || (_config_.running_mode == RUNNING_MODE_TESTING) ) {
		if( (access(fname, F_OK))==-1) {
			_config_.running_mode = RUNNING_MODE_SCANNER;
			info.task.func = task_scanner;
			info.task.start = STATUS_NONE;
			info.task.end = STATUS_RUN;
		}
	}
}

int manager_proc(void)
{
	if ( !info.exit ) {
		info.task.func();
		server_message_proc();
		timer_proc();
	}
	else {
		server_release();
		log_qcy(DEBUG_SERIOUS, "-----------main exit-----------");
		exit(0);
	}
}

int manager_message(message_t *msg)
{
	int ret=0,ret1;
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add message lock fail, ret = %d", ret);
		return ret;
	}
	ret = msg_buffer_push(&message, msg);
	log_qcy(DEBUG_VERBOSE, "push into the manager message queue: sender=%d, message=%x, ret=%d, head=%d, tail=%d", msg->sender, msg->message, ret,
				message.head, message.tail);
	if( ret!=0 )
		log_qcy(DEBUG_WARNING, "message push in manager error =%d", ret);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1)
		log_qcy(DEBUG_SERIOUS, "add message unlock fail, ret = %d", ret1);
	return ret;
}
