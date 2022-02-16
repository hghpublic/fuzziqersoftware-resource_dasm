#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <phosg/Encoding.hh>


class MemoryContext {
public:
  MemoryContext();
  ~MemoryContext() = default;

  uint32_t guest_addr_for_host_addr(const void* ptr);

  inline void* at(uint32_t addr, size_t size = 1) {
    // TODO: This breaks if addr = 0 and size = 0. I'm assuming that's never
    // going to be a problem, but.........
    size_t start_page_num = this->page_number_for_addr(addr);
    size_t end_page_num = this->page_number_for_addr(addr + size - 1);
    auto arena = this->arena_for_page_number[start_page_num];
    if (!arena.get()) {
      throw std::out_of_range("address not within allocated pages");
    }
    for (size_t z = start_page_num + 1; z <= end_page_num; z++) {
      if (this->arena_for_page_number[z] != arena) {
        throw std::out_of_range("data not entirely contained within one arena");
      }
    }
    return reinterpret_cast<uint8_t*>(arena->host_addr) + (addr - arena->addr);
  }
  inline const void* at(uint32_t addr, size_t size = 1) const {
    return const_cast<MemoryContext*>(this)->at(addr, size);
  }

  template <typename T>
  T* obj(uint32_t addr, uint32_t size = sizeof(T)) {
    return reinterpret_cast<T*>(this->at(addr, size));
  }
  template <typename T>
  const T* obj(uint32_t addr, uint32_t size = sizeof(T)) const {
    return reinterpret_cast<const T*>(this->at(addr, size));
  }

  template <typename T>
  T read(uint32_t addr) const {
    return *this->obj<T>(addr);
  }
  template <typename T>
  void write(uint32_t addr, const T& obj) {
    *this->obj<T>(addr) = obj;
  }

  inline int8_t read_s8(uint32_t addr) const {
    return this->read<int8_t>(addr);
  }
  inline void write_s8(uint32_t addr, int8_t value) {
    this->write<int8_t>(addr, value);
  }
  inline uint8_t read_u8(uint32_t addr) const {
    return this->read<uint8_t>(addr);
  }
  inline void write_u8(uint32_t addr, uint8_t value) {
    this->write<uint8_t>(addr, value);
  }
  inline int16_t read_s16(uint32_t addr) const {
    return bswap16(this->read<int16_t>(addr));
  }
  inline void write_s16(uint32_t addr, int16_t value) {
    this->write<int16_t>(addr, bswap16(value));
  }
  inline uint16_t read_u16(uint32_t addr) const {
    return bswap16(this->read<uint16_t>(addr));
  }
  inline void write_u16(uint32_t addr, uint16_t value) {
    this->write<uint16_t>(addr, bswap16(value));
  }
  inline int32_t read_s32(uint32_t addr) const {
    return bswap32(this->read<int32_t>(addr));
  }
  inline void write_s32(uint32_t addr, int32_t value) {
    this->write<int32_t>(addr, bswap32(value));
  }
  inline uint32_t read_u32(uint32_t addr) const {
    return bswap32(this->read<uint32_t>(addr));
  }
  inline void write_u32(uint32_t addr, uint32_t value) {
    this->write<uint32_t>(addr, bswap32(value));
  }

  uint32_t allocate(size_t size);
  void allocate_at(uint32_t addr, size_t size);
  uint32_t allocate_within(uint32_t addr_low, uint32_t addr_high, size_t size);
  void free(uint32_t addr);
  size_t get_block_size(uint32_t addr) const;

  void set_symbol_addr(const char* name, uint32_t addr);
  uint32_t get_symbol_addr(const char* name) const;
  const std::unordered_map<std::string, uint32_t> all_symbols() const;

  size_t get_page_size() const;

  void print_state(FILE* stream) const;
  void print_contents(FILE* stream) const;

  void import_state(FILE* stream);
  void export_state(FILE* stream) const;

private:
  uint8_t page_bits;
  size_t page_size;
  size_t total_pages;

  size_t size;
  size_t allocated_bytes;
  size_t free_bytes;

  struct Arena {
    uint32_t addr;
    void* host_addr;
    size_t size;
    size_t allocated_bytes;
    size_t free_bytes;
    std::map<uint32_t, uint32_t> allocated_blocks;
    std::map<uint32_t, uint32_t> free_blocks_by_addr;
    std::map<uint32_t, uint32_t> free_blocks_by_size;

    Arena(uint32_t addr, size_t size);
    Arena(const Arena&) = delete;
    Arena(Arena&&);
    Arena& operator=(const Arena&) = delete;
    Arena& operator=(Arena&&);
    ~Arena();

    void split_free_block(
        uint32_t free_block_addr,
        uint32_t allocate_addr,
        uint32_t allocate_size);
    void delete_free_block(uint32_t addr, uint32_t size);
  };

  inline uint32_t page_base_for_addr(uint32_t addr) const {
    return (addr & ~(this->page_size - 1));
  }

  inline uint32_t page_number_for_addr(uint32_t addr) const {
    return this->page_base_for_addr(addr) >> this->page_bits;
  }

  inline size_t page_size_for_size(size_t size) const {
    return ((size + (this->page_size - 1)) & ~(this->page_size - 1));
  }

  inline size_t page_count_for_size(size_t size) const {
    return this->page_size_for_size(size) >> this->page_bits;
  }

  uint32_t find_arena_space(
      uint32_t addr_low, uint32_t addr_high, uint32_t size) const;
  std::shared_ptr<Arena> create_arena(uint32_t addr, size_t min_size);
  void delete_arena(std::shared_ptr<Arena> arena);

  // TODO: We probably should have an index of {free block size: Arena ptr} to
  // make allocations sub-linear time. I'm not going to implement this just yet.
  std::map<uint32_t, std::shared_ptr<Arena>> arenas_by_addr;
  std::vector<std::shared_ptr<Arena>> arena_for_page_number;

  std::unordered_map<std::string, uint32_t> symbol_addrs;
};
