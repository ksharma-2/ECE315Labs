/*
 * uart_driver.h
 *
 *  Created on: July 27, 2021
 *  Modified on: February 11, 2023
 *      Author: Shyama M. Gandhi
 *
 */

#ifndef SRC_UART_DRIVER_H_
#define SRC_UART_DRIVER_H_

#include "initialization.h"

void Task_UART_buffer_receive(void *p);
void Task_UART_buffer_send(void *p);

QueueHandle_t xQueue_for_transmit;	//queue handle for transmit direction queue
QueueHandle_t xQueue_for_receive;	//queue handle for receive direction queue
int CountRxIrq;						//variable to count the receive side interrupts
int CountTxIrq;						//variable to count the transmit side interrupts

//Interrupt handler that takes higher priority during its execution.
void Interrupt_Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
	u8 receive_buffer;
	u8 transmit_data;
	u32 mask;
	if (Event == XUARTPS_EVENT_RECV_DATA){
			BaseType_t xHigherPriorityTaskWoken;
			xHigherPriorityTaskWoken = pdFALSE;

			/*******************************************************/
			//add one line of code to increment the receive interrupt counter variable

			CountRxIrq++;

			/*******************************************************/

			/*******************************************************/
			//implement the logic to check if there are received bytes from the UART. If yes, then read them and send it to the back of the receive queue.
			//Make sure to use proper queue function in the ISR! That is use the FromISR queue methods to secure the queues for ISR.
			//the UART received byte will be read inside the u8 "receive_buffer" variable already declared for you!
			while (XUartPs_IsReceiveData(XPAR_XUARTPS_0_BASEADDR) == pdTRUE) {
				receive_buffer = XUartPs_ReadReg(XPAR_XUARTPS_0_BASEADDR, XUARTPS_FIFO_OFFSET);
				xQueueSendToBackFromISR(xQueue_for_receive, &receive_buffer, &xHigherPriorityTaskWoken);
			}


			/*******************************************************/
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	else if (Event == XUARTPS_EVENT_SENT_DATA){
		BaseType_t xHigherPriorityTaskWoken;
		xHigherPriorityTaskWoken = pdFALSE;

		/*******************************************************/
		//add one line of code to increment the transmit interrupt counter variable
		CountTxIrq++;
		/*******************************************************/

			//case when there are bytes inside the transmit byte and TxFIFO is not Full yet!
			while (uxQueueMessagesWaitingFromISR( xQueue_for_transmit ) > 0 && !XUartPs_IsTransmitFull(XPAR_XUARTPS_0_BASEADDR)){
				/*******************************************************/
				//write two lines of code to read the data from the front of the transmit queue and then send it to the UART.
				//Make sure to use proper queue function in the ISR! That is use the FromISR queue methods to secure the queues for ISR.
				//the data is read into the variable "transmit_data" variable from the front of the transmit queue. Now, send this data to the UART using an appropriate function.
				xQueueReceiveFromISR(xQueue_for_transmit, &transmit_data, &xHigherPriorityTaskWoken);
				XUartPs_WriteReg(XPAR_XUARTPS_0_BASEADDR, XUARTPS_FIFO_OFFSET, transmit_data);
				/*******************************************************/
			}
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			if (uxQueueMessagesWaitingFromISR( xQueue_for_transmit ) <= 0){
				//read the current interrupt mask
				mask = XUartPs_GetInterruptMask(&UART);

				/*******************************************************/
				//Use the XUartPs_SetInterruptMask() function to disable the TEMPTY interrupt.
				XUartPs_SetInterruptMask(&UART, (mask & ~(XUARTPS_IXR_TXEMPTY)));
				/*******************************************************/
			}


	}
	else{
			xil_printf("Nor a RECEIVE event nor a SEND event\n");
	}
}


//Be sure to protect any critical sections into these four functions by using the FreeRTOS macros taskENTER_CRITICAL() and taskEXIT_CRITICAL().


u8 MyReceiveFunction( void ){

	taskENTER_CRITICAL();

	u8 recv;
	/*******************************************************/
	//Check if there is any data in the receive direction queue.
	//Suggestion: use a while loop to check if there is data inside receive queue. If yes, you want to read the data from the queue.

	if (uxQueueMessagesWaiting(xQueue_for_receive) > 0){
		xQueueReceive(xQueue_for_receive, &recv, portMAX_DELAY);
	}

	/*******************************************************/

	taskEXIT_CRITICAL()

	return recv;
}

void MySendFunction( u8 Data ) {

	u32 mask;

	taskENTER_CRITICAL();

	//check if space in the queue
	while(uxQueueSpacesAvailable(xQueue_for_transmit) <= 0){
		//keep on looping until there is a space in transmit queue.
	}

	/*******************************************************/
	//add the code to enable TEMPTY interrupt bit. You can first read the interrupt mask and then set the interrupt mask using the XUARTPS_IXR_TXEMPTY.
	mask = XUartPs_GetInterruptMask(&UART);
	mask = mask | XUARTPS_IXR_TXEMPTY;
	XUartPs_SetInterruptMask(&UART, mask);
	/*******************************************************/

	//if transmit FIFO empty, use polling, otherwise insert to queue for interrupt method
	if (XUartPs_IsTransmitEmpty(&UART)){
		XUartPs_WriteReg(XPAR_XUARTPS_0_BASEADDR, XUARTPS_FIFO_OFFSET, Data);
	}else if(xQueueSendToBack( xQueue_for_transmit, &Data, 0UL) != pdPASS ){
		xil_printf("Fail to send the data\n");
	}

	taskEXIT_CRITICAL();

}


#endif /* SRC_UART_DRIVER_H_ */

