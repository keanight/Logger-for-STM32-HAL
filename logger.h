#pragma once

#include "stm32f4xx_hal.h"

class Logger {
 private:
  enum LogLevel { LOG_DEBUG = 0, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_NONE };
  static  const char *logHeader[];

 public:
  Logger();

  /**
   *Uart needs to be configured with transfer DMA enabled in normal mode
   */
  void init(UART_HandleTypeDef *uartHandle);

  void setLogLevel(LogLevel level);

  template <typename... Args>
  void debug(const char *format, Args... args) {
    _log(LOG_DEBUG, format, args...);
  }

  template <typename... Args>
  void info(const char *format, Args... args) {
    _log(LOG_INFO, format, args...);
  }

  template <typename... Args>
  void warning(const char *format, Args... args) {
    _log(LOG_WARNING, format, args...);
  }

  template <typename... Args>
  void error(const char *format, Args... args) {
    _log(LOG_ERROR, format, args...);
  }

  template <typename... Args>
  void log(const char *format, Args... args) {
    _log(LOG_NONE, format, args...);
  }

  void onDMATransmitComplete();

  UART_HandleTypeDef *uartHandle() { return m_uartHandle; }

  inline uint32_t freeSpace() {
    return (m_readIndex <= m_writeIndex)
               ? (m_bufferSize - m_writeIndex + m_readIndex - 1)
               : (m_readIndex - m_writeIndex - 1);
  }

 private:
  void startDMATransmission();

  void _log(LogLevel level, const char *format, ...);

 private:
  static constexpr const char *EOL = "\r\n";
  static constexpr uint32_t m_bufferSize = 512;
  uint8_t m_logBuffer[m_bufferSize];
  UART_HandleTypeDef *m_uartHandle;
  volatile uint32_t m_writeIndex;  // Write index
  volatile uint32_t m_readIndex;   // Read index
  volatile uint8_t m_dmaInProgress;
  volatile uint32_t m_lengthTransmitted;
  LogLevel m_logLevel;  // Current log level threshold
};

extern Logger logger;