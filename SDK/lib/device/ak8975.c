/*
 *  lib/device/ak8975.c
 *
 *  Copyright (C) 2015  Iulian Gheorghiu <morgoth.creator@gmail.com>
 *
 *  This file is part of Multiplatform SDK.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include "board_init.h"
#include "ak8975.h"
#include "api/twi_def.h"
#include "api/twi_api.h"
#include "api/timer_api.h"
#include "api/uart_api.h"


bool ak8975_start_measure(AK8975_t *structure) {
	if(!structure->TWI)
		return false;
	Twi_t *TwiStruct = structure->TWI;
	TwiStruct->MasterSlaveAddr = AK8975_ADDR | (structure->IcNr & 0x03);
	TwiStruct->TxBuff[0] = AK8975_CNTL_REG;
	TwiStruct->TxBuff[1] = AK8975_CTRL_MODE_SINGLE;
	if(!SetupI2CTransmit(TwiStruct, 2)) return false;
	return true;
}

bool ak8975_ready(AK8975_t *structure) {
	if(!structure->TWI)
		return false;
	Twi_t *TwiStruct = structure->TWI;
	TwiStruct->MasterSlaveAddr = AK8975_ADDR | (structure->IcNr & 0x03);
	TwiStruct->TxBuff[0] = AK8975_ST1_REG;
	if(!SetupI2CReception(TwiStruct, 1, 1)) return false;
	if(TwiStruct->RxBuff[0] && AK8975_ST1_DREADY_bm)
		return true;
	else
		return false;
}

bool ak8975_read_data(AK8975_t *structure, signed short *X_Axis, signed short *Y_Axis, signed short *Z_Axis) {
	if(!structure->TWI)
		return false;
	Twi_t *TwiStruct = structure->TWI;
	TwiStruct->MasterSlaveAddr = AK8975_ADDR | (structure->IcNr & 0x03);
	TwiStruct->TxBuff[0] = AK8975_HXL_REG;
	if(!SetupI2CReception(TwiStruct, 1, 6)) return false;
	*X_Axis = (signed short)((TwiStruct->RxBuff[1] << 8) + TwiStruct->RxBuff[0]);
	*Y_Axis = (signed short)((TwiStruct->RxBuff[3] << 8) + TwiStruct->RxBuff[2]);
	*Z_Axis = (signed short)((TwiStruct->RxBuff[5] << 8) + TwiStruct->RxBuff[4]);
	return true;
}

bool ak8975_get_mag(AK8975_t *structure, signed short *X_Axis, signed short *Y_Axis, signed short *Z_Axis) {
	if(!ak8975_start_measure(structure))
		return false;
	timer(Timeout);
    timer_interval(&Timeout, 11);
    timer_enable(&Timeout);
	while(!ak8975_ready(structure)) {
		if(timer_tick(&Timeout))
			return false;
	}
	if(!ak8975_read_data(structure, X_Axis, Y_Axis, Z_Axis))
		return false;
	return true;
}

bool ak8975_display_result(AK8975_t *structure) {
	signed short Xg = 0;
	signed short Yg = 0;
	signed short Zg = 0;
	if(!ak8975_start_measure(structure))
		return false;
	timer(Timeout);
    timer_interval(&Timeout, 11);
    timer_enable(&Timeout);
	while(!ak8975_ready(structure)) {
		if(timer_tick(&Timeout))
			return false;
	}
	if(!ak8975_read_data(structure, &Xg, &Yg, &Zg))
		return false;
	UARTprintf(DebugCom, "AK8975: Magnetometer: Xg = %d, Yg = %d, Zg = %d\n\r", Xg, Yg, Zg);
	return true;
}


