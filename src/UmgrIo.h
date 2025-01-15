#pragma once
#include "SerialPort.h"

namespace umgrhands {

class UmgrIo {
public:
  bool open(const wchar_t* filePath);

  bool update();
  
  char m_airState = 0b000000;

protected:
  void recieveData();
  void onData();
  void sendData(uint8_t frame[], int frameLength);

  SerialPort m_serial;
  uint8_t m_receivedBuf[0xFE];
  size_t m_receivedBufLength = 0;
  bool m_receivedBufInvalid = false;

  bool m_isAutoScanEnabled = false;
};

}
