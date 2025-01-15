// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#pragma once
#include "opencv2/opencv.hpp"
#include "UmgrIo.h"

namespace umgrhands {

class AppImpl {
public:
  int run();

private:
  bool openVideoCapture();
  bool readFrame();

  void detectHands();

  cv::Mat toBinaryFrame(cv::Mat mat);
  
  cv::VideoCapture m_vid;
  cv::Mat m_initialFrame;
  cv::Mat m_currentFrame;
  cv::Mat m_previewFrame;

  UmgrIo m_umgrIo;
};

}

