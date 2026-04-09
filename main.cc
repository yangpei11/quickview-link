/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <getopt.h>
#include <csignal>
#include <iostream>
#include "webrtc_api.h"
#include <thread>    
#include <chrono>
#include "reader.h"

using namespace std;

bool is_exit = false;

void signal_handler(int)
{
  is_exit = true;
}

int main(int argc, char *argv[])
{
  std::string room_id = "default";
  std::string addr;
  string filename = "test.h264";
  int server_id = -1;
  int long_index = 0;
  const string short_opts = "a:r:f:s:hv";
  static const struct option long_opts[] = {
      {"addr", required_argument, nullptr, 'a'},
      {"room", required_argument, nullptr, 'r'},
      {"file", no_argument, nullptr, 'f'},
      {"help", no_argument, nullptr, 'h'},
      {"version", no_argument, nullptr, 'v'},
      {"server", required_argument, nullptr, 's'},
      {NULL, no_argument, nullptr, 0}};
  do
  {
    int opt = getopt_long(argc, argv, short_opts.c_str(), long_opts, &long_index);
    if (opt == -1)
    {
      break;
    }
    switch (opt)
    {
    case 'a':
      addr = string(optarg);
      break;
    case 'r':
      room_id = string(optarg);
      break;
    case 'f':
      filename = string(optarg);
      break;
    case 'h':
      cout << " usage: " << endl;
      cout << "   ./pusher <filename> <addr> <room>. examples: ./pusher -a ws://127.0.0.1:8080 -r 10" << endl;
      cout << " options: " << endl;
      cout << "   -a --addr arg     signal addr" << endl;
      cout << "   -r --room arg     server room" << endl;
      cout << "   -f --filename arg pusher filename" << endl;
      cout << "   -v --version" << endl;
      exit(0);
    case 'v':
    //   std::cout << "version: " << BRANCH_NAME << "." << COMMIT_HASH << std::endl;
    //   std::cout << "build at: " << BUILD_TIME << std::endl;
      exit(0);
    case 's':
      server_id = atoi(optarg);
      break;
    }
  } while (true);

  cout << "pusher file: " + filename + " to server with addr: " + addr + " room: " + room_id << endl;

  signal(SIGINT, signal_handler);

  auto instance = webrtc_sdk::WebrtcSdkClient::Create(addr, room_id, filename);
  instance->AddTrack(10000);
  //instance->AddTrack(10001);
  std::shared_ptr<Reader> m_h264_reader = std::make_shared<H264Reader>(filename, 10);

  m_h264_reader->SetDataCallback([&instance](const char *data, int size){
    static int frameId = 0;
    if(instance){
      instance->InputFrame((const uint8_t*)data, size, 10000, 200, frameId++);
      //instance->InputFrame((const uint8_t*)data, size, 10001, frameId++, 0);
    }
  });

  m_h264_reader->Start();

  // 只接受单声道 采样率48000 int16的pcm的数据
  std::shared_ptr<Reader> m_pcm_reader = std::make_shared<PCMReader>("output_mono.pcm", 10, 960);
  m_pcm_reader->SetDataCallback([&instance](const char *data, int size){
    if(instance){
      //cout << "音频数据发送" <<endl;
      instance->InputAudioData(data, size);
    }
  });
  m_pcm_reader->Start();

  instance->setOnAudioFrameReceived([](const uint8_t *buffer, int size){
    std::cout << "接收到音频数据,转发出去" << std::endl;
  });

  instance->SetOnConnected( [m_h264_reader](){
    // 把文件指针置于文件开头
    m_h264_reader->ResetToBeginning();
    std::cout << "和对端建立音视频连接" << std::endl;
  });

  instance->SetOnDisconnected( [](){
    std::cout << "和对端断开音视频连接" << std::endl;
  });


  instance->Start();

  while (!is_exit)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  m_h264_reader->Stop();
  instance->Stop();
  cout << "end." << endl;

  return 0;
}
