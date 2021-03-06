/*
 * twi_interface.c
 *
 *  Created on: Dec 3, 2012
 *      Author: XxXx
 */
/*#####################################################*/
#include <stdbool.h>
#include <stdlib.h>
#include "twi_interface.h"
#include "api/twi_def.h"
#include "aintc/aintc_twi.h"
#include "pinmux/pin_mux_twi.h"
#include "clk/clk_twi.h"
#include "driver/hsi2c.h"
/*#####################################################*/
/*
** Transmits data over I2C bus
*/
bool _SetupI2CTransmit(new_twi* TwiStruct, unsigned int TransmitBytes)
{
    if(!TwiStruct) return false;
	/* Set i2c slave address */
    I2CMasterSlaveAddrSet(TwiStruct->BaseAddr, TwiStruct->MasterSlaveAddr);

    if(TwiStruct->WithInterrupt)
    {
        /* Data Count specifies the number of bytes to be transmitted */
        I2CSetDataCount(TwiStruct->BaseAddr, TransmitBytes);

        TwiStruct->numOfBytes = I2CDataCountGet(TwiStruct->BaseAddr);

        /* Clear status of all interrupts */
        CleanUpInterrupts(TwiStruct);

        /* Configure I2C controller in Master Transmitter mode */
        I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_TX | I2C_CFG_STOP);

        /* Transmit interrupt is enabled */
        I2CMasterIntEnableEx(TwiStruct->BaseAddr, I2C_INT_TRANSMIT_READY | I2C_INT_STOP_CONDITION | I2C_INT_NO_ACK);

        TwiStruct->error_flag = 0;

        /* Generate Start Condition over I2C bus */
        I2CMasterStart(TwiStruct->BaseAddr);

        while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

        while(TwiStruct->flag);

        if(TwiStruct->error_flag) return false;

        while(I2CMasterBusy(TwiStruct->BaseAddr));

        /* Wait untill I2C registers are ready to access */
        while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_ADRR_READY_ACESS));

        TwiStruct->flag = 1;
		return true;
    }
    else
    {
		I2CSetDataCount(TwiStruct->BaseAddr, TransmitBytes);

		CleanUpInterrupts(TwiStruct);

		I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_TX);

        I2CMasterStart(TwiStruct->BaseAddr);

		while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

		while((I2C_INT_TRANSMIT_READY == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_TRANSMIT_READY))
			   && TransmitBytes--)
		{
        	if(I2C_INT_NO_ACK == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_NO_ACK))
        	{
        		I2CMasterStop(TwiStruct->BaseAddr);

        		while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_STOP_CONDITION));

        		I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_STOP_CONDITION | I2C_INT_NO_ACK);

        		return false;
        	}

        	I2CMasterDataPut(TwiStruct->BaseAddr, TwiStruct->TxBuff[TwiStruct->tCount++]);

			I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_TRANSMIT_READY);
		}

		I2CMasterStop(TwiStruct->BaseAddr);

		while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_STOP_CONDITION));

		I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_STOP_CONDITION);
		return true;
    }
}
/*#####################################################*/
/*
** Receives data over I2C bus
*/
bool _SetupI2CReception(new_twi* TwiStruct, unsigned int TransmitBytes, unsigned int ReceiveBytes)
{
    if(!TwiStruct) return false;
	/* Set i2c slave address */
    I2CMasterSlaveAddrSet(TwiStruct->BaseAddr, TwiStruct->MasterSlaveAddr);

    if(TwiStruct->WithInterrupt)
    {
		TwiStruct->error_flag = 0;
		/* Data Count specifies the number of bytes to be transmitted */
		I2CSetDataCount(TwiStruct->BaseAddr, TransmitBytes);

		TwiStruct->numOfBytes = I2CDataCountGet(TwiStruct->BaseAddr);

		/* Clear status of all interrupts */
		CleanUpInterrupts(TwiStruct);

		/* Configure I2C controller in Master Transmitter mode */
		I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_TX);

		/* Transmit interrupt is enabled */
		I2CMasterIntEnableEx(TwiStruct->BaseAddr, I2C_INT_TRANSMIT_READY | I2C_INT_NO_ACK);

		/* Generate Start Condition over I2C bus */
		I2CMasterStart(TwiStruct->BaseAddr);

		while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

		while(TwiStruct->tCount != TwiStruct->numOfBytes);


		TwiStruct->flag = 1;

		/* Wait untill I2C registers are ready to access */
		while(!(I2CMasterIntRawStatus(TwiStruct->BaseAddr) & (I2C_INT_ADRR_READY_ACESS)));

		/* Data Count specifies the number of bytes to be received */
		I2CSetDataCount(TwiStruct->BaseAddr, ReceiveBytes);

		TwiStruct->numOfBytes = I2CDataCountGet(TwiStruct->BaseAddr);

		CleanUpInterrupts(TwiStruct);

		/* Configure I2C controller in Master Receiver mode */
		I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_RX);

		/* Receive and Stop Condition Interrupts are enabled */
		I2CMasterIntEnableEx(TwiStruct->BaseAddr,  I2C_INT_RECV_READY |
											  I2C_INT_STOP_CONDITION | I2C_INT_NO_ACK);

		/* Generate Start Condition over I2C bus */
		I2CMasterStart(TwiStruct->BaseAddr);

		do
		{
			if(TwiStruct->error_flag) return false;
		}
		while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

		do
		{
			if(TwiStruct->error_flag) return false;
		}
		while(TwiStruct->flag);

		TwiStruct->flag = 1;

		return true;
    }
    else
    {
        I2CSetDataCount(TwiStruct->BaseAddr, TransmitBytes);

        CleanUpInterrupts(TwiStruct);

        I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_TX);

		I2CMasterStart(TwiStruct->BaseAddr);

        while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

        while((I2C_INT_TRANSMIT_READY == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_TRANSMIT_READY))
               && TransmitBytes--)
        {
        	if(I2C_INT_NO_ACK == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_NO_ACK))
        	{
                I2CMasterStop(TwiStruct->BaseAddr);

        		while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_STOP_CONDITION));

                I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_STOP_CONDITION | I2C_INT_NO_ACK);

        		return false;
        	}

        	I2CMasterDataPut(TwiStruct->BaseAddr, TwiStruct->TxBuff[TwiStruct->tCount++]);

            I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_TRANSMIT_READY);
        }
        while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_ADRR_READY_ACESS));

        I2CSetDataCount(TwiStruct->BaseAddr, ReceiveBytes);

        CleanUpInterrupts(TwiStruct);

        I2CMasterControl(TwiStruct->BaseAddr, I2C_CFG_MST_RX);

		I2CMasterStart(TwiStruct->BaseAddr);

        while(I2CMasterBusBusy(TwiStruct->BaseAddr) == 0);

        while((ReceiveBytes--))
        {
            do
            {
            	if(I2C_INT_NO_ACK == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_NO_ACK))
            	{
                    I2CMasterStop(TwiStruct->BaseAddr);

            		while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_STOP_CONDITION));

                    I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_STOP_CONDITION | I2C_INT_NO_ACK);

            		return false;
            	}
            }
        	while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_RECV_READY));

            TwiStruct->RxBuff[TwiStruct->rCount++] = I2CMasterDataGet(TwiStruct->BaseAddr);

            I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_RECV_READY);
        }

        I2CMasterStop(TwiStruct->BaseAddr);

		while(0 == (I2CMasterIntRawStatus(TwiStruct->BaseAddr) & I2C_INT_STOP_CONDITION));

        I2CMasterIntClearEx(TwiStruct->BaseAddr, I2C_INT_STOP_CONDITION);

		return true;
    }
}
/*#####################################################*/
bool _twi_open(new_twi* TwiStruct)
{
	unsigned char *RxBuff;
	unsigned char *TxBuff;
	if(TwiStruct->RxBuffSize)
	{
		RxBuff = calloc(1, TwiStruct->RxBuffSize);
		if(RxBuff == NULL) return 0;
		TwiStruct->RxBuff = RxBuff;
	}
	if(TwiStruct->TxBuffSize)
	{
		TxBuff = calloc(1, TwiStruct->TxBuffSize);
		if(TxBuff == NULL ) return 0;
		TwiStruct->TxBuff = TxBuff;
	}
    //new_twi* TwiStruct = new_(new_twi);
	TWIModuleClkConfig(TwiStruct->TwiNr);
    //if(TwiStruct == NULL) return NULL;
    if(TwiStruct->WithInterrupt)I2CAINTCConfigure(TwiStruct);
    switch(TwiStruct->TwiNr)
    {
    case 0:
		TwiStruct->BaseAddr = I2C0_addr;
		Twi_Int_Service.Twi0 = TwiStruct;
    	break;
    case 1:
		TwiStruct->BaseAddr = I2C1_addr;
		Twi_Int_Service.Twi1 = TwiStruct;
    	break;
    case 2:
		TwiStruct->BaseAddr = I2C2_addr;
		Twi_Int_Service.Twi2 = TwiStruct;
    	break;
    case 3:
		TwiStruct->BaseAddr = I2C3_addr;
		Twi_Int_Service.Twi2 = TwiStruct;
    	break;
    }
	pin_mux_twi_sda(TwiStruct->SdaPin);
	pin_mux_twi_scl(TwiStruct->SclPin);


    //I2CPinMuxSetup(TwiStruct->TwiNr);
    /* Put i2c in reset/disabled state */
    I2CMasterDisable(TwiStruct->BaseAddr);

    if(TwiStruct->WithInterrupt) I2CSoftReset(TwiStruct->BaseAddr);
    /* Disable auto Idle functionality */
    I2CAutoIdleDisable(TwiStruct->BaseAddr);

    I2CIdleModeSelect(TwiStruct->BaseAddr, I2C_SYSC_IDLEMODE_NO_IDLE);

    //I2CFIFOThresholdConfig(TwiStruct->BaseAddr, 1, I2C_TX_MODE);
    //I2CFIFOThresholdConfig(TwiStruct->BaseAddr, 1, I2C_RX_MODE);

    /* Configure i2c bus speed to 100khz */
    I2CMasterInitExpClk(TwiStruct->BaseAddr, 96000000, 12000000, TwiStruct->BaudRate);

    /* Bring I2C out of reset */
    I2CMasterEnable(TwiStruct->BaseAddr);
    while(!I2CSystemStatusGet(TwiStruct->BaseAddr));
    return true;
}
/*#####################################################*/
void _twi_close(new_twi* TwiStruct)
{
    I2CMasterDisable(TwiStruct->BaseAddr);
    TWIModuleClkUnConfig(TwiStruct->TwiNr);
    I2CAINTCUnConfigure(TwiStruct);
    switch(TwiStruct->TwiNr)
    {
    case 0:
		Twi_Int_Service.Twi0 = NULL;
    	break;
    case 1:
		Twi_Int_Service.Twi1 = NULL;
    	break;
    case 2:
		Twi_Int_Service.Twi2 = NULL;
    	break;
    case 3:
		Twi_Int_Service.Twi3 = NULL;
    	break;
    }
    if(TwiStruct->RxBuffSize) free((void*)TwiStruct->RxBuffSize);
    if(TwiStruct->TxBuffSize) free((void*)TwiStruct->TxBuffSize);
    if(TwiStruct) free(TwiStruct);
}
/*#####################################################*/


