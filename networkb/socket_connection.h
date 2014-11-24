#pragma once
#ifndef __INCLUDED_NETWORKB_SOCKET_CONNECTION_H__
#define __INCLUDED_NETWORKB_SOCKET_CONNECTION_H__

#include <chrono>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

#include "networkb/connection.h"

#ifdef _WIN32
#include <WinSock2.h>

#else  // _WIN32
#define SOCKET int
#endif  // _WIN32

namespace wwiv {
namespace net {

class SocketConnection : public Connection
{
public:
  SocketConnection(const std::string& host, int port);
  virtual ~SocketConnection();

  virtual int receive(void* data, int size, std::chrono::milliseconds d) override;
  virtual int send(const void* data, int size, std::chrono::milliseconds d) override;

  virtual uint16_t read_uint16(std::chrono::milliseconds d) override;
  virtual uint8_t read_uint8(std::chrono::milliseconds d) override;

  virtual bool send_uint8(uint8_t data, std::chrono::milliseconds d) override;
  virtual bool send_uint16(uint16_t data, std::chrono::milliseconds d) override;

private:
  const std::string host_;
  const int port_;
  SOCKET sock_;
};

}  // namespace net
}  // namespace wwiv

#endif  // __INCLUDED_NETWORKB_SOCKET_CONNECTION_H__