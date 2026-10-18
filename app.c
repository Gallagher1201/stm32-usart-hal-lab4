#include "app.h"
#include "usart.h"

/* Select a Keil target: 1 = polling, 2 = interrupt, 3 = DMA. */
#ifndef APP_MODE
#define APP_MODE 2
#endif

/* UTF-8 full-width exclamation mark, followed by CR/LF. No trailing NUL sent. */
static uint8_t message[] = "hello windows\xEF\xBC\x81\r\n";
#if APP_MODE != 3
static uint8_t rx_byte;
#endif
volatile uint32_t sending = 1;
volatile uint32_t tx_frames, rx_commands, uart_errors;
volatile uint32_t app_mode = APP_MODE;

#if APP_MODE != 3
static void Handle_Command(uint8_t ch)
{
    if (ch == '#') sending = 0;
    if (ch == '*') sending = 1;
    rx_commands++;
}
#endif

void App_Start(void)
{
#if APP_MODE == 2
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1) != HAL_OK)
        Error_Handler();
#endif
}

void App_Step(void)
{
#if APP_MODE == 1
    /* Timeout 0 checks RXNE immediately, without waiting for a command. */
    if (HAL_UART_Receive(&huart1, &rx_byte, 1, 0) == HAL_OK)
        Handle_Command(rx_byte);
    if (sending) {
        if (HAL_UART_Transmit(&huart1, message, sizeof(message)-1, 100) == HAL_OK)
            tx_frames++;
        else uart_errors++;
    }
#else
    /* Start the next whole message only after the previous one completes. */
    if (sending && huart1.gState == HAL_UART_STATE_READY) {
#if APP_MODE == 2
        if (HAL_UART_Transmit_IT(&huart1, message, sizeof(message)-1) != HAL_OK)
            uart_errors++;
#else
        if (HAL_UART_Transmit_DMA(&huart1, message, sizeof(message)-1) != HAL_OK)
            uart_errors++;
#endif
    }
#endif
    /* Intentionally no HAL_Delay: continuous transmission. */
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1) tx_frames++;
}

#if APP_MODE == 2
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1) {
        Handle_Command(rx_byte);
        /* HAL reception completes after one byte, so arm it again. */
        if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1) != HAL_OK)
            uart_errors++;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1) {
        uart_errors++;
        /* Recover reception after an overrun or framing error. */
        HAL_UART_AbortReceive(uart);
        if (HAL_UART_Receive_IT(uart, &rx_byte, 1) != HAL_OK)
            Error_Handler();
    }
}
#endif
