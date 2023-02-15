/*
 * initialization.h
 *
 *  Created on: Feb 11, 2022
 *      Author: Shyama Gandhi
 *      References: Xilinx UART and interrupt drivers files
 */


#include "xil_io.h"
#include "xuartps.h" //UART definitions header file
#include "xscugic.h" //interrupt controller header file

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//UART Interrupt definitions
#define INTC 				XScuGic						 	//Interrupt controller
#define UART_DEVICE_ID 		XPAR_XUARTPS_0_DEVICE_ID 		//UART Device ID
#define INTC_DEVICE_ID 		XPAR_SCUGIC_SINGLE_DEVICE_ID	//Interrupt controller device ID
#define UART_INT_IRQ_ID 	XPAR_XUARTPS_1_INTR 			//UART interrupt identifier
#define UART_RX_BUFFER_SIZE	3U								//3 bytes from host
#define SIZE_OF_QUEUE		100


//UART interrupt control ISR declaration
void Interrupt_Handler(void *CallBackRef, u32 Event, unsigned int EventData);

XUartPs UART; 				//UART Instance
XUartPs_Config *Config; 	//Pointer to UART
INTC InterruptController; 	//Interrupt controller instance
u32 IntrMask;				//interrupt mask variable to be used to enable different type of interrupt on Rx and Tx in UART
