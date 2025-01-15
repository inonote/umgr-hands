// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#pragma once
#include <memory>

namespace umgrhands {

class AppImpl;
class App {
public:
  App();

  int run();

private:
  std::shared_ptr<AppImpl> m_appImpl;
};

}
