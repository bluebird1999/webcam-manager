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
#include "../manager/manager.h"

#include <stdio.h>
#include <pthread.h>
#include <syscall.h>
#include <signal.h>
#include <sys/time.h>

#include "../manager/global_interface.h"
#include "../manager/manager_interface.h"
#include "../manager/timer.h"
//program header
#include "../server/config/config_interface.h"
#include "../server/audio/audio_interface.h"
#include "../server/video/video_interface.h"
#include "../server/miio/miio_interface.h"
#include "../server/miss/miss_interface.h"
//#include "../server/micloud/micloud_interface.h"
#include "../server/realtek/realtek_interface.h"
//#include "../server/device/device_interface.h"
//#include "../server/kernel/kernel_interface.h"
//#include "../server/recorder/recorder_interface.h"
//#include "../server/player/player_interface.h"
#include "../tools/tools_interface.h"
//server header

/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t 	info;
static int				thread_exit = 0;
static int				thread_start = 0;
static int				global_sigint = 0;
static int 				sw=0;
//function;
static int server_message_proc(void);
static int server_none(void);
static int server_wait(void);
static int server_setup(void);
static int server_idle(void);
static int server_start(void);
static int server_run(void);
static int server_stop(void);
static int server_restart(void);
static int server_error(void);
static int server_release(void);
static int server_set_thread_start(int index);
static int server_set_thread_exit(int index);
static int server_get_status(int type);
static int server_set_status(int type, int st);
//specific
static void manager_kill_all(void);
static int manager_server_start(int server);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static int manager_server_start(int server)
{
	int ret=0;
	switch(server) {
	case SERVER_CONFIG:
		if( !server_config_start() )
			server_set_thread_start(SERVER_CONFIG);
		break;
	case SERVER_DEVICE:
//		if( !server_device_start() )
//			server_set_thread_start(SERVER_DEVICE);
		break;
	case SERVER_KERNEL:
//		if( !server_kernel_start() )
	//		server_set_thread_start(SERVER_CONFIG);
		break;
	case SERVER_REALTEK:
		if( !server_realtek_start() )
			server_set_thread_start(SERVER_REALTEK);
		break;
	case SERVER_MIIO:
		if( !server_miio_start() )
			server_set_thread_start(SERVER_MIIO);
		break;
	case SERVER_MISS:
//		if( !server_miis_start() )
//			server_set_thread_start(SERVER_MISS);
		break;
	case SERVER_MICLOUD:
//		if( !server_micloud_start() )
//			server_set_thread_start(SERVER_MICLOUD);
		break;
	case SERVER_VIDEO:
		if( !server_video_start() )
			server_set_thread_start(SERVER_VIDEO);
		break;
	case SERVER_AUDIO:
		if( !server_audio_start() )
			server_set_thread_start(SERVER_AUDIO);
		break;
	case SERVER_RECORDER:
//		if( !server_recorder_start() )
//			server_set_thread_start(SERVER_RECORDER);
		break;
	case SERVER_PLAYER:
//		if( !server_player_start() )
//			server_set_thread_start(SERVER_PLAYER);
		break;
	}
	return ret;
}

static void manager_kill_all(void)
{
	log_err("%%%%%%%%forcefully kill all%%%%%%%%%");
	exit(0);
}

static int server_set_thread_exit(int index)
{
	thread_exit |= (1<<index);
}

static int server_set_thread_start(int index)
{
	thread_start |= (1<<index);
}

static int server_release(void)
{
	int ret = 0;
	timer_release();
	return ret;
}

static int server_set_status(int type, int st)
{
	int ret=-1;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_err("add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		info.status = st;
	else if(type==STATUS_TYPE_EXIT)
		info.exit = st;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_err("add unlock fail, ret = %d", ret);
	return ret;
}

static int server_get_status(int type)
{
	int st;
	int ret;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_err("add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		st = info.status;
	else if(type== STATUS_TYPE_EXIT)
		st = info.exit;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_err("add unlock fail, ret = %d", ret);
	return st;
}

static int server_message_proc(void)
{
	int ret = 0, ret1 = 0;
	message_t msg;
	message_t send_msg;
	msg_init(&msg);
	msg_init(&send_msg);
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_err("add lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_pop(&message, &msg);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1) {
		log_err("add message unlock fail, ret = %d\n", ret1);
	}
	if( ret == -1) {
		msg_free(&msg);
		return -1;
	}
	else if( ret == 1) {
		return 0;
	}
	switch(msg.message){
		case MSG_CONFIG_SIGINT:
	//	case MSG_DEVICE_SIGINT:
	//	case MSG_KERNEL_SIGINT:
		case MSG_REALTEK_SIGINT:
	//	case MSG_MICLOUD_SIGINT:
		case MSG_MISS_SIGINT:
		case MSG_MIIO_SIGINT:
		case MSG_VIDEO_SIGINT:
		case MSG_AUDIO_SIGINT:
			send_msg.message = MSG_MANAGER_EXIT;
			server_config_message(&send_msg);
		//	server_device_message(&send_msg);
			//server_kernel_message(&send_msg);
			server_realtek_message(&send_msg);
		//	server_micloud_message(&send_msg);
			server_miss_message(&send_msg);
			server_miio_message(&send_msg);
			server_video_message(&send_msg);
			server_audio_message(&send_msg);
			log_info("sigint request from server %d", msg.sender);
			global_sigint = 1;
			break;
		case MSG_MANAGER_EXIT_ACK:
			server_set_thread_exit(msg.sender);
			if( !global_sigint )
				manager_server_start(msg.sender);
			break;
		case MSG_MANAGER_TIMER_ADD:
			if( timer_add(msg.arg_in.handler, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck, msg.sender) )
				log_err("add timer failed!");
			break;
		case MSG_MANAGER_TIMER_ACK:
			((HANDLER)msg.arg_in.handler)();
			break;
		case MSG_MANAGER_TIMER_REMOVE:
			if( timer_remove(msg.arg_in.handler) )
				log_err("remove timer failed!");
			break;
	}
	msg_free(&msg);
	return ret;
}

/*
 * state machine
 */
static int server_none(void)
{
	server_set_status(STATUS_TYPE_STATUS, STATUS_WAIT);
	return 0;
}

static int server_wait(void)
{
	server_set_status(STATUS_TYPE_STATUS, STATUS_SETUP);
	return 0;
}

static int server_setup(void)
{
    if( timer_init()!= 0 )
    	return -1;
	msg_buffer_init(&message, MSG_BUFFER_OVERFLOW_NO);
	pthread_rwlock_init(&info.lock, NULL);

	//start all servers
	if( !server_config_start() )
		server_set_thread_start(SERVER_CONFIG);
//	if( !server_device_start() )
	//	manager_set_thread_start(SERVER_DEVICE);
//	if( !server_kernel_start() )
	//	manager_set_thread_start(SERVER_KERNEL);
	if( !server_realtek_start() )
		server_set_thread_start(SERVER_REALTEK);
//	if( !server_micloud_start() )
	//	manager_set_thread_start(SERVER_MICLOUD);
	if( !server_miio_start() )
		server_set_thread_start(SERVER_MIIO);
	if( !server_miss_start() )
		server_set_thread_start(SERVER_MISS);
	if( !server_video_start() )
		server_set_thread_start(SERVER_VIDEO);
	if( !server_audio_start() )
		server_set_thread_start(SERVER_AUDIO);
//	if( !server_recorder_start() )
	//	manager_set_thread_start(SERVER_RECORDER);
//	if( !server_player_start() )
	//	manager_set_thread_start(SERVER_PLAYER);
	server_set_status(STATUS_TYPE_STATUS, STATUS_IDLE);
	return 0;
}

static int server_idle(void)
{
	server_set_status(STATUS_TYPE_STATUS, STATUS_START);
	return 0;
}

static int server_start(void)
{
	server_set_status(STATUS_TYPE_STATUS, STATUS_RUN);
	return 0;
}

static int server_run(void)
{
	if(global_sigint) {
		if(!sw) {
			message_t msg;
			message_arg_t arg;
		    /********message body********/
			msg_init(&msg);
			msg.message = MSG_MANAGER_TIMER_ADD;
			msg.sender = SERVER_MANAGER;
			arg.cat = 5000;
			arg.dog = 0;
			arg.duck = 1;
			arg.handler = &manager_kill_all;
			msg.arg = &arg;
			msg.arg_size = sizeof(message_arg_t);
			/****************************/
			manager_message(&msg);
			sw = 1;
		}
		if( thread_exit == thread_start ) {
			log_info("quit all! thread exit code = %x ", thread_exit);
			exit(0);
		}
	}
	if( timer_proc()!=0 )
		log_err("error in timer proc!");
	if( server_message_proc()!=0 )
		log_err("error in message proc!");
	return 0;
}

static int server_stop(void)
{
	return 0;
}

static int server_restart(void)
{
	return 0;
}

static int server_error(void)
{
	server_release();
	msg_buffer_release(&message);
	log_err("!!!!!!!!error in manager!!!!!!!!");
	return 0;
}

int manager_proc(void)
{
	switch( server_get_status(STATUS_TYPE_STATUS) )
	{
	case STATUS_NONE:
		server_none();
		break;
	case STATUS_WAIT:
		server_wait();
		break;
	case STATUS_SETUP:
		server_setup();
		break;
	case STATUS_IDLE:
		server_idle();
		break;
	case STATUS_START:
		server_start();
		break;
	case STATUS_RUN:
		server_run();
		break;
	case STATUS_STOP:
		server_stop();
		break;
	case STATUS_RESTART:
		server_restart();
		break;
	case STATUS_ERROR:
		server_error();
		break;
	}
	return 0;
}

/*
 * external interface
 */
int manager_message(message_t *msg)
{
	int ret=0,ret1;
	if( server_get_status(STATUS_TYPE_STATUS)!= STATUS_RUN ) {
		log_err("manager is not ready!");
		return -1;
	}
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_err("add message lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_push(&message, msg);
	log_info("push into the manager message queue: sender=%d, message=%d, ret=%d", msg->sender, msg->message, ret);
	if( ret!=0 )
		log_err("message push in manager error =%d", ret);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1)
		log_err("add message unlock fail, ret = %d\n", ret1);
	return ret;
}
