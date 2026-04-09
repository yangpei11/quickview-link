#ifndef _READER_H_
#define _READER_H_
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <cstring>
using DataCallback = std::function<void(const char* data, int size)>;
class Reader {
 public:
  Reader(std::string path);
  virtual ~Reader();
  bool Start();
  void Stop();
  void SetDataCallback(DataCallback cb);
  virtual void RunLoop() = 0;
  virtual void ResetToBeginning() {}
 protected:
  std::string m_path;
  DataCallback m_cb;
  std::unique_ptr<std::thread> m_thread;
  std::atomic_bool m_bStart;
  FILE* m_fp;
  char* m_buf;
  std::atomic<bool> m_reset_requested{false}; 
};

class H264Reader : public Reader{
 public:
  H264Reader(std::string path, uint32_t fps);
  ~H264Reader();
  virtual void RunLoop() override;
  virtual void ResetToBeginning() override;
private:
  void ReadOneNalu();
 private:
  uint32_t m_fps;
  std::string m_sps;
  std::string m_pps;
  std::string m_sei;
  int64_t _lastFrameMillis;
};

class PCMReader : public Reader {
 public:
  PCMReader(std::string path,int frameInterval,int frameSize);
  ~PCMReader();
  virtual void RunLoop() override;

 private:
  int m_frameInterval;
  int m_frameSize;
};
#endif  // !_H264_READER_H_