#include "logger.h"

#include "stdarg.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "string.h"

#define SINGLE_MAX 128
Logger logger;

const char *Logger::logHeader[] = {"[DEBUG] ", "[INFO] ", "[WARNING] ",
                                   "[ERROR] ", "",        "UNKNOWN"};

Logger::Logger()
    : m_uartHandle(nullptr),
      m_writeIndex(0),
      m_readIndex(0),
      m_dmaInProgress(0),
      m_lengthTransmitted(0),
      m_logLevel(LOG_DEBUG) {
  memset(m_logBuffer, 0, m_bufferSize);
}

// Callback function to be called by the HAL_UART_TxCpltCallback
extern "C" void Logger_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == logger.uartHandle()) {
    logger.onDMATransmitComplete();
  }
}

void Logger::init(UART_HandleTypeDef *uartHandle) {
  m_uartHandle = uartHandle;
  HAL_UART_RegisterCallback(m_uartHandle, HAL_UART_TX_COMPLETE_CB_ID,
                            Logger_TxCpltCallback);
}

void Logger::setLogLevel(LogLevel level) { m_logLevel = level; }

void Logger::_log(LogLevel level, const char *format, ...) {
  if (level < m_logLevel) {
    return;
  }

  char buffer[SINGLE_MAX];

  // Copy header
  const char *header = logHeader[level];
  size_t headerLen = strlen(header);
  memcpy(buffer, header, headerLen);

  // Format content
  va_list args;
  va_start(args, format);
  int contentMax = SINGLE_MAX - headerLen - strlen(EOL);
  int contentLen = vsnprintf(buffer + headerLen, contentMax, format, args);
  va_end(args);

  if (contentLen < 0) return;
  contentLen = contentLen > contentMax ? contentMax : contentLen;

  // Copy EOL
  strcpy(buffer + headerLen + contentLen, EOL);

  // Enqueue the whole message log into the circular buffer using an atomic way
  uint32_t totalLen = headerLen + contentLen + strlen(EOL);
  uint32_t writeIndex;
  do {
    writeIndex = __LDREXW(&m_writeIndex);
    uint32_t readIndex = m_readIndex;  // Get a local copy of readIndex, it may
                                       // change in this loop, but that's fine
    uint32_t freeSpace = (readIndex <= writeIndex)
                             ? (m_bufferSize - writeIndex + readIndex - 1)
                             : (readIndex - writeIndex - 1);

    if (totalLen > freeSpace) {  // Not enough space, discard
      __CLREX();
      return;
    } else {
      if (writeIndex + totalLen <= m_bufferSize) {
        memcpy(&m_logBuffer[writeIndex], buffer, totalLen);
      } else {  // Handle wrap-around
        uint32_t firstPartSize = m_bufferSize - writeIndex;
        memcpy(&m_logBuffer[writeIndex], buffer, firstPartSize);
        memcpy(m_logBuffer, buffer + firstPartSize, totalLen - firstPartSize);
      }
    }
  } while (__STREXW((writeIndex + totalLen) % m_bufferSize, &m_writeIndex));

  // Trigger UART DMA transmission if not already in progress
  startDMATransmission();
}

void Logger::startDMATransmission() {
  // Start the DMA transfer exclusively
  do {
    uint8_t dmaInProgress = __LDREXB(&m_dmaInProgress);
    if (dmaInProgress) {  // DMA is already in progress, so exit
      __CLREX();
      return;
    }
    // Atomically set dmaInProgress to true
  } while (__STREXB(1, &m_dmaInProgress));

  uint32_t writeIndex = m_writeIndex;  // Get a local copy
  m_lengthTransmitted = (m_readIndex <= writeIndex)
                            ? (writeIndex - m_readIndex)
                            : (m_bufferSize - m_readIndex);
  HAL_UART_Transmit_DMA(m_uartHandle, &m_logBuffer[m_readIndex],
                        m_lengthTransmitted);
}

// Callback to be called when DMA transmission is complete
void Logger::onDMATransmitComplete() {
  m_readIndex = (m_readIndex + m_lengthTransmitted) % m_bufferSize;
  m_dmaInProgress = false;

  // Check if new data has been added to the buffer during the transmission
  if (m_readIndex != m_writeIndex) {
    startDMATransmission();
  }
}
