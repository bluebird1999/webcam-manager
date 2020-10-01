/*
 * main.c
 *
 *  Created on: Aug 20, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <stdio.h>
#include <signal.h>
//program header
#include "manager_interface.h"
#include "../tools/tools_interface.h"
#include "watchdog_interface.h"
//server header

/*
 * static
 */
//variable

//function;



/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static void main_thread_termination(void)
{
	log_info("main ended!");
}

/*
 * 	Main function, entry point
 *
 * 		NING,  2020-08-13
 *
 */
int main(int argc, char *argv[])
{
	log_info("started!");
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);

/*
 * main loop
 */
	while(1) {
	/*
	 * manager proc
	 */
		manager_proc();
	/*
	 *  watch-dog proc
	 */
		watchdog_proc();
	}
//---unexpected catch here---
	log_info("-----------thread exit: main-----------");
	return 1;
}
