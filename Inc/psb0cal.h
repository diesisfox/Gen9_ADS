/*
 * psb0cal.h
 *
 *  Created on: Jun 13, 2017
 *      Author: jamesliu
 *
 *  Misc. functions that groups specific set of actions in a friendlier way
 */

#ifndef PSB0CAL_H_
#define PSB0CAL_H_

#include "main.h"
#include "nodeConf.h"
#include "stm32f4xx_hal.h"

#ifndef PSB_OA
#define PSB_OA		4000000		//uA
#endif
#ifndef PSB_UA
#define PSB_UA		-4000000	//uA
#endif
#ifndef PSB_OV
#define PSB_OV		140000000	//uV
#endif
#ifndef PSB_UV
#define PSB_UV		80000000	//uV
#endif

#define roundivide(a,b) (((((a)<0)&&((b)<0))||(((a)>=0)&&((b)>=0)))?(((a)+((b)/2))/(b)):(((a)-((b)/2))/(b)))

int32_t psb0ch0Map(int32_t raw);
int32_t psb0ch1Map(int32_t raw);
int32_t psb0ch2Map(int32_t raw);
int32_t psb0ch3Map(int32_t raw);
int32_t psb0ch4Map(int32_t raw);
int32_t psb0ch5Map(int32_t raw);

#endif /* PSB0CAL_H_ */
