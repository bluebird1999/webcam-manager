/*
 * global.c
 *
 *  Created on: Sep 14, 2020
 *      Author: ning
 */

/*
 * header
 */
//system header
#include <stdio.h>
#include <signal.h>
#include <malloc.h>
#ifdef DMALLOC_ENABLE
#include <dmalloc.h>
#endif
//program header

//server header
#include "global_interface.h"


/*
 * global
 *
 */
manager_config_t	_config_;
elr_mpl_t 			_pool_ = ELR_MPL_INITIALIZER;
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
