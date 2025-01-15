// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#include "UmgrIo.h"
#include "debug.h"

// from https://github.com/inonote/umgrio/blob/main/iofw.ino

#define SLIDER_COMMAND_SLIDER_REPORT    0x01
#define SLIDER_COMMAND_SET_LED          0x02
#define SLIDER_COMMAND_START_AUTOSCAN   0x03
#define SLIDER_COMMAND_STOP_AUTOSCAN    0x04
#define SLIDER_COMMAND_RESET            0x10
#define SLIDER_COMMAND_EXCEPTION        0xEE
#define SLIDER_COMMAND_BOARD_INFO       0xF0

#define SLIDER_BD_NUMBER "99331   "
#define SLIDER_BD_DEV_CLASS 0xA1
#define SLIDER_BD_CHIP_NUM "00000"
#define SLIDER_BD_FW_VER 1

#define JVS_SYN 0xE0
#define JVS_ESC 0xD0

namespace umgrhands {

bool UmgrIo::open(const wchar_t* filePath) {
  SerialPortOptions options;
  options.baudRate = 115200;
  options.dataBits = 8;
  options.stopBits = 0;
  options.parity = 0;
  options.flow = 0;
  return m_serial.open(filePath, &options);
}

bool UmgrIo::update() {
  if (!m_serial.isOpened())
    return false;

  recieveData();

  if (m_isAutoScanEnabled) {
    uint8_t frame[5];
    frame[0] = JVS_SYN;
    frame[1] = SLIDER_COMMAND_SLIDER_REPORT;
    frame[2] = 2;
    frame[3] = 0;
    frame[4] = m_airState;
    sendData(frame, sizeof(frame) / sizeof(frame[0]));
  }
  return true;
}

void UmgrIo::recieveData() {
  SerialPortPacket packet;
  while (m_serial.peekPacket(&packet)) {
    for (int i = 0; i < packet.size; ++i) {
      uint8_t rawByte = packet.data[i];
      if (rawByte == JVS_SYN) {
        if (m_receivedBufLength > 0 && !m_receivedBufInvalid)
          onData();
        m_receivedBufLength = 0;
        m_receivedBufInvalid = false;
      }

      if (rawByte != -1) {
        if (m_receivedBufLength >= 0xFE)
          m_receivedBufInvalid = true;
        if (!m_receivedBufInvalid) {
          m_receivedBuf[m_receivedBufLength] = rawByte;
          ++m_receivedBufLength;
        }
      }
    }
    m_serial.popPacket();
  }

  if (m_receivedBufLength >= 4 && m_receivedBuf[2] == m_receivedBufLength - 4 && !m_receivedBufInvalid) {
    onData();
    m_receivedBufLength = 0;
    m_receivedBufInvalid = false;
  }
}

void UmgrIo::onData() {
  if (m_receivedBufLength < 4)
    return;

  switch (m_receivedBuf[1]) {
  case SLIDER_COMMAND_START_AUTOSCAN:
    m_isAutoScanEnabled = true;
    umgrhands::printLog(L"INF: umgrio: autoscan started");
    break;
  case SLIDER_COMMAND_STOP_AUTOSCAN:
    m_isAutoScanEnabled = false;
    umgrhands::printLog(L"INF: umgrio: autoscan stopped");
    break;

    // 色設定
  case SLIDER_COMMAND_SET_LED:
    if (m_receivedBufLength < 4 + 19)
      break;

    break;

  case SLIDER_COMMAND_RESET:
    {
      uint8_t frame[3];
      frame[0] = JVS_SYN;
      frame[1] = SLIDER_COMMAND_RESET;
      frame[2] = 0;
      sendData(frame, sizeof(frame) / sizeof(frame[0]));
    }
    
    umgrhands::printLog(L"INF: umgrio: reset");
    break;

  case SLIDER_COMMAND_BOARD_INFO:
  {
    uint8_t frame[21];
    frame[0] = JVS_SYN;
    frame[1] = SLIDER_COMMAND_BOARD_INFO;
    frame[2] = 18;
    memcpy(frame + 3, SLIDER_BD_NUMBER, 8);
    frame[11] = SLIDER_BD_DEV_CLASS;
    memcpy(frame + 12, SLIDER_BD_CHIP_NUM, 5);
    frame[17] = 0xFF;
    frame[18] = SLIDER_BD_FW_VER;
    frame[19] = 0x00;
    frame[20] = 0x64;
    sendData(frame, sizeof(frame) / sizeof(frame[0]));

    umgrhands::printLog(L"INF: umgrio: report board info");
  }
  break;
  }
}

void UmgrIo::sendData(uint8_t frame[], int frameLength) {
  if (frameLength <= 0 || frameLength >= 0x80)
    return;

  uint8_t sum = JVS_SYN;
  uint8_t writeBuffer[0xFE] = { 0 };
  size_t writeBufferCursor = 0;

  for (int i = 0; i < frameLength; ++i) {
    sum -= frame[i];
    if (i && (frame[i] == JVS_SYN || frame[i] == JVS_ESC)) {
      writeBuffer[writeBufferCursor++] = JVS_ESC;
      writeBuffer[writeBufferCursor++] = frame[i] - 1;
    }
    else {
      writeBuffer[writeBufferCursor++] = frame[i];
    }
  }
  sum = (sum - 0xFF) & 0xFF;
  if (sum == JVS_SYN || sum == JVS_ESC) {
    writeBuffer[writeBufferCursor++] = JVS_ESC;
    writeBuffer[writeBufferCursor++] = sum - 1;
  }
  else {
    writeBuffer[writeBufferCursor++] = sum;
  }

  m_serial.write(writeBuffer, writeBufferCursor);
}

}
