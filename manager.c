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
#include <dmalloc.h>
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
//server header
#include "global_interface.h"
#include "manager_interface.h"
#include "timer.h"

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
//		if( !server_micloud_start() )
//			misc_set_bit(&info.thread_start, SERVER_MICLOUD, 1);
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
	}
	return ret;
}

static void manager_kill_all(void)
{
	log_err("%%%%%%%%forcefully kill all%%%%%%%%%");
	exit(0);
}

static int server_release(void)
{
	int ret = 0;
	timer_release();
	return ret;
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
			send_msg.message = MSG_MANAGER_EXIT;
			server_device_message(&send_msg);
			//server_kernel_message(&send_msg);
			server_realtek_message(&send_msg);
		//	server_micloud_message(&send_msg);
			server_miss_message(&send_msg);
			server_miio_message(&send_msg);
			server_video_message(&send_msg);
			server_audio_message(&send_msg);
			server_recorder_message(&send_msg);
			server_player_message(&send_msg);
			server_speaker_message(&send_msg);
			log_info("sigint request from server %d", msg.sender);
			global_sigint = 1;
			break;
		case MSG_MANAGER_EXIT_ACK:
			misc_set_bit(&info.thread_start, msg.sender, 0);
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
		case MSG_MANAGER_HEARTBEAT:
			log_info("---heartbeat---at:%d",time_get_now_stamp());
			log_info("---from: %d---status: %d---thread: %d---init: %d", msg.sender, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck);
			break;
		default:
			log_err("not processed message = %d", msg.message);
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
	info.status = STATUS_WAIT;
	return 0;
}

static int server_wait(void)
{
	info.status = STATUS_SETUP;
	return 0;
}

static int server_setup(void)
{
    if( timer_init()!= 0 )
    	return -1;
	msg_buffer_init(&message, MSG_BUFFER_OVERFLOW_NO);
	pthread_rwlock_init(&info.lock, NULL);
	//start all servers
	if( !server_device_start() )
		misc_set_bit(&info.thread_start, SERVER_DEVICE, 1);
//	if( !server_kernel_start() )
	//	misc_set_bit(&info.thread_start, SERVER_KERNEL, 1);
	if( !server_realtek_start() )
		misc_set_bit(&info.thread_start, SERVER_REALTEK,1);
//	if( !server_micloud_start() )
	//	misc_set_bit(&info.thread_start, SERVER_MICLOUD, 1);
	if( !server_miio_start() )
		misc_set_bit(&info.thread_start, SERVER_MIIO, 1);
	if( !server_miss_start() )
		misc_set_bit(&info.thread_start, SERVER_MISS, 1);
	if( !server_video_start() )
		misc_set_bit(&info.thread_start, SERVER_VIDEO, 1);
	if( !server_audio_start() )
		misc_set_bit(&info.thread_start, SERVER_AUDIO, 1);
	if( !server_recorder_start() )
		misc_set_bit(&info.thread_start, SERVER_RECORDER, 1);
	if( !server_player_start() )
		misc_set_bit(&info.thread_start, SERVER_PLAYER, 1);
	if( !server_speaker_start() )
		misc_set_bit(&info.thread_start, SERVER_SPEAKER, 1);
	info.status = STATUS_IDLE;
	return 0;
}

static int server_idle(void)
{
	info.status = STATUS_START;
	return 0;
}

static int server_start(void)
{
	info.status = STATUS_RUN;
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
		if( thread_start == 0 ) {
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
	switch( info.status )
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
	sleep(1);
	return 0;
}

/*
 * external interface
 */
int manager_message(message_t *msg)
{
	int ret=0,ret1;
	if( info.status != STATUS_RUN ) {
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
