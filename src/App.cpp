// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#include "App.h"
#include "AppImpl.h"

namespace umgrhands {

App::App(): m_appImpl(std::make_shared<AppImpl>()) {
}

int App::run() {
  return m_appImpl->run();
}

}
