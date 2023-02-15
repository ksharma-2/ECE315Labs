/*
 * initialization.c
 *
 *  Created on: Feb 12, 2022
 *      Author: Shyama Gandhi
 *      References: Xilinx UART and interrupt drivers files
 */
# include "initialization.h"

//this function initializes and configures the UART

//this function initializes and configures the UART
int Initialize_UART(){

	int Status;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);
	if (NULL == Config)
	{
	return XST_FAILURE;
	xil_printf("UART PS Config failed\n");
	}

	//Initialize UART
	Status = XUartPs_CfgInitialize(&UART, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS)
	{
	return XST_FAILURE;
	xil_printf("UART PS init failed\n");
	}

	return XST_SUCCESS;
}

//Function for interrupt system setup
int SetupInterruptSystem(INTC *IntcInstancePtr, XUartPs *UartInstancePtr, u16 UartIntrId){
	int Status;
	XScuGic_Config *IntcConfig; // Config pointer for interrupt controller

	//Lookup the config information for interrupt controller
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig)
	{
	return XST_FAILURE;
	}

	//Initialize interrupt controller
	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS)
	{
	return XST_FAILURE;
	}

	//Connect the interrupt controller interrupt handler
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, IntcInstancePtr);

	//Connect the PS UART interrupt handler
	//The interrupt handler which handles the interrupts for the UART peripheral is connected to it's unique ID number (82 in this case)
	Status = XScuGic_Connect(IntcInstancePtr, UartIntrId, (Xil_ExceptionHandler) XUartPs_InterruptHandler, (void *) UartInstancePtr);
	if (Status != XST_SUCCESS) {
	return XST_FAILURE;
	}

	//Enable the UART interrupt input on the interrupt controller
	XScuGic_Enable(IntcInstancePtr, UartIntrId);

	//Enable the processor interrupt handling on the ARM processor
	Xil_ExceptionEnable();


	//Setup the UART Interrupt handler function
	XUartPs_SetHandler(UartInstancePtr, (XUartPs_Handler)Interrupt_Handler, UartInstancePtr);

	//Create mask for UART interrupt, Enable the interrupt when the receive buffer has reached a particular threshold
	 IntrMask = XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY | XUARTPS_IXR_FRAMING | XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL | XUARTPS_IXR_RXOVR;

	//Setup the UART interrupt Mask
	XUartPs_SetInterruptMask(UartInstancePtr, IntrMask);

	//Setup the PS UART to Work in Normal Mode
	XUartPs_SetOperMode(UartInstancePtr, XUARTPS_OPER_MODE_NORMAL);

	return XST_SUCCESS;
}
