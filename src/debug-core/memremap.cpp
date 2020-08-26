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
#include "memremap.h"

constexpr const char target_addr::addr_space_map[];

target_addr::target_addr()
    : space(AS_UNDEF)
    , addr(INVALID_ADDR) {}

target_addr::target_addr(target_addr_space space, ADDR addr)
    : space(space)
    , addr(addr) {}

target_addr target_addr::from_name(char name, ADDR addr) {
  target_addr_space space = AS_UNDEF;

  for (int i = 0; i < sizeof(addr_space_map); i++) {
    if (addr_space_map[i] == name) {
      space = target_addr_space(i);
      break;
    }
  }

  return {space, addr};
}