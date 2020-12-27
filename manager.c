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
#include "../server/miio/miio_interface.h"
#include "../server/miss/miss_interface.h"
#include "../server/micloud/micloud_interface.h"
#include "../server/realtek/realtek_interface.h"
#include "../server/device/device_interface.h"
#include "../server/kernel/kernel_interface.h"
#include "../server/recorder/recorder_interface.h"
#include "../server/player/player_interface.h"
#include "../server/speaker/speaker_interface.h"
#include "../tools/tools_interface.h"
#include "../server/scanner/scanner_interface.h"
#include "../server/video/video_interface.h"
#include "../server/video2/video2_interface.h"
//server header
#include "manager.h"
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
static pthread_rwlock_t	ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;

//function;
static int server_message_proc(void);
static void task_deep_sleep(void);
static void task_sleep(void);
static void task_default(void);
static void task_testing(void);
static void task_scanner(void);
static void task_exit(void);
static void main_thread_termination(void);
//specific
static int manager_server_start(int server);
static void manager_sleep(void);
static void manager_send_wakeup(int server);
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static void *manager_timer_func(void)
{
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);
	pthread_detach(pthread_self());
	//default task
	misc_set_bit(&info.error, THREAD_TIMER, 1);
	misc_set_thread_name("manager_timer");
	while( 1 ) {
		if(info.exit ) break;
		if( misc_get_bit(info.thread_exit, THREAD_TIMER) ) break;
		timer_proc(info.thread_start);
		usleep(100000);
	}
	timer_release();
	misc_set_bit(&info.error, THREAD_TIMER, 0);
	manager_common_send_dummy(SERVER_MANAGER);
	log_qcy(DEBUG_INFO, "-----------thread exit: timer-----------");
	pthread_exit(0);
}

static int manager_mempool_init(void)
{
	int ret = 0;
    elr_mpl_init();
    _pool_ = elr_mpl_create(NULL,32768);
    return ret;
}

static int manager_mempool_deinit(void)
{
	int ret = 0;
    elr_mpl_destroy(&_pool_);
    elr_mpl_finalize();
    return ret;
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
		if( _config_.sleep.enable == 0 )
			temp = 1;
		else if( _config_.sleep.enable == 1 )
			temp = 0;
		else if( _config_.sleep.enable == 2 )
			temp = 2;
		else
			log_qcy(DEBUG_WARNING, "----current sleeping status = %d", _config_.running_mode );
		send_msg.arg = (void*)(&temp);
		send_msg.arg_size = sizeof(temp);
	}
	ret = manager_common_send_message( msg->receiver, &send_msg);
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
	send_msg.arg_in.wolf = 0;
	if( msg->arg_in.cat == MANAGER_PROPERTY_SLEEP ) {
		int temp = *((int*)(msg->arg));
		if( ( (temp == 1) && ( _config_.sleep.enable == 0) ) ||
			( (temp == 0) && ( _config_.sleep.enable == 1) ) ||
			( (temp == 2) && ( _config_.sleep.enable == 2) ) ) {
			ret = 0;
		}
		else {
			if( temp == 1 ) {
				if( info.status == STATUS_RUN ) {
					_config_.running_mode = RUNNING_MODE_NORMAL;
					_config_.sleep.enable = 0;
					config_manager_set(0, &_config_);
					manager_wakeup();
					send_msg.arg_in.wolf = 1;
				}
				else {
					log_qcy(DEBUG_INFO,"still in previous normal enter processing");
				}
			}
			else if( temp == 0 ) {
				if( info.status == STATUS_RUN ) {
					_config_.running_mode = RUNNING_MODE_SLEEP;
					_config_.sleep.enable = 1;
					config_manager_set(0, &_config_);
					manager_sleep();
					send_msg.arg_in.wolf = 1;
				}
				else {
					log_qcy(DEBUG_INFO,"still in previous sleep leaving processing");
				}
			}
		}
		send_msg.arg = (void*)(&temp);
		send_msg.arg_size = sizeof(temp);
	}
	/***************************/
	send_msg.result = ret;
	ret = manager_common_send_message(msg->receiver, &send_msg);
	/***************************/
	return ret;
}

static void manager_send_wakeup(int server)
{
	message_t msg;
	msg_init(&msg);
	msg.message = MSG_MANAGER_WAKEUP;
	msg.sender = msg.receiver = SERVER_MANAGER;
	manager_common_send_message(server, &msg);
}

static void manager_sleep(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_sleep;
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
			if( !server_kernel_start() )
				misc_set_bit(&info.thread_start, SERVER_KERNEL, 1);
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
			if( !server_micloud_start() )
				misc_set_bit(&info.thread_start, SERVER_MICLOUD, 1);
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
	msg.arg_in.cat = info.thread_start;
	ret = manager_common_send_message(server, &msg);
	return ret;
}

static int heart_beat_proc(void)
{
	int ret = 0;
	long long int tick = 0;
	tick = time_get_now_stamp();
	if( (tick - info.tick) > 3 ) {
		info.tick = tick;
		system("top -n 1 | grep 'Load average:'");
		system("top -n 1 | grep 'webcam'");
		system("top -n 1 | grep 'CPU'");
	}
	return ret;
}

static void manager_kill_all(message_arg_t *msg)
{
	log_qcy(DEBUG_INFO, "%%%%%%%%forcefully kill all%%%%%%%%%");
	exit(0);
}

static void manager_delayed_start(message_arg_t *msg)
{
	manager_server_start(msg->cat);
	manager_send_wakeup(SERVER_MIIO);
}

static void manager_broadcast_thread_exit(void)
{
}

static void server_release_1(void)
{
}

static void server_release_2(void)
{
	sleep_release();
	msg_buffer_release2(&message, &mutex);
#ifdef MEMORY_POOL
    manager_mempool_deinit();
#endif
}

static void server_release_3(void)
{
	msg_free(&info.task.msg);
	memset(&info, 0, sizeof(server_info_t));
}

static int server_message_proc(void)
{
	int ret = 0;
	message_t msg;
	message_t send_msg;
//condition
	pthread_mutex_lock(&mutex);
	if( message.head == message.tail ) {
		if( info.status == info.old_status ) {
			pthread_cond_wait(&cond,&mutex);
		}
	}
	if( info.msg_lock ) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	msg_init(&msg);
	ret = msg_buffer_pop(&message, &msg);
	pthread_mutex_unlock(&mutex);
	if( ret == 1 ) {
		return 0;
	}
	log_qcy(DEBUG_VERBOSE, "-----pop out from the MANAGER message queue: sender=%d, message=%x, ret=%d, head=%d, tail=%d", msg.sender, msg.message,
				ret, message.head, message.tail);
	msg_init(&info.task.msg);
	msg_copy(&info.task.msg, &msg);
	switch(msg.message){
		case MSG_MANAGER_SIGINT:
		case MSG_DEVICE_SIGINT:
		case MSG_KERNEL_SIGINT:
		case MSG_REALTEK_SIGINT:
		case MSG_MICLOUD_SIGINT:
		case MSG_MISS_SIGINT:
		case MSG_MIIO_SIGINT:
		case MSG_VIDEO_SIGINT:
		case MSG_AUDIO_SIGINT:
		case MSG_RECORDER_SIGINT:
		case MSG_PLAYER_SIGINT:
		case MSG_SPEAKER_SIGINT:
		case MSG_VIDEO2_SIGINT:
		case MSG_SCANNER_SIGINT:
			if( info.task.func != task_exit) {
				info.task.func = task_exit;
				info.status = EXIT_INIT;
				_config_.running_mode = RUNNING_MODE_EXIT;
			}
			else {
				log_qcy(DEBUG_WARNING, "already inside the manager exit task!");
			}
			break;
		case MSG_MANAGER_EXIT_ACK:
			misc_set_bit(&info.thread_start, msg.sender, 0);
			if( _config_.running_mode == RUNNING_MODE_NORMAL ){
				if( _config_.fail_restart ) {
					/********message body********/
					msg_init(&send_msg);
					send_msg.message = MSG_MANAGER_TIMER_ADD;
					send_msg.sender = SERVER_MANAGER;
					send_msg.arg_in.cat = _config_.fail_restart_interval * 1000;
					send_msg.arg_in.dog = 0;
					send_msg.arg_in.duck = 1;
					send_msg.arg_in.handler = manager_delayed_start;
					send_msg.arg_pass.cat = msg.sender;
					manager_common_send_message(SERVER_MANAGER, &send_msg);
					/*****************************/
				}
				log_qcy(DEBUG_INFO, "restart process quit status = %x", info.thread_start);
			}
			//to do: other mode
			break;
		case MSG_MANAGER_TIMER_ADD:
			if( timer_add(msg.arg_in.handler, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck, msg.sender, msg.arg_pass) )
				log_qcy(DEBUG_WARNING, "add timer failed!");
			break;
		case MSG_MANAGER_TIMER_ACK:
			( *( void(*)(message_arg_t*) ) msg.arg_in.handler )(&(msg.arg_pass));
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
						manager_common_send_message(SERVER_SCANNER, &send_msg);
//						manager_common_send_message(SERVER_SPEAKER, &send_msg);
//						manager_common_send_message(SERVER_MIIO, &send_msg);
						log_qcy(DEBUG_INFO, "---scanner success!---");
//						info.task.func = task_exit;
//						info.status = EXIT_INIT;
//						_config_.running_mode = RUNNING_MODE_EXIT;
//						info.status2 = 1;
						info.task.func = task_default;
						info.status = STATUS_NONE;
						_config_.running_mode = RUNNING_MODE_NORMAL;
						info.status2 = 0;
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
		case MSG_MANAGER_DUMMY:
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
static void task_sleep(void)
{
	message_t msg;
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
				if( misc_get_bit( info.thread_start, i)) {
					if( (i != SERVER_MIIO) )
						manager_server_stop(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			if( info.thread_start == (1<<SERVER_MIIO) ) {
				info.status = STATUS_START;
				log_qcy(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<MAX_SERVER;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							manager_common_send_message(i, &msg);
						}
					}
					log_qcy(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_deep_sleep(void)
{
	message_t msg;
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
					manager_server_stop(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			if( info.thread_start == 0 ) {
				info.status = STATUS_START;
				log_qcy(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<MAX_SERVER;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							manager_common_send_message(i, &msg);
						}
					}
					log_qcy(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_default(void)
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
			info.task.func = task_exit;
			info.status = EXIT_INIT;
			_config_.running_mode = RUNNING_MODE_EXIT;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_testing(void)
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
			info.exit = 1;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_testing = %d", info.status);
			break;
	}
}

static void task_scanner(void)
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
			info.exit = 1;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_scanner = %d", info.status);
			break;
	}
}


/****************************/
/*
 * exit: *->exit
 */
static void task_exit(void)
{
	message_t msg;
	int i;
	switch( info.status ){
		case EXIT_INIT:
			log_qcy(DEBUG_INFO,"MANAGER: switch to exit task!");
			msg_init(&msg);
			msg.message = MSG_MANAGER_EXIT;
			msg.sender = msg.receiver = SERVER_MANAGER;
			msg.arg_in.cat = info.thread_start;
			for(i=0;i<MAX_SERVER;i++) {
				if( misc_get_bit( info.thread_start, i) ) {
					manager_common_send_message(i, &msg);
				}
			}
			log_qcy(DEBUG_INFO, "sigint request get, exit code = %x", info.thread_start);
			if( info.status2==0 ) {
				/********message body********/
				msg_init(&msg);
				msg.message = MSG_MANAGER_TIMER_ADD;
				msg.sender = SERVER_MANAGER;
				msg.arg_in.cat = 10000;
				msg.arg_in.dog = 0;
				msg.arg_in.duck = 1;
				msg.arg_in.handler = &manager_kill_all;
				manager_common_send_message(SERVER_MANAGER, &msg);
			}
			/****************************/
			info.status = EXIT_SERVER;
			break;
		case EXIT_SERVER:
			if( info.thread_start == 0 ) {	//quit all
				log_qcy(DEBUG_INFO, "termination process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
				info.status = EXIT_STAGE1;
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<MAX_SERVER;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							manager_common_send_message(i, &msg);
						}
					}
					log_qcy(DEBUG_INFO, "termination process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case EXIT_STAGE1:
			server_release_1();
			info.status = EXIT_THREAD;
			break;
		case EXIT_THREAD:
			info.thread_exit = info.thread_start;
			manager_broadcast_thread_exit();
			if( !info.thread_start )
				info.status = EXIT_STAGE2;
			break;
			break;
		case EXIT_STAGE2:
			server_release_2();
			info.status = EXIT_FINISH;
			break;
		case EXIT_FINISH:
			if( info.status2 ) {
				info.status2 = 0;
				info.task.func = task_default;
				_config_.running_mode = RUNNING_MODE_NORMAL;
				info.msg_lock = 0;
			}
			else {
				info.exit = 1;
			}
			info.status = STATUS_NONE;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_exit = %d", info.status);
			break;
		}
	return;
}

static void main_thread_termination(void)
{
    message_t msg;
    /********message body********/
    msg_init(&msg);
    msg.message = MSG_MANAGER_SIGINT;
	msg.sender = msg.receiver = SERVER_MANAGER;
    /****************************/
    manager_common_send_message(SERVER_MANAGER, &msg);
}


/*
 * internal interface
 */
void manager_deep_sleep(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_deep_sleep;
	_config_.running_mode = RUNNING_MODE_DEEP_SLEEP;
	_config_.sleep.enable = 2;
}

void manager_wakeup(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_default;
	manager_send_wakeup(SERVER_MIIO);
}


/*
 * external interface
 */
int manager_init(void)
{
	int ret = 0;
	pthread_t	id;
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);
	timer_init();
    memset(&info, 0, sizeof(server_info_t));
	ret = config_manager_read(&_config_);
	if( ret )
		return -1;
	ret = pthread_create(&id, NULL, manager_timer_func, NULL);
	if(ret != 0) {
		log_qcy(DEBUG_SERIOUS, "timer create error! ret = %d",ret);
		 return ret;
	 }
	else {
		log_qcy(DEBUG_INFO, "timer create successful!");
	}
#ifdef MEMORY_POOL
    if( manager_mempool_init() )
    	return -1;
#endif
    _config_.timezone = 8;			//temporarily set to utc+8
    _config_.condition_limit = 3;	//3s
	msg_buffer_init2(&message, MSG_BUFFER_OVERFLOW_NO, &mutex);
	info.init = 1;
	//sleep timer
	sleep_init(_config_.sleep.enable, _config_.sleep.start, _config_.sleep.stop);
	//default task
	if( _config_.running_mode == RUNNING_MODE_SLEEP ) {
		info.task.func = task_sleep;
	}
	else if( _config_.running_mode == RUNNING_MODE_NORMAL ) {
		info.task.func = task_default;
	}
	else if( _config_.running_mode == RUNNING_MODE_TESTING ) {
		info.task.func = task_testing;
	}
	else if( _config_.running_mode == RUNNING_MODE_SCANNER ) {
		info.task.func = task_scanner;
	}
	//check scanner
	char fname[MAX_SYSTEM_STRING_SIZE*2];
	memset(fname,0,sizeof(fname));
	sprintf(fname, "%swifi.conf", _config_.miio_path);
	if( (_config_.running_mode == RUNNING_MODE_NORMAL) || (_config_.running_mode == RUNNING_MODE_TESTING) ) {
		if( (access(fname, F_OK))==-1) {
			_config_.running_mode = RUNNING_MODE_SCANNER;
			info.task.func = task_scanner;
		}
	}
}

int manager_proc(void)
{
	if ( !info.exit ) {
		info.old_status = info.status;
		info.task.func();
		server_message_proc();
	}
	else {
		server_release_3();
		log_qcy(DEBUG_INFO, "-----------main exit-----------");
		exit(0);
	}
}

int manager_message(message_t *msg)
{
	int ret=0;
	pthread_mutex_lock(&mutex);
	ret = msg_buffer_push(&message, msg);
	log_qcy(DEBUG_VERBOSE, "push into the manager message queue: sender=%d, message=%x, ret=%d, head=%d, tail=%d", msg->sender, msg->message, ret,
				message.head, message.tail);
	if( ret!=0 )
		log_qcy(DEBUG_WARNING, "message push in manager error =%d", ret);
	else {
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}
