// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#pragma once
#include <Windows.h>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>

namespace umgrhands {

struct SerialPortOptions {
  int baudRate;
  int dataBits;
  int parity;
  int stopBits;
  int flow;
};

struct SerialPortPacket {
  uint8_t* data;
  size_t size;
};

class SerialPort {
public:
  ~SerialPort();

  bool open(const wchar_t* filePath, const SerialPortOptions* options);
  bool close(); // 任意

  bool isOpened() { return !!m_context; }

  bool peekPacket(SerialPortPacket* packet);
  bool popPacket();

  bool write(const uint8_t* data, size_t size);

protected:
  struct Context {
    std::mutex mutex;
    std::condition_variable writeThreadCondition;
    HANDLE hSerialPort = INVALID_HANDLE_VALUE;
    std::queue<std::vector<uint8_t>> writeQueue;
    std::queue<std::vector<uint8_t>> readQueue;
    bool requestExit = false;
  };

  std::shared_ptr<Context> m_context;

  std::unique_ptr<std::thread> m_readThread;
  static void readThread(std::shared_ptr<Context> ctx);

  std::unique_ptr<std::thread> m_writeThread;
  static void writeThread(std::shared_ptr<Context> ctx);
};

}
