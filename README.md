# Logger-for-STM32-HAL

This C++ logger utilizes the STM32 HAL library to provide efficient logging capabilities. The logger temporarily stores log entries in a circular buffer in RAM before transmitting the data via UART using DMA. It is designed to be thread-safe, making it suitable for use in Interrupt Service Routines (ISRs).

## Configuration

A sample configuration is available in `usart.h` and `usart.c`. When setting up UART for this logger in STM32CubeMX, ensure the following settings:

- Enable DMA Normal mode for `UART_TX`.
- Enable the UART global interrupt.
- Enable UART callback registration from the Project Manager in the STM32CubeMX.

## Usage

```cpp
MX_DMA_Init();
MX_USART2_UART_Init();

logger.init(&huart);
logger.info("This is a RAM logger");
logger.warning("The value is %d", value);
