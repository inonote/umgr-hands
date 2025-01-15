// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#include "SerialPort.h"
#include <cstring>

namespace umgrhands {

SerialPort::~SerialPort() {
  close();
}

bool SerialPort::open(const wchar_t* filePath, const SerialPortOptions* options) {
  if (m_context)
    return true;

  HANDLE hSerialPort = CreateFileW(filePath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

  if (hSerialPort == INVALID_HANDLE_VALUE)
    return false;

  m_context = std::make_shared<Context>();
  m_context->hSerialPort = hSerialPort;

  SetupComm(hSerialPort, 1024, 1024);
  PurgeComm(hSerialPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  DCB dcb;
  GetCommState(hSerialPort, &dcb);
  dcb.DCBlength = sizeof(dcb);
  dcb.fBinary = true;
  dcb.BaudRate = options->baudRate;
  dcb.ByteSize = options->dataBits;
  dcb.Parity = options->parity;
  dcb.StopBits = options->stopBits;
  dcb.fOutxCtsFlow = false;
  dcb.fOutxDsrFlow = false;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fInX = false;
  dcb.fOutX = false;
  dcb.fTXContinueOnXoff = false;


  if (options->flow) {
    dcb.fOutxCtsFlow = true;
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  }

  SetCommState(hSerialPort, &dcb);

  m_readThread = std::make_unique<std::thread>(readThread, m_context);
  m_writeThread = std::make_unique<std::thread>(writeThread, m_context);
  return true;
}

bool SerialPort::close() {
  if (!m_context)
    return false;

  {
    std::scoped_lock<std::mutex> mlock(m_context->mutex);
    m_context->requestExit = true;
  }

  m_context->writeThreadCondition.notify_one();
  m_writeThread->join();

  SetCommMask(m_context->hSerialPort, 0);

  m_readThread->join();

  PurgeComm(m_context->hSerialPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
  CloseHandle(m_context->hSerialPort);

  m_context.reset();
  return true;
}

bool SerialPort::write(const uint8_t* data, size_t size) {
  if (!m_context)
    return false;

  {
    std::scoped_lock<std::mutex> mlock(m_context->mutex);
    std::vector<uint8_t> buf;
    buf.resize(size);
    std::memcpy(buf.data(), data, buf.size());
    m_context->writeQueue.push(std::move(buf));
  }
  m_context->writeThreadCondition.notify_one();
  return true;
}

bool SerialPort::peekPacket(SerialPortPacket* packet) {
  if (!m_context || !packet)
    return false;

  std::scoped_lock<std::mutex> mlock(m_context->mutex);

  if (m_context->readQueue.empty())
    return false;

  packet->data = m_context->readQueue.front().data();
  packet->size = m_context->readQueue.front().size();
  return true;
}

bool SerialPort::popPacket() {
  if (!m_context)
    return false;

  std::scoped_lock<std::mutex> mlock(m_context->mutex);

  if (m_context->readQueue.empty())
    return false;

  m_context->readQueue.pop();
  return true;
}

void SerialPort::readThread(std::shared_ptr<Context> ctx) {
  bool requestExit = false;
  uint8_t buf[512];
  ptrdiff_t bufOffset = 0;;

  OVERLAPPED ov = { 0 };
  ov.hEvent = CreateEventW(nullptr, true, false, nullptr);
  SetCommMask(ctx->hSerialPort, EV_RXCHAR | EV_ERR);

  while (true) {
    {
      std::scoped_lock<std::mutex> mlock(ctx->mutex);
      if (ctx->requestExit)
        break;
    }

    DWORD evtMask = 0;
    DWORD transfer = 0;
    if (!WaitCommEvent(ctx->hSerialPort, &evtMask, &ov)) {
      if (::GetLastError() == ERROR_IO_PENDING)
        GetOverlappedResult(ctx->hSerialPort, &ov, &transfer, true);
      else
        break;
    }


    if (evtMask & EV_RXCHAR) {

      DWORD errors;
      COMSTAT comStat;
      ClearCommError(ctx->hSerialPort, &errors, &comStat);

      if (!comStat.cbInQue)
        continue;
      std::vector<uint8_t> buf(comStat.cbInQue);
      ReadFile(ctx->hSerialPort, (LPVOID*)buf.data(), buf.size(), nullptr, &ov); // バッファから取り込み

      {
        std::scoped_lock<std::mutex> mlock(ctx->mutex);
        ctx->readQueue.push(std::move(buf));
      }
    }
  }

  CloseHandle(ov.hEvent);
}

void SerialPort::writeThread(std::shared_ptr<Context> ctx) {
  OVERLAPPED ov = { 0 };

  while (true) {
    std::unique_lock<std::mutex> lock(ctx->mutex);
    ctx->writeThreadCondition.wait(lock, [&] {return !ctx->writeQueue.empty() || ctx->requestExit; });
    if (ctx->requestExit)
      break;

    while (!ctx->writeQueue.empty()) {
      std::vector<uint8_t>& buf = ctx->writeQueue.front();

      WriteFile(ctx->hSerialPort, buf.data(), (DWORD)buf.size(), nullptr, &ov);

      ctx->writeQueue.pop();
    }
  }
}

}
