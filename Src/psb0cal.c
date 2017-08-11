/*
 * psb0cal.c
 *
 *  Created on: Jun 13, 2017
 *      Author: jamesliu
 */
#include "psb0cal.h"

/*
psb0ch0 linear fit: y = 32,867.086170862700x + 15,232.021526172000
psb0ch1 linear fit: y = 164,015.724807117000x - 3,442.500000000020
psb0ch2 linear fit: y = 32,968.361508850600x + 11,865.598039215400
psb0ch3 linear fit: y = 323,411.068847426000x + 35,406.116438356200
psb0ch4 linear fit: y = 33,059.320186758100x - 11,774.259803921900
psb0ch5 linear fit: y = 324,696.617028814000x - 106,756.513698630000
*/

//returns microVolts
int32_t psb0ch0Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 - 15232021526172),32867086);
}

//returns microAmps
int32_t psb0ch1Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 + 3442500000000),164015725);
//    return (int32_t)(((int64_t)raw*1000000000-69080970588235)/(int64_t)165597927);
}

//returns microVolts
int32_t psb0ch2Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 - 11865598039215),32968362);
}

//returns microAmps
int32_t psb0ch3Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 - 35406116438356),323411069);
//    return (int32_t)(((int64_t)raw*1000000000-69080970588235)/(int64_t)165597927);
}

//returns microVolts
int32_t psb0ch4Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 + 11774259803922),33059320);
}

//returns microAmps
int32_t psb0ch5Map(int32_t raw){
    if(raw&0x800000) raw -= 0x1000000;
	return (int32_t)roundivide(((int64_t)raw*1000000000 + 106756513698630),324696617);
//    return (int32_t)(((int64_t)raw*1000000000-69080970588235)/(int64_t)165597927);
}
