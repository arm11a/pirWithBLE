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
#ifndef SRC_STATUSSERVICE_H_
#define SRC_STATUSSERVICE_H_


typedef  struct pirData
{
	uint8_t adType;
	uint8_t compType[2];
	uint8_t reserved1[10];
	uint8_t packetNum;
	uint8_t pir[10];
	uint8_t reserved[7];
}PirPacketData;


#endif /* SRC_STATUSSERVICE_H_ */
