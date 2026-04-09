#include "reader.h"
#include <time.h>
// #include "rtc_base/time_utils.h"
// #include "system_wrappers/include/sleep.h"
#include <iostream>
#include <chrono>
#include <thread>
#define MAX_BUFFER_SIZE 1024 * 1024
static const char start_code_4_bytes[4] = {0x00, 0x00, 0x00, 0x01};
static const char start_code_3_bytes[3] = {0x00, 0x00, 0x01};
Reader::Reader(std::string path) {
  m_path = path;
  m_bStart = false;
  m_fp = NULL;
  m_buf = NULL;
  m_cb = NULL;
}

Reader::~Reader() {
  Stop();
}

bool Reader::Start() {
  if (m_bStart) {
    return true;
  }

  m_bStart = true;

  if (m_path.empty()) {
    return true;
  }

  m_fp = fopen(m_path.data(), "rb+");

  if (NULL == m_fp) {
    return false;
  }

  m_buf = (char*)malloc(MAX_BUFFER_SIZE);

  if (NULL == m_buf) {
    return false;
  }

  memset(m_buf, 0, MAX_BUFFER_SIZE);

  m_thread.reset(new std::thread(std::bind(&Reader::RunLoop, this)));

  return true;
}

void Reader::Stop() {
  m_bStart = false;

  if (m_thread && m_thread->joinable()) {
    m_thread->join();
  }

  if (NULL != m_fp) {
    fclose(m_fp);
    m_fp = NULL;
  }

  if (NULL != m_buf) {
    free(m_buf);
    m_buf = NULL;
  }
}

void Reader::SetDataCallback(DataCallback cb) {
  m_cb = cb;
}

H264Reader::H264Reader(std::string path, uint32_t fps) : Reader(path) {
  m_fps = fps;
  _lastFrameMillis = 0;
}

H264Reader::~H264Reader() {}

void H264Reader::ResetToBeginning() {
    m_reset_requested = true;
}

void H264Reader::RunLoop() {
  auto lastFrameTime = std::chrono::steady_clock::now();
  const std::chrono::milliseconds frameInterval(1000 / m_fps);

  while (m_bStart) {
    if (m_reset_requested.exchange(false)) {
      if (m_fp) {
        fseek(m_fp, 0, SEEK_SET);
        lastFrameTime = std::chrono::steady_clock::now(); // 重置时间基准
        std::cout << " ✅ ✅  ✅  ✅  ✅  ✅  ✅  ✅  ✅  H264Reader: File pointer reset to beginning" << std::endl;
      }
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);

    if (elapsed >= frameInterval) {
      ReadOneNalu();
      lastFrameTime = now;
    } else {
      auto sleepTime = frameInterval - elapsed;
      if (sleepTime > std::chrono::milliseconds(0)) {
        std::this_thread::sleep_for(sleepTime);
      }
    }
  }
}

void H264Reader::ReadOneNalu(){
  int index = 0;
  std::string data = "";
  bool bStop = false;
  while (!bStop) {
    int ch = fgetc(m_fp);

    if (EOF == ch) {
      fseek(m_fp, 0, SEEK_SET);
      continue;
    }

    if (index >= MAX_BUFFER_SIZE - 1) {
      break;
    }else{
      m_buf[index] = ch;
    }

    if (index >= 3 && 
        m_buf[index] == 0x01 && 
        m_buf[index - 1] == 0x00 &&
        m_buf[index - 2] == 0x00 && 
        m_buf[index - 3] == 0x00) {
      switch (m_buf[0] & 0x1F) {
        case 1:
          data = std::string(start_code_4_bytes, 4) + std::string(m_buf, index - 3);
          bStop = true;
          break;
        case 5:
          data = m_sps + m_pps + std::string(start_code_4_bytes, 4) + std::string(m_buf, index - 3);
          bStop = true;
          break;
        case 6:
          m_sei = std::string(start_code_4_bytes, 4) + std::string(m_buf, index - 3);
          break;
        case 7:
          m_sps = std::string(start_code_4_bytes, 4) + std::string(m_buf, index - 3);
          break;
        case 8:
          m_pps = std::string(start_code_4_bytes, 4) + std::string(m_buf, index - 3);
          break; 
        default:
          break;
      }
      if (m_cb && !data.empty()) {
        m_cb(data.data(), data.size());
      }
      index = 0;
    } else if (index >= 2 &&
        m_buf[index] == 0x01 &&
        m_buf[index - 1] == 0x00 &&
        m_buf[index - 2] == 0x00) {
      switch (m_buf[0] & 0x1F) {
        case 1:
          data = std::string(start_code_3_bytes, 3) + std::string(m_buf, index - 2);
          bStop = true;
          break;
        case 5:
          data = m_sps + m_pps  + std::string(start_code_3_bytes, 3) + std::string(m_buf, index - 2);
          bStop = true;
          break;
        case 6:
          m_sei = std::string(start_code_3_bytes, 3) + std::string(m_buf, index - 2);
          break;
        case 7:
          m_sps = std::string(start_code_3_bytes, 3) + std::string(m_buf, index - 2);
          break;
        case 8:
          m_pps = std::string(start_code_3_bytes, 3) + std::string(m_buf, index - 2);
          break;
        default:
          break;
      }
      if (m_cb && !data.empty()) {
        m_cb(data.data(), data.size());
      }
      index = 0;
    } else {
      index++;
    }
  }
}

PCMReader::PCMReader(std::string path, int frameInterval, int frameSize):Reader(path) {
  m_frameInterval = frameInterval;
  m_frameSize = frameSize;
}

PCMReader::~PCMReader() {}

void PCMReader::RunLoop() {
  while (m_bStart) {
    int size = fread(m_buf, 1, m_frameSize, m_fp);

    if (feof(m_fp)) {
      fseek(m_fp, 0, SEEK_SET);
      continue;
    }

    if (m_cb) {
      m_cb(m_buf, size);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(m_frameInterval));
  }
}
