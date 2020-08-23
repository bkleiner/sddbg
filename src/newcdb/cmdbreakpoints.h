/***************************************************************************
 *   Copyright (C) 2006 by Ricky White   *
 *   ricky@localhost.localdomain   *
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
#ifndef CMDBREAKPOINTS_H
#define CMDBREAKPOINTS_H

#include "parsecmd.h"

/**
	@author Ricky White <ricky@localhost.localdomain>
*/
class CmdBreakpoints : public CmdShowSetInfoHelp {
public:
  CmdBreakpoints() { name = "BREAKPoints"; }

  bool show(ParseCmd::Args cmd) override;
  bool info(ParseCmd::Args cmd) override;
};

class CmdBreak : public CmdShowSetInfoHelp {
public:
  CmdBreak() { name = "BReak"; }
  bool direct(ParseCmd::Args cmd) override;
  bool directnoarg() override;
  bool help(ParseCmd::Args cmd) override;
};

class CmdTBreak : public CmdShowSetInfoHelp {
public:
  CmdTBreak() { name = "TBreak"; }
  bool direct(ParseCmd::Args cmd) override;
  bool directnoarg() override;
  bool help(ParseCmd::Args cmd) override;
};

class CmdClear : public CmdShowSetInfoHelp {
public:
  CmdClear() { name = "CLear"; }
  bool direct(ParseCmd::Args cmd) override;
  bool directnoarg() override;
};

class CmdDelete : public CmdShowSetInfoHelp {
public:
  CmdDelete() { name = "DElete"; }
  bool direct(ParseCmd::Args cmd) override;
};

class CmdDisable : public CmdShowSetInfoHelp {
public:
  CmdDisable() { name = "DIsable"; }
  bool direct(ParseCmd::Args cmd) override;
};

class CmdEnable : public CmdShowSetInfoHelp {
public:
  CmdEnable() { name = "ENable"; }
  bool direct(ParseCmd::Args cmd) override;
};

#endif
