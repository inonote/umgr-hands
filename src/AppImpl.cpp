// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#include "AppImpl.h"
#include <iostream>

#include "debug.h"

namespace umgrhands {

int AppImpl::run() {
  if (!openVideoCapture()) {
    umgrhands::printError(L"ERR: failed open camera!");
    return -1;
  }

  umgrhands::printLog(L"INF: camera opened");

  const wchar_t* serialPath = L"\\\\.\\COM11";
  if (!m_umgrIo.open(serialPath)) {
    umgrhands::printError(L"ERR: failed open serial! [%s]", serialPath);
    return -1;
  }

  umgrhands::printLog(L"INF: serial opened [%s]", serialPath);


  while (readFrame()) {
    m_umgrIo.update();

    if (m_previewFrame.empty())
      m_previewFrame.create(cv::Size(m_currentFrame.cols, m_currentFrame.rows), CV_8UC3);
    else
      m_previewFrame = 0;

    detectHands();
    cv::imshow("umgr-hands", m_previewFrame);

    const int key = cv::waitKey(1);
    if (key == 'q')
      break;
    else if (key == 'w') {
      m_initialFrame = toBinaryFrame(m_currentFrame);
      umgrhands::printLog(L"INF: initial frame is taken!!", serialPath);
    }
  }

  cv::destroyAllWindows();
  return 0;
}

bool AppImpl::openVideoCapture() {
  if (!m_vid.open(0))
    return false;

  if (!m_vid.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G')))
    umgrhands::printError(L"ERR: failed set cv::CAP_PROP_FOURCC");

  if (!m_vid.set(cv::CAP_PROP_BUFFERSIZE,1))
    umgrhands::printError(L"ERR: failed set cv::CAP_PROP_BUFFERSIZE");

  return true;
}

bool AppImpl::readFrame() {
  if (!m_vid.isOpened())
    return false;

  return m_vid.read(m_currentFrame);
}

void AppImpl::detectHands() {
  if (m_initialFrame.empty())
    return;

  auto matCurrent = toBinaryFrame(m_currentFrame);
  cv::Mat matDiff;
  cv::absdiff(matCurrent, m_initialFrame, matDiff);
  matDiff.copyTo(m_previewFrame);
  //cv::cvtColor(m_previewFrame, m_previewFrame, cv::COLOR_GRAY2RGB);
  m_umgrIo.m_airState = 0;
  int airStateOffsets[6] = { 6, 3, 5, 2, 4, 1 };

  const int rangeTop = 0;
  const int rangeBottom = 260;
  const int range = m_initialFrame.rows - rangeTop - rangeBottom;
  for (int i = 0; i < 6; ++i) {
    const int left = m_initialFrame.cols * 5 / 7;
    const int right = m_initialFrame.cols * 6 / 7;
    const int top = rangeTop + range * (i * 4) / 24;
    const int bottom = rangeTop + range * (i * 4 + 5) / 24;
    cv::Mat subMat = matDiff(cv::Rect(left, top, right - left, bottom - top));
    const double ratio = cv::mean(subMat)[0] / 255.0;

    if (ratio > 0.1) {
      cv::rectangle(m_previewFrame, cv::Rect(left, top, right - left, bottom - top), cv::Scalar(0.0, 0.0, 255.0), 8);

      m_umgrIo.m_airState |= 1 << airStateOffsets[i];
      
    }
  }
}

cv::Mat AppImpl::toBinaryFrame(cv::Mat mat) {
  cv::Mat retMat = mat.clone();
  //cv::blur(mat, retMat, cv::Size(10, 10));
  //cv::cvtColor(retMat, retMat, cv::COLOR_BGR2GRAY);
  //cv::threshold(retMat, retMat, 45, 255, cv::THRESH_BINARY);
  return retMat;
}

}
