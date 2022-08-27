#pragma once

#include <array>
#include <cstdint>
#include <string>

std::string toString( const std::array<uint8_t,32>& key );

std::string toString( const std::array<char,32>& key );

std::string toString( const std::array<char,64>& key );

