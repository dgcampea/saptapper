/// @file
/// Header file for Saptapper class.

#ifndef SAPTAPPER_HPP_
#define SAPTAPPER_HPP_

#include "cartridge.hpp"
#include "types.hpp"

namespace saptapper {

class Saptapper {
 public:
  Saptapper() = default;

  void Inspect(Cartridge& cartridge) const;

  static agbptr_t FindFreeSpace(std::string_view rom, agbsize_t size);
  static agbptr_t FindFreeSpace(std::string_view rom, agbsize_t size,
                                char filter, bool largest);

  static constexpr agbsize_t GetMinigsfSize(int song_count) {
    if (song_count <= 0) return 0;

    agbsize_t size = 1;
    int remaining = song_count >> 8;
    while (size < 4 && remaining != 0) {
      size++;
      remaining >>= 8;
    };
    return size;
  }
};

}  // namespace saptapper

#endif