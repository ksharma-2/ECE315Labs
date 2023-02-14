/*
 * ECE 315 - Winter 2023
 * Lab 2 : Cipher Decipher System
 * Created By: Shyama Gandhi, February 2023
 *
 * Copyrights @ 2023, Department of Electrical and Computer Engineering, University of Alberta
 */

#include "xparameters.h"
#include "xuartps.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cipher_lookup_table.h"

#define UART_BASEADDR 	XPAR_XUARTPS_0_BASEADDR
#define QUEUE_LENGTH 	400
#define QUEUE_ITEM_SIZE sizeof(char)

#define UART_DEVICE_ID  XPAR_XUARTPS_0_DEVICE_ID
#define UART_BASEADDR	XPAR_XUARTPS_0_BASEADDR

//shift parameter as a key to cipher and decipher the text
#define SHIFT_KEY 		2

XUartPs uart;			/* Instance of the UART Device */
XUartPs_Config *Config;	/* The instance of the UART-PS Config */
XUartPs UartPs;

// variable declaration for Cipher/Decipher selection
typedef enum {CIPHER, DECIPHER} Operation;
Operation opr;

QueueHandle_t input_queue;
QueueHandle_t output_queue;

char output_text_sequence[QUEUE_LENGTH];


void vInputTask(void *pvParameters) {

    char input;
    char option;

    while (1)
    {
        // display menu to select the option of cipher or decipher
    	xil_printf("\r*******************************************\r");
    	xil_printf("\rMenu:\n\r1. Cipher\r2. Decipher\n\r");
        xil_printf("Enter your option: ");

        while(1){
			// read option from UART terminal
        	option = XUartPs_RecvByte(UART_BASEADDR);

			// Parse the user input
			if (option == '1') {
				opr = CIPHER;
				break;
			} else if (option == '2') {
				opr = DECIPHER;
				break;
			} else {
				continue;
			}
        }

        // read input from UART terminal
        xil_printf("%c\r", option);
        xil_printf("\rEnter input message: ");
        input = XUartPs_RecvByte(UART_BASEADDR);


		/**************************************************/
		// write the code to read the input from UART and store in "input" variable.
		// keep on sending the input character via input_queue until '\r' i.e. carriage return is encountered.
		// the entire logic that you write must be enclosed inside while(1) loop.

        while(1){
        	input = XUartPs_RecvByte(UART_BASEADDR);
        	if (input == '\r'){
        		break;
        	} else {
        		xQueueSendToBack(input_queue, &input, 0);
        	}
        }

		/**************************************************/


        //once the '\r' is received, we send '\0' character in the end via input_queue.
        //consider the character '\0' as a flag that signals the vCipherDecipherTask to start the encryption/decryption.
        //in other words, '\0' marks the end of input sequence from user that must be converted.
        input = '\0';
        xQueueSend(input_queue, &input, portMAX_DELAY);
    }
}

void vCipherDecipherTask(void *pvParameters) {

    static char read_from_queue12_value[QUEUE_LENGTH];
    int no_of_characters_read=0;

    while (1){

    	no_of_characters_read=0;
    	memset(output_text_sequence, 0, QUEUE_LENGTH);

    	while(no_of_characters_read<QUEUE_LENGTH){
    		//while loop across xQueueReceive() ensures that the character was received properly.
    		while (xQueueReceive(input_queue, &read_from_queue12_value[no_of_characters_read], portMAX_DELAY) != pdTRUE);
    		/**************************************************/
			// keep on receiving the characters and store them.
    		// Once '\0' is encountered you must exit this loop.
    		// "no_of_characters_read" variable is use to keep track of no. f characters read from queue.

    		if (read_from_queue12_value[no_of_characters_read] == '\0'){
    			break;
    		}

    		no_of_characters_read++;

			/**************************************************/
    	}


    	/**************************************************/

		// You must now begin to start the cipher or decipher based on the user choice.
		// If user has selected CIPHER, you must use valid function to encrypt the read character.
		// If user has selected DECIPHER, you must use corresponding function for decryption.
    	// The functions that you will need for these purposes are defined inside the header files.
		// Brief explanation is also given inside the lab manual.
    	// Use the "output_text_sequence" array for storing the output message.

		if (opr == CIPHER){
			for(int i = 0; i < no_of_characters_read; i++){
				output_text_sequence[i] = cipher_char(read_from_queue12_value[i], 2);
			}
		}
		else if( opr == DECIPHER){
			for(int i = 0; i < no_of_characters_read; i++){
				output_text_sequence[i] = decipher_char(read_from_queue12_value[i], 2);
			}
		}
		xil_printf("\r");

    	/**************************************************/

		xil_printf("Output message: ");

		/**************************************************/
		// Send the output message via "output_queue" to vDisplayTask.
		for(int i = 0; i < no_of_characters_read; i++){
			xQueueSendToBack(output_queue, &output_text_sequence[i], 0);
		}

		/**************************************************/

    }

}

void vDisplayTask(void *pvParameters) {

	u8 write_to_console;
	while(1){
		while (xQueueReceive(output_queue, &write_to_console, portMAX_DELAY) != pdTRUE);

		while(1){
			//if the transmitter is full, wait else send the data...
			if(XUartPs_IsTransmitFull(UART_BASEADDR)){
			}
			else{
				/***********************************************/
				//Send the byte to the UART using the variable "write_to_console"

					XUartPs_SendByte(UART_BASEADDR, write_to_console);

				/***********************************************/
				break;
			}
		}
	}
}


//function definition for UART initialization
//******STUDENTS SHOULD NOT CHANGE THIS FUNCTION AT ALL ******
int Intialize_UART(u16 DeviceId)
{
	int Status;
	/*
	 * Initialize the UART driver so that it's ready to use.
	 * Look up the configuration in the config table, then initialize it.
	 */
	Config = XUartPs_LookupConfig(DeviceId);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XUartPs_CfgInitialize(&uart, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Use NORMAL UART mode. */
	XUartPs_SetOperMode(&uart, XUARTPS_OPER_MODE_NORMAL);

	return XST_SUCCESS;
}


int main() {

		int Status;

		xTaskCreate( 	vInputTask,
						"Task_Input",
						configMINIMAL_STACK_SIZE*5,
						NULL,
						tskIDLE_PRIORITY+1,
						NULL );

		xTaskCreate( 	vCipherDecipherTask,
						"Task_Cipher_Decipher",
						configMINIMAL_STACK_SIZE*5,
						NULL,
						tskIDLE_PRIORITY+2,
						NULL );

		xTaskCreate(	vDisplayTask,
						"Task_Display",
						configMINIMAL_STACK_SIZE*5,
						NULL,
						tskIDLE_PRIORITY+3,
						NULL);

		//function call for UART initialization
		Status = Intialize_UART(UART_DEVICE_ID);
		if (Status != XST_SUCCESS) {
			xil_printf("UART Polled Mode Example Test Failed\r\n");
		}

		xil_printf("A cipher is an algorithm used for encrypting a message to hide its content from unauthorized users.\r");
		xil_printf("Deciphering is the process of converting an encrypted message back into its original form.\r");
		xil_printf("In this lab, we use a shift parameter such that each letter is replaced by a different letter in the alphabet order.\r");

		//Input queue - shared between vInputTask and vCipherDecipherTask
		input_queue = xQueueCreate( QUEUE_LENGTH, QUEUE_ITEM_SIZE );
		//Output queue - shared between vCipherDecipherTask and vDisplayTask
		output_queue = xQueueCreate( QUEUE_LENGTH, QUEUE_ITEM_SIZE );

		/* Check the queue was created. */
		configASSERT(input_queue);

		//initialize the cipher and decipher lookup table
		init_cipher_lookup_table(SHIFT_KEY);
		init_decipher_lookup_table(SHIFT_KEY);

		vTaskStartScheduler();

		while(1);
		return 0;

}
