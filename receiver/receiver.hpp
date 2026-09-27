#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> recv_exact(int sockfd, size_t n);
