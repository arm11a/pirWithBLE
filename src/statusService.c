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

#include <ls_types.h>
#include <ls_app_if.h>
#include <types.h>
#include <timer.h>
#include <pio.h>
#include "beacon.h"
#include "statusService.h"


static PirPacketData PirPacket;
static timer_id timer_pirStop;
static timer_id timer_m1;
static timer_id timer_s6;
static int timeleft_6s = 0;
static int totalpirCount = 0;
static int pirMissingCountPer1M = 0;
static uint32 packetCount = 0;

static void pirPacketIncrease(PirPacketData* packet);
static void timerCallback_pirStop(timer_id id);
static void timerCallback_s6(timer_id id);

#define SEC_6	5800*MILLISECOND
#define MIN_1	1*MINUTE
#define MAX_PIR_MISSING_COUNT	10


static void pirPacketInit(PirPacketData* packet)
{
	uint8 i =0;
	//memset(packet,0,sizeof(PirPacketData));
	if(packet== NULL)
		packet = &PirPacket;

	for(i=0; i<sizeof(PirPacketData);i++)
	{
		((uint8*)packet)[i] = 0;
	}

	packet->adType = AD_TYPE_MANUF;
#if 0
	packet->compType[0] = 01;
	packet->compType[1] = 02;
	packet->packetNum = 14;
	packet->pir[0] = 1;
	packet->pir[1] = 1;
	packet->pir[2] = 1;
	packet->pir[3] = 1;
	packet->pir[4] = 7;
	packet->pir[9] = 19;
#endif
}

static void pirPacketSend(PirPacketData* packet)
{
    /* store the advertisement data */
	uint8 i = 0;
	static PirPacketData sendPacket;
	for(i=0; i<sizeof(PirPacketData);i++)
	{
		((uint8*)&sendPacket)[i] = ((uint8*)packet)[i];
	}
    pirPacketInit(NULL);
    sendPacket.packetNum = packetCount;
    LsStoreAdvScanData(0, NULL, ad_src_advertise);

    LsStoreAdvScanData(DEFAULT_ADVERT_PAYLOAD_SIZE + 3, (uint8*)&sendPacket, ad_src_advertise);

    /* Start broadcasting */
    LsStartStopAdvertise(TRUE, whitelist_disabled, ls_addr_type_public);

    timer_pirStop = TimerCreate(1*SECOND,TRUE,timerCallback_pirStop);
    packetCount++;
}

static void pirPacketStop(void)
{
    LsStartStopAdvertise(FALSE, whitelist_disabled, ls_addr_type_public);
}


static void pirPacketIncrease(PirPacketData* packet)
{
	totalpirCount++;
	packet->pir[timeleft_6s]++;
}
static void timerCallback_pirStop(timer_id id)
{
	pirPacketStop();
}

static void timerCallback_s6(timer_id id)
{
	if(PioGet(PIR_SIGNAL))
	{
		if(PirPacket.pir[timeleft_6s] == 0)
		{
			PirPacket.pir[timeleft_6s] = 1;
		}
	}
	timeleft_6s++;
	if(timeleft_6s <10)
	{
		timer_s6 = TimerCreate(SEC_6,TRUE,timerCallback_s6);
	}else
	{
		timer_s6 = NULL;
	}
}
static void timerCallback_m1(timer_id id)
{
	if(timer_s6 != NULL)
	{
		TimerDelete(timer_s6);
	}
	if(totalpirCount == 0)
		pirMissingCountPer1M++;
	else
		pirMissingCountPer1M = 0;

	if(pirMissingCountPer1M < MAX_PIR_MISSING_COUNT )
	{
		pirPacketSend(&PirPacket);
		timer_s6 = TimerCreate(SEC_6,TRUE,timerCallback_s6);
		timer_m1 = TimerCreate(MIN_1,TRUE,timerCallback_m1);
	}else
	{
		timer_s6 = NULL;
		timer_m1 = NULL;
		pirMissingCountPer1M = 0;
		packetCount = 0;
	}
	timeleft_6s = 0 ;
	totalpirCount = 0;
}

void pirDetected(void)
{
	if(timer_m1 != NULL)
	{
		pirPacketIncrease(&PirPacket);
	}
	else
	{
		pirMissingCountPer1M = 0;
		packetCount = 0;
		pirPacketInit(&PirPacket);
		pirPacketSend(&PirPacket);
		timer_m1 = TimerCreate(MIN_1,TRUE,timerCallback_m1);
		timer_s6 = TimerCreate(SEC_6,TRUE,timerCallback_s6);
	}
}

void pirStatusServiceInit(void)
{
	pirPacketInit(&PirPacket);
}

