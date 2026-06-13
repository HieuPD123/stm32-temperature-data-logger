/******************************************************************************
 * UART User Interface
 ******************************************************************************/
#include <stdint.h>

void UART_Send(uint8_t *data, uint16_t length);
/**
 * @param buffer Pointer to receive buffer
 * @param length Number of bytes expected
 * Enter your code here.
 */
void UART_Receive(uint8_t *buffer, uint16_t length);
/**
 * @param data Pointer to transmit buffer
 * @param length Number of bytes
 * Enter your code here.
 */
