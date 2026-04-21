# Ultra-Low Latency WebRTC Streaming SDK for Teleoperation, Remote Driving & Robot Control

`teleoperation` `remote-driving` `robot-control` `webrtc-streaming` `low-latency` `embedded-linux` `arm64` `h264` `real-time-video` `c++-sdk`

**[English](#english) | [中文](#中文)**

---

<a name="english"></a>

> **Note:** This repository is a **test/evaluation demo** for the SDK. For the full commercial product **QuickView Link** — an ultra-low latency adaptive video transmission system — please visit: **[https://shallyjin3.github.io/QuickView-link/](https://shallyjin3.github.io/QuickView-link/)**

**QuickView Link SDK** enables ultra-low latency (<70ms) video and audio streaming over WebRTC, purpose-built for **teleoperation**, **remote driving**, **robot control**, and **edge AI** applications. Deploy on embedded Linux devices (NVIDIA Jetson, RK3588, Raspberry Pi) or x86 servers with a simple C++ API — no WebRTC expertise required.

A minimal example demonstrating how to use the **QuickView Link SDK** to push H.264 video and PCM audio streams over WebRTC. **This demo uses file input for simplicity** — in real-world deployments, you would capture video/audio locally from cameras, microphones, or sensors on the device, and feed them directly into the SDK via `InputFrame()` / `InputAudioData()`.

Designed for scenarios demanding **ultra-low latency** (end-to-end as low as **70ms**), such as:

- Autonomous / teleoperated driving
- Remote robot control
- UAV / drone inspection (low-altitude economy)
- Real-time industrial monitoring
- Any scenario where end-to-end latency is critical

## Why This SDK?

| Feature | Description |
|---------|-------------|
| **Ultra-low latency** | End-to-end latency as low as **70ms**, optimized for real-time control scenarios (remote driving, robot teleoperation) |
| **Adaptive transmission** | Proprietary adaptive transport with strong resilience to weak/unstable networks and fast recovery |
| **Easy integration** | Single `.so` library + one header file. Less than 50 lines of code to push audio/video streams |
| **Multi-codec support** | H.264 / VP8 / VP9 / AV1 encoding, supports external encoded data input |
| **Audio support** | Mono PCM input (48kHz, 16-bit), with bidirectional audio callback |
| **Multi-track** | Support multiple video tracks in a single session |
| **Cross-platform** | Native Linux (x86_64) + ARM64 cross-compilation support (ideal for embedded devices, edge computing, Jetson, etc.) |
| **Security** | WebRTC standard with DTLS key exchange and SRTP encryption |
| **99.9% stability** | High transmission reliability for mission-critical scenarios |
| **Lightweight** | Pure C++ with minimal dependencies, no heavy framework required |

## Project Structure

```
pusher-demo/
├── main.cc                    # Entry point — SDK usage example
├── reader.h                   # H264/PCM file reader interface
├── reader.cc                  # H264 NALU parser & PCM reader implementation
├── CMakeLists.txt             # Build configuration
├── arm64-toolchain.cmake      # ARM64 cross-compilation toolchain
├── webrtc_sdk/
│   ├── include/
│   │   └── webrtc_api.h       # SDK public API header
│   └── lib/
│       └── libwebrtc_sdk.so   # SDK shared library (not included in repo*)
└── build/                     # Build output directory
```

> \* `libwebrtc_sdk.so` is **not included** in this repository. See [How to Get the SDK](#how-to-get-the-sdk) below.

## SDK API Overview

The SDK exposes a clean, callback-driven C++ interface via `webrtc_api.h`:

```cpp
#include "webrtc_api.h"

// 1. Create client instance (signal server URL, room ID)
auto client = webrtc_sdk::WebrtcSdkClient::Create(signal_url, room_id);

// 2. Add a video track
client->AddTrack(track_id);

// 3. Push H.264 frames (raw NAL units with start code)
client->InputFrame(buffer, size, track_id, capture_time_ms, frame_id);

// 4. Push audio data (mono, 48kHz, 16-bit PCM)
client->InputAudioData(buffer, size);

// 5. Receive audio from remote peer
client->setOnAudioFrameReceived([](const uint8_t* buffer, int size) {
    // handle received audio
});

// 6. Connection lifecycle callbacks
client->SetOnConnected([]() { /* peer connected */ });
client->SetOnDisconnected([]() { /* peer disconnected */ });
client->SetOnError([](const std::string& msg) { /* error handling */ });

// 7. Start / Stop
client->Start();
// ... when done:
client->Stop();
```

**That's it.** No complex configuration, no SDP manipulation, no ICE candidate handling — the SDK manages all WebRTC internals.

## Build & Run

### Prerequisites

- CMake >= 3.14
- C++14 compatible compiler (GCC recommended)
- `libwebrtc_sdk.so` placed in `webrtc_sdk/lib/`
- A WebSocket signaling server running

### Build (x86_64)

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build (ARM64 cross-compilation)

For embedded Linux devices (Jetson, RK3588, Raspberry Pi, etc.):

```bash
# Install cross-compilation toolchain
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user

# Build
mkdir build-arm64 && cd build-arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm64-toolchain.cmake
make -j$(nproc)
```

### Run

```bash
./pusher -a ws://<signal-server>:<port> -r <room_id> [-f <h264_file>]
```

**Options:**

| Flag | Description | Default |
|------|-------------|---------|
| `-a, --addr` | WebSocket signaling server address | (required) |
| `-r, --room` | Room ID for the session | `default` |
| `-f, --file` | H.264 video file to push | `test.h264` |
| `-s, --server` | Server ID | -1 |
| `-v, --version` | Print version info | |
| `-h, --help` | Print usage | |

**Example:**

```bash
# Push test.h264 video + output_mono.pcm audio to room 10
./pusher -a ws://192.168.1.100:8080 -r 10
```

### Run ARM64 binary with QEMU

```bash
qemu-aarch64 -L /usr/aarch64-linux-gnu ./pusher -a ws://127.0.0.1:8080 -r 10
```

## Input Format Requirements

| Media | Format | Details |
|-------|--------|---------|
| **Video** | H.264 Annex B | Raw NAL units with start codes (`00 00 00 01` or `00 00 01`). SPS/PPS are automatically prepended to IDR frames. |
| **Audio** | Raw PCM | Mono channel, 48000 Hz sample rate, 16-bit signed integer (S16LE), 960 samples per frame (20ms) |

## How It Works

```
  ┌─────────────────────────────────────────────────────┐
  │            Device (Robot / Vehicle / Edge)           │
  │                                                     │
  │  ┌────────────┐   H.264 NALUs   ┌──────────────┐   │
  │  │  Camera /  │ ──────────────> │              │   │     WebRTC
  │  │  Encoder   │                 │  WebRTC SDK  │ ──────────────> Receiver /
  │  │            │   PCM Audio     │              │   │              Browser
  │  │  Mic /     │ ──────────────> │              │ <────────────── (Audio)
  │  │  Sensor    │                 └──────────────┘   │
  │  └────────────┘                        │           │
  └────────────────────────────────────────│───────────┘
                                    WebSocket Signaling
                                           │
                                    ┌──────────────┐
                                    │   Signal     │
                                    │   Server     │
                                    └──────────────┘
```

> **Note:** This demo reads from H.264/PCM files to simulate the capture source. In production, replace the file readers with your actual camera/microphone capture pipeline and call `InputFrame()` / `InputAudioData()` directly.

1. **Local Capture**: The device captures video (H.264 encoded) and audio (PCM) from cameras, microphones, or sensors
2. **Signaling**: The SDK connects to a WebSocket signaling server to exchange SDP offers/answers and ICE candidates
3. **Media Push**: Captured H.264 NAL units and PCM audio frames are fed into the SDK via `InputFrame()` and `InputAudioData()`
4. **WebRTC Transport**: The SDK handles all WebRTC internals (DTLS, SRTP, ICE, STUN/TURN) transparently
5. **Bidirectional Audio**: The SDK supports receiving audio from the remote peer via `setOnAudioFrameReceived()` callback

## Typical Use Cases

### Autonomous Vehicle Teleoperation / Remote Driving
Push multiple camera feeds from an onboard computing unit (e.g., NVIDIA Jetson, industrial PC) to a remote operator console, with ultra-low latency for real-time vehicle control, takeover, and fleet monitoring.

### Robot Teleoperation & Remote Control
Stream first-person video from a robot to an operator while receiving audio commands, enabling responsive teleoperation over the internet. Ideal for inspection robots, humanoid robots, and robotic arms.

### UAV / Drone FPV & Real-Time Inspection
Real-time FPV video downlink from drones for inspection, surveillance, mapping, and low-altitude economy applications with minimal delay.

### Industrial IoT Edge Video Monitoring
Push real-time video from edge devices and IP cameras to control centers with minimal delay for time-sensitive industrial operations and predictive maintenance.

## QuickView Link - Commercial SDK

This repository only contains a **test demo**. The full commercial product **QuickView Link** offers:

- End-to-end latency as low as **70ms**
- **99.9%** transmission stability
- Proprietary adaptive transport with strong weak-network resilience
- Multi-codec support (H.264 / VP8 / VP9 / AV1)
- DTLS + SRTP security encryption
- Built-in debugging and log analysis tools
- Multi-platform deployment (x86 & ARM)

For full product details, visit: **[https://shallyjin3.github.io/QuickView-link/](https://shallyjin3.github.io/QuickView-link/)**

## How to Get the SDK

The `libwebrtc_sdk.so` library and the receiver-side code are **not included** in this repository.

If you are interested in using this SDK for your project, please contact me via email:

**yangpei11@pku.edu.cn**

I will provide:
- The commercial SDK (`libwebrtc_sdk.so`) for your target platform (x86_64 / ARM64 / others)
- Receiver-side sample code (browser / native)
- Technical support for integration
- Customization for your specific use case

## License

This demo code is provided as-is for evaluation purposes. The WebRTC SDK (`libwebrtc_sdk.so`) is proprietary software — please contact me for licensing details.

---

<a name="中文"></a>

# WebRTC 推流 Demo — 超低时延音视频推流 SDK（远程驾驶/机器人遥操作）

> **说明：** 本仓库为 SDK 的**测试/评估 Demo**。完整商用产品 **QuickView Link** — 超低时延视频自适应传输系统，请访问：**[https://shallyjin3.github.io/QuickView-link/](https://shallyjin3.github.io/QuickView-link/)**

**QuickView Link SDK** 基于 WebRTC 实现超低时延（<70ms）音视频传输，专为**远程驾驶**、**机器人遥操作**、**无人机图传**、**车端推流**及**边缘AI**场景打造。支持嵌入式 Linux 设备（NVIDIA Jetson、RK3588、树莓派）和 x86 服务器部署，C++ API 极简接入，无需 WebRTC 专业知识。

一个极简示例，演示如何使用 **QuickView Link SDK** 通过 WebRTC 推送 H.264 视频和 PCM 音频流。**本 Demo 使用文件输入仅为演示方便** — 在实际部署中，您需要从设备本地的摄像头、麦克风或传感器采集音视频数据，然后通过 `InputFrame()` / `InputAudioData()` 直接送入 SDK。

专为**超低时延**（端到端低至 **70ms**）场景设计：

- 自动驾驶 / 远程驾驶
- 远程机器人控制
- 无人机巡检（低空经济）
- 实时工业监控
- 任何对端到端时延有严格要求的场景

## 为什么选择这个 SDK？

| 特性 | 说明 |
|------|------|
| **超低时延** | 端到端时延低至 **70ms**，针对实时控制场景深度优化（远程驾驶、机器人遥操作） |
| **自适应传输** | 自研自适应传输算法，强弱网对抗能力，网络波动快速恢复 |
| **极简接入** | 一个 `.so` 动态库 + 一个头文件，不到 50 行代码即可完成音视频推流 |
| **多编码支持** | H.264 / VP8 / VP9 / AV1 编码，支持外部编码数据输入 |
| **音频支持** | 支持单声道 PCM 输入（48kHz, 16-bit），支持双向音频回调 |
| **多路视频** | 单会话支持多路视频 Track |
| **跨平台** | 原生支持 Linux (x86_64) + ARM64 交叉编译（适配嵌入式设备、边缘计算、Jetson 等） |
| **安全可靠** | 基于 WebRTC 标准，DTLS 密钥交换 + SRTP 加密 |
| **99.9% 稳定性** | 高传输可靠性，满足关键任务场景需求 |
| **轻量级** | 纯 C++ 实现，依赖极少，无需重量级框架 |

## 项目结构

```
pusher-demo/
├── main.cc                    # 入口文件 — SDK 使用示例
├── reader.h                   # H264/PCM 文件读取器接口
├── reader.cc                  # H264 NALU 解析器 & PCM 读取器实现
├── CMakeLists.txt             # 构建配置
├── arm64-toolchain.cmake      # ARM64 交叉编译工具链
├── webrtc_sdk/
│   ├── include/
│   │   └── webrtc_api.h       # SDK 公开 API 头文件
│   └── lib/
│       └── libwebrtc_sdk.so   # SDK 动态库（不包含在仓库中*）
└── build/                     # 构建输出目录
```

> \* `libwebrtc_sdk.so` **不包含**在本仓库中。请参阅下方 [如何获取 SDK](#如何获取-sdk)。

## SDK API 概览

SDK 通过 `webrtc_api.h` 暴露简洁的回调驱动式 C++ 接口：

```cpp
#include "webrtc_api.h"

// 1. 创建客户端实例（信令服务器地址、房间号）
auto client = webrtc_sdk::WebrtcSdkClient::Create(signal_url, room_id);

// 2. 添加视频轨道
client->AddTrack(track_id);

// 3. 推送 H.264 视频帧（带 start code 的裸 NAL 单元）
client->InputFrame(buffer, size, track_id, capture_time_ms, frame_id);

// 4. 推送音频数据（单声道, 48kHz, 16-bit PCM）
client->InputAudioData(buffer, size);

// 5. 接收远端音频
client->setOnAudioFrameReceived([](const uint8_t* buffer, int size) {
    // 处理接收到的音频
});

// 6. 连接生命周期回调
client->SetOnConnected([]() { /* 对端已连接 */ });
client->SetOnDisconnected([]() { /* 对端已断开 */ });
client->SetOnError([](const std::string& msg) { /* 错误处理 */ });

// 7. 启动 / 停止
client->Start();
// ... 结束时：
client->Stop();
```

**就这么简单。** 无需复杂配置，无需手动处理 SDP，无需管理 ICE 候选 — SDK 内部处理所有 WebRTC 细节。

## 编译与运行

### 环境要求

- CMake >= 3.14
- 支持 C++14 的编译器（推荐 GCC）
- 将 `libwebrtc_sdk.so` 放置于 `webrtc_sdk/lib/` 目录
- 一个运行中的 WebSocket 信令服务器

### 编译（x86_64）

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 编译（ARM64 交叉编译）

适用于嵌入式 Linux 设备（Jetson、RK3588、树莓派等）：

```bash
# 安装交叉编译工具链
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user

# 编译
mkdir build-arm64 && cd build-arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm64-toolchain.cmake
make -j$(nproc)
```

### 运行

```bash
./pusher -a ws://<信令服务器>:<端口> -r <房间号> [-f <h264文件>]
```

**参数说明：**

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-a, --addr` | WebSocket 信令服务器地址 | （必填） |
| `-r, --room` | 房间号 | `default` |
| `-f, --file` | 推流的 H.264 视频文件 | `test.h264` |
| `-s, --server` | 服务器 ID | -1 |
| `-v, --version` | 打印版本信息 | |
| `-h, --help` | 打印帮助 | |

**示例：**

```bash
# 向房间 10 推送 test.h264 视频 + output_mono.pcm 音频
./pusher -a ws://192.168.1.100:8080 -r 10
```

### 使用 QEMU 运行 ARM64 二进制

```bash
qemu-aarch64 -L /usr/aarch64-linux-gnu ./pusher -a ws://127.0.0.1:8080 -r 10
```

## 输入格式要求

| 媒体类型 | 格式 | 详细说明 |
|----------|------|----------|
| **视频** | H.264 Annex B | 带 start code（`00 00 00 01` 或 `00 00 01`）的裸 NAL 单元。IDR 帧会自动前置 SPS/PPS。 |
| **音频** | Raw PCM | 单声道，48000 Hz 采样率，16-bit 有符号整数（S16LE），每帧 960 采样（20ms） |

## 工作原理

```
  ┌─────────────────────────────────────────────────────┐
  │           设备端（机器人 / 无人车 / 边缘设备）          │
  │                                                     │
  │  ┌────────────┐   H.264 NALUs   ┌──────────────┐   │
  │  │  摄像头 /  │ ──────────────> │              │   │     WebRTC
  │  │  编码器    │                 │  WebRTC SDK  │ ──────────────> 接收端 /
  │  │            │   PCM 音频      │              │   │              浏览器
  │  │  麦克风 /  │ ──────────────> │              │ <────────────── (音频)
  │  │  传感器    │                 └──────────────┘   │
  │  └────────────┘                        │           │
  └────────────────────────────────────────│───────────┘
                                    WebSocket 信令
                                           │
                                    ┌──────────────┐
                                    │   信令服务器   │
                                    └──────────────┘
```

> **说明：** 本 Demo 从 H.264/PCM 文件读取数据来模拟采集源。实际生产环境中，请替换为您的摄像头/麦克风采集管线，直接调用 `InputFrame()` / `InputAudioData()` 送入数据。

1. **本地采集**：设备从摄像头、麦克风或传感器采集视频（H.264 编码）和音频（PCM）
2. **信令**：SDK 连接 WebSocket 信令服务器，交换 SDP Offer/Answer 和 ICE 候选
3. **媒体推送**：通过 `InputFrame()` 和 `InputAudioData()` 将采集到的 H.264 NAL 单元和 PCM 音频帧送入 SDK
4. **WebRTC 传输**：SDK 内部透明处理所有 WebRTC 细节（DTLS、SRTP、ICE、STUN/TURN）
5. **双向音频**：通过 `setOnAudioFrameReceived()` 回调接收远端音频

## 典型应用场景

### 自动驾驶 / 远程驾驶 / 车端推流
从车载计算单元（如 NVIDIA Jetson、工控机）推送多路摄像头画面到远程操控台，凭借超低时延实现实时车辆控制、接管与车队监控。

### 机器人遥操作 / 远程控制
将机器人第一视角视频流推送到操作员端，同时接收音频指令，实现通过互联网的低时延遥操作。适用于巡检机器人、人形机器人、机械臂等。

### 无人机图传 / FPV 实时巡检
无人机实时 FPV 视频下行，用于巡检、测绘、监控及低空经济应用，传输时延极低。

### 工业物联网 / 边缘视频监控
从边缘设备和 IP 摄像头向控制中心推送实时视频，以极低延迟满足时间敏感型工业作业与预测性维护需求。

## QuickView Link — 商用 SDK

本仓库仅包含**测试 Demo**。完整商用产品 **QuickView Link** 提供：

- 端到端时延低至 **70ms**
- **99.9%** 传输稳定性
- 自研自适应传输，强弱网对抗能力
- 多编码支持（H.264 / VP8 / VP9 / AV1）
- DTLS + SRTP 安全加密
- 内置调试与日志分析工具
- 多平台部署（x86 & ARM）

完整产品详情请访问：**[https://shallyjin3.github.io/QuickView-link/](https://shallyjin3.github.io/QuickView-link/)**

## 如何获取 SDK

`libwebrtc_sdk.so` 动态库和接收端代码**不包含**在本仓库中。

如果您对本 SDK 感兴趣，欢迎通过邮件联系我：

**yangpei11@pku.edu.cn**

我将提供：
- 适配您目标平台的商用 SDK（`libwebrtc_sdk.so`）（x86_64 / ARM64 / 其他平台）
- 接收端示例代码（浏览器 / 原生应用）
- 接入技术支持
- 针对您具体场景的定制化方案

## 许可证

本 Demo 代码按原样提供，仅供评估使用。WebRTC SDK（`libwebrtc_sdk.so`）为私有软件 — 请联系我了解授权详情。
