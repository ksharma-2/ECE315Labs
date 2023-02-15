/*
 *	Lab 2: Part 2: UART in INterrupt Mode
 *
 *	ECE 315		: Computer Interfacing
 *  Created on	: July 27, 2021
 *  Modified on : February 11, 2023
 *      Author	: Shyama M. Gandhi, Mazen Elbaz
 *
 *     ------------------------------------------------------------------------------------------------------------------------------
 *     Overview of this main source .c file:
 *     The driver file does all the background work and the queue implementation, which is hidden from the user.
 *     This file will also have the logic responsible for detecting the "\r#\r" and "\r%\r" termination sequence, for displaying the interrupt statistics and resetting the variables respectively.
 *     ------------------------------------------------------------------------------------------------------------------------------
 */


#include "stdio.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xtime_l.h"

//this is the uart driver file where students will add the implementation as mentioned in the lab manual
#include "uart_driver.h"

#define CHAR_HASH				0x23	/* '#' character is used as termination sequence */
#define CHAR_CARRIAGE_RETURN	0x0D	/* '\r' character is used in the termination sequence */

#define CHAR_PERCENT			0x25	/* '%' character is used as termination sequence */

void printString(char countMessage[]);

TaskHandle_t task_receiveuarthandle = NULL;
TaskHandle_t task_transmituarthandle = NULL;
BaseType_t resetFlag;
BaseType_t endingSeq;
int Countbytes; 								// to count the number of bytes received over the UART
extern QueueHandle_t xQueue_for_transmit;
extern QueueHandle_t xQueue_for_receive;
extern int CountRxIrq;							//variable to count the receive side interrupts
extern int CountTxIrq;							//variable to count the transmit side interrupts

//Function declaration for UART interrupt setup
extern int SetupInterruptSystem(INTC *IntcInstancePtr, XUartPs *UartInstancePtr, u16 UartIntrId);
//Initialization function for UART
extern int Initialize_UART();

// two new driver functions
u8 MyReceiveFunction( void );
void MySendFunction (u8 Data);


int main()
{
	int Status;

	xTaskCreate(Task_UART_buffer_receive, "uart_receive_task", 2048, (void*)0, tskIDLE_PRIORITY, &task_receiveuarthandle);
	xTaskCreate(Task_UART_buffer_send, "uart_transmit_task", 2048, (void*)0, tskIDLE_PRIORITY, &task_transmituarthandle);

	xQueue_for_transmit = xQueueCreate( SIZE_OF_QUEUE, sizeof( u8 ) );
	xQueue_for_receive  = xQueueCreate( SIZE_OF_QUEUE, sizeof( u8 ));
	CountRxIrq = 0;
	CountTxIrq = 0;
	Countbytes = 0;
	resetFlag = pdFALSE;
	endingSeq = pdFALSE;

	Status = Initialize_UART();
	if (Status != XST_SUCCESS){
		xil_printf("UART Initialization failed\n");
	}


	vTaskStartScheduler();

	while(1);

	return 0;

}


void Task_UART_buffer_receive(void *p){

	int Status;

	Status = SetupInterruptSystem(&InterruptController, &UART, UART_INT_IRQ_ID);
	if (Status != XST_SUCCESS){
		xil_printf("UART PS interrupt failed\n");
	}

	u8 returnFlag = 0;
	u8 restartFlag = 0;


	for( ;; ){
		while(1){
				u8 pcString;
				char write_to_queue_value;

				/*******************************************************/
				//add one line to call user defined receive function to return the received data.
				//use "pcString" variable to store the data.
				pcString = MyReceiveFunction();
				/*******************************************************/

				write_to_queue_value = (char) pcString;	//casted to "char" type.

				/*******************************************************/
				//write the code to change the capitalization from lower to upper case and vice versa for the byte present inside the
				//"write_to_queue_value" variable.
				//store the changed byte to "write_to_queue_value" itself.

				if (write_to_queue_value >= 'a' && write_to_queue_value <= 'z' ) {
					write_to_queue_value = toupper(write_to_queue_value);
				} else if (write_to_queue_value >= 'A' && write_to_queue_value <= 'Z'){
					write_to_queue_value = tolower(write_to_queue_value);
				}

				/*******************************************************/

				/*******************************************************/
				//write one line of code to increment the variable used as a byte counter for UART characters

				Countbytes++;

				/*******************************************************/

				MySendFunction(write_to_queue_value);

				//detect \r#\r
				if (returnFlag == 2 && write_to_queue_value == CHAR_CARRIAGE_RETURN){
					returnFlag = 0;
					taskYIELD(); //force context switch
				}else if (returnFlag == 1 && write_to_queue_value == CHAR_HASH){
					returnFlag = 2;
				}else if (write_to_queue_value == CHAR_CARRIAGE_RETURN){
					returnFlag = 1;
				}else{
					returnFlag = 0;
				}

				/*******************************************************/
				//detect \r%\r sequence here.
				//Once you have detected this sequence, reset the byte counter, transmit interrupt counter and receive interrupt counter to zero.
				//Also, print this message on the console: "Byte Counter, CountRxIrq && CountTxIrq set to zero\n\n"
				//the counter variables have been already given to you.
				//note that the received character byte is stored inside "write_to_queue_value" variable.

				if (restartFlag == 2 && write_to_queue_value == CHAR_CARRIAGE_RETURN){
					restartFlag = 0;
					Countbytes = 0;
					CountRxIrq = 0;
					CountTxIrq = 0;
					xil_printf("Byte Counter, CountRxIrq && CountTxIrq set to zero \n\n");
				}else if (restartFlag == 1 && write_to_queue_value == CHAR_PERCENT){
					restartFlag = 2;
				}else if (write_to_queue_value == CHAR_CARRIAGE_RETURN){
					restartFlag = 1;
				}else{
					restartFlag = 0;
				}
				/*******************************************************/
			}
	}
}

//print the provided number using driver functions
void printNumber(char number[]){
	for (int i = 0; i < 10; i++){
		if (number[i] >= '0' && number[i] <= '9'){
			MySendFunction(number[i]);
		}
	}
}

//print the provided string using driver functions
void printString(char countMessage[]){
	for (int i = 0; countMessage[i] != '\0'; i++){
		MySendFunction(countMessage[i]);
	}
}

void Task_UART_buffer_send(void *p){


	for( ;; ) {

		while(1){

			//print the special messages for ending sequence
			char countArray[10];
			char CountRxIrqArray[10];
			char CountTxIrqArray[10];
			sprintf(countArray,"%d", Countbytes);
			sprintf(CountRxIrqArray,"%d", CountRxIrq);
			sprintf(CountTxIrqArray,"%d", CountTxIrq);
			printString("Bytes processed: ");
			printNumber(countArray);
			MySendFunction(CHAR_CARRIAGE_RETURN);
			printString("Rx interrupts: ");
			printNumber(CountRxIrqArray);
			MySendFunction(CHAR_CARRIAGE_RETURN);
			printString("Tx interrupts: ");
			printNumber(CountTxIrqArray);
			MySendFunction(CHAR_CARRIAGE_RETURN);


			//perform context switch
			taskYIELD();
		}
	}
}
