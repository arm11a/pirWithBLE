/*
    Copyright (c) 2015-2016, Raven( craven@crowz.kr )
    All rights reserved.
    
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
 */


#include <stdint.h>
#include <string.h>
#include "statusService.h"
#include <ls_types.h>
#include <timer.h>
#include "beacon.h"


static PirPacketData PirPacket;
static timer_id timer_m1;


void ppirPacketSend(PirPacketData* packet)
{
    /* store the advertisement data */
    LsStoreAdvScanData(DEFAULT_ADVERT_PAYLOAD_SIZE + 3, (uint8_t*)packet, ad_src_advertise);

    /* Start broadcasting */
    LsStartStopAdvertise(TRUE, whitelist_disabled, addressType);
}

void pirPacketInit(PirPacketData* packet)
{
	memset(packet,0,sizeof(PirPacketData));
	packet->adType = AD_TYPE_MANUF;
}

void pirPacketIncrease(PirPacketData* packet)
{
	int pirNum = 0;
	///@todo Timer Check and slice 1m/6

	packet->pir[pirNum]++;
}

void pirDetected(void)
{
	if(timer_m1 != NULL)
		pirPacketIncrease(&PirPacket);
	else
		;
}


