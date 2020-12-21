/*
 * common.c
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
#include "manager_interface.h"
/*
 * static
 */
//variable


//specific

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
int manager_common_send_message(int receiver, message_t *msg)
{
	int st = 0;
	switch(receiver) {
		case SERVER_DEVICE:
			st = server_device_message(msg);
			break;
		case SERVER_KERNEL:
			st = server_kernel_message(msg);
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
			st = server_micloud_message(msg);
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
			log_qcy(DEBUG_SERIOUS, "unknown message target! %d FROM msg id %d", receiver, msg->message);
			break;
	}
	return st;
}

void manager_common_send_dummy(int server)
{
	message_t msg;
	msg_init(&msg);
	msg.sender = msg.receiver = server;
	msg.message = MSG_MANAGER_DUMMY;
	manager_common_send_message(server,&msg);
}

void manager_common_message_do_delivery(int receiver, message_t *msg)
{
	int num = 0;
	int ret = 0;
	ret = manager_common_send_message(receiver, &msg);
	while( (ret!=0) && (num<MESSAGE_RESENT) ) {
		ret = manager_common_send_message(receiver, &msg);
		num++;
		sleep(MESSAGE_RESENT_SLEEP);
	};
}
