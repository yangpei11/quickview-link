// webrtc_api.h
#ifndef WEBRTC_API_H_
#define WEBRTC_API_H_

#include <string>
#include <functional>
#include <memory>

namespace webrtc_sdk {

using OnConnectedCallback = std::function<void()>;
using OnDisconnectedCallback = std::function<void()>;
using OnErrorCallback = std::function<void(const std::string& message)>;
using OnIceCandidateCallback = std::function<void(const std::string& sdp_mid,
                                                  int sdp_mline_index,
                                                  const std::string& sdp)>;
using OnAudioFrameCallback = std::function<void(const uint8_t *buffer, int size)>;

class WebrtcSdkClient {
 public:
  static std::unique_ptr<WebrtcSdkClient> Create(
      const std::string& signal_url,
      const std::string& room_id,
      bool enable_audio = true);

  virtual ~WebrtcSdkClient() = default;

  virtual bool Start() = 0;
  virtual void Stop() = 0;
  virtual void SetServerId(int server_id) = 0;

  virtual void SetOnConnected(OnConnectedCallback cb) = 0;
  virtual void SetOnDisconnected(OnDisconnectedCallback cb) = 0;
  virtual void SetOnError(OnErrorCallback cb) = 0;
  virtual void SetOnIceCandidate(OnIceCandidateCallback cb) = 0;

  virtual void AddTrack(int trackId) = 0;
  virtual void InputFrame(const uint8_t *buffer, int size, int trackId, uint64_t captureTime, int frameId, bool keyFrame = false) = 0;
  virtual void InputAudioData(const char *buffer, int size) = 0;
  virtual void setOnAudioFrameReceived(OnAudioFrameCallback cb) = 0;

 protected:
  WebrtcSdkClient() = default;
};

}  // namespace webrtc_sdk

#endif  // WEBRTC_API_H_