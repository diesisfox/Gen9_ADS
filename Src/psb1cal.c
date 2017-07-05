/*
 * psb1cal.c
 *
 *  Created on: Jun 12, 2017
 *      Author: jamesliu
 */
#include "psb1cal.h"

//psb1ch0 linear fit: y = 32,576.492732415400x + 16,626.063780851700
//psb1ch1 linear fit: y = 82,194.619981105300x + 38,037.876712328800
//the below returns microamps

//int64_t roundivide(int32_t a, int32_t b){
//    return (((a<0)&&(b<0))||((a>=0)&&(b>=0)))?((a+(b/2))/b):((a-(b/2))/b);
//}


//returns microVolts
int32_t psb1ch0Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 - 16626063780852),32576493);
}

//returns microAmps
int32_t psb1ch1Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 - 38037876712329),82194620);
//    return (int32_t)(((int64_t)raw*1000000000-69080970588235)/(int64_t)165597927);
}
