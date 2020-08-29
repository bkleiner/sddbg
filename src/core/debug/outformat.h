#pragma once

#include <stdint.h>
#include <string>

#include "dbgsession.h"

namespace debug::core {
  class out_format {
  public:
    enum ENDIAN {
      ENDIAN_BIG,
      ENDIAN_LITTLE
    };

    out_format(dbg_session *session);

    void set_endian(ENDIAN e);

    /** Print a memory location in GDB output format as specified.
		\param fmt			Format character as per gdb print
		\param flat_addr	Lowest address of object to print.
		\param size			Number of bytes
		\returns 			The formatted std::string.
	*/
    std::string print(char fmt, target_addr addr, uint32_t size);

  private:
    dbg_session *mSession;
    ENDIAN mTargetEndian;

    /** Read an unsigned integer from the device starting at the spcififed 
		address.
		The endian flag is obayed and size is the number of bytes.
	*/
    uint32_t get_uint(target_addr addr, char size);

    /** Read an signed integer from the device starting at the spcififed 
	address.
	The endian flag is obayed and size is the number of bytes.
	 */
    int32_t get_int(target_addr addr, char size);
  };

} // namespace debug::core
