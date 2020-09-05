#include "target.h"

#include <cstdio>
#include <cstring>

#include "ihex.h"
#include "log.h"

namespace debug::core {

  target::target()
      : force_stop(false) {
  }

  target::~target() {
  }

  void target::print_buf_dump(char *buf, int len) {
    const int PerLine = 16;
    int i, addr;

    for (addr = 0; addr < len; addr += PerLine) {
      log::printf("%05x\t", (unsigned int)addr);

      for (i = 0; i < PerLine; i++)
        log::printf("%02x ", (unsigned int)buf[addr + i] & 0xff);

      log::printf("\t");

      for (i = 0; i < PerLine; i++)
        log::print("{}", (buf[addr + i] >= '0' && buf[addr + i] <= 'z') ? buf[addr + i] : '.');

      log::print("\n");
    }
  }

  /** Default implementation, load an intel hex file and use write_code to place
	it in memory
*/
  bool target::load_file(std::string name) {
    // set all data to 0xff, since this is the default erased value for flash
    char buf[0x20000];
    memset(buf, 0xff, 0x20000);

    log::print("Loading file '{}'\n", name);

    uint32_t start, end;
    if (!ihex_load_file(name.c_str(), buf, &start, &end)) {
      return false;
    }

    print_buf_dump(buf, end - start);
    log::printf("start %d %d\n", start, end);

    const uint32_t size = end - start + 1;
    write_code(start, end - start + 1, (uint8_t *)&buf[start]);

    char verify[size];
    read_code(start, size, (uint8_t *)verify);

    for (size_t i = 0; i < size; i++) {
      if (buf[start + i] != verify[i]) {
        return false;
      }
    }

    write_PC(start);
    return true;
  }

  void target::stop() {
    force_stop = true;
  }

  bool target::check_stop_forced() {
    if (force_stop) {
      force_stop = false;
      return true;
    }
    return false;
  }

  /** derived calsses must call this function to ensure the cache is updated.
*/
  void target::write_sfr(uint8_t addr,
                         uint8_t page,
                         uint8_t len,
                         unsigned char *buf) {
    SFR_PAGE_LIST::iterator it;
    it = cache_get_sfr_page(page);

    if (it != mCacheSfrPages.end()) {
      // update values in cache
      memcpy((*it).buf + (addr - 0x80), buf, len);
    }
  }

  void target::invalidate_cache() {
    mCacheSfrPages.clear();
  }

  void target::read_sfr_cache(uint8_t addr,
                              uint8_t page,
                              uint8_t len,
                              unsigned char *buf) {
    SFR_PAGE_LIST::iterator it;
    it = cache_get_sfr_page(page);

    if (it == mCacheSfrPages.end()) {
      // not in cache, read it and cache it.
      SFR_CACHE_PAGE page_entry;
      page_entry.page = page;
      read_sfr(0x80, page_entry.page, 128, page_entry.buf);
      mCacheSfrPages.push_back(page_entry);
      memcpy(buf, page_entry.buf + (addr - 0x80), len);
    } else {
      // in cache
      memcpy(buf, (*it).buf + (addr - 0x80), len);
    }
  }

  void target::read_memory(target_addr addr, int len, uint8_t *buf) {
    switch (addr.space) {
    case target_addr::AS_CODE:
    case target_addr::AS_CODE_STATIC:
      return read_code(addr.addr, len, buf);

    case target_addr::AS_ISTACK:
    case target_addr::AS_IRAM_LOW:
    case target_addr::AS_INT_RAM:
      return read_data(addr.addr, len, buf);

    case target_addr::AS_XSTACK:
    case target_addr::AS_EXT_RAM:
      return read_xdata(addr.addr, len, buf);

    case target_addr::AS_SFR:
      return read_sfr(addr.addr, len, buf);

    case target_addr::AS_REGISTER: {
      uint8_t offset = 0;
      read_sfr(0xd0, 1, &offset);
      return read_data(addr.addr + (offset & 0x18), len, buf);
    }

    default:
      break;
    }
  }
} // namespace debug::core