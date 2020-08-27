/***************************************************************************
 *   Copyright (C) 2005 by Ricky White   *
 *   rickyw@neatstuff.co.nz   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef OUTFORMAT_H
#define OUTFORMAT_H
#include "dbgsession.h"
#include <stdint.h>
#include <string>

/** Output Format for printing values
	Manages the pringing of various values given their specifiers as per the
	print format options.

	@author Ricky White <rickyw@neatstuff.co.nz>
*/
class OutFormat {
public:
  enum ENDIAN {
    ENDIAN_BIG,
    ENDIAN_LITTLE
  };

  OutFormat(DbgSession *session);
  ~OutFormat();

  void set_endian(ENDIAN e);

  /** Print a memory location in GDB output format as specified.
		\param fmt			Format character as per gdb print
		\param flat_addr	Lowest address of object to print.
		\param size			Number of bytes
		\returns 			The formatted std::string.
	*/
  std::string print(char fmt, target_addr addr, uint32_t size);

private:
  DbgSession *mSession;
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

#endif
