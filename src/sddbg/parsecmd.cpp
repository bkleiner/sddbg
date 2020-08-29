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
#include <iostream>

#include "cmdcommon.h"
#include "parsecmd.h"
#include "types.h"

ParseCmd::ParseCmd() {
}

ParseCmd::~ParseCmd() {
}

ParseCmd::Args ParseCmd::tokenize(const std::string &str, const std::string &delimiters) {
  ParseCmd::Args tokens;

  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the std::vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}

bool ParseCmd::match(const std::string &token, const std::string &mask) {
  return token.compare(mask) == 0;
}

std::string ParseCmd::join(ParseCmd::Args args, const std::string &delimiter) {
  std::string res = "";
  while (!args.empty()) {
    res += args.front();
    args.pop_front();

    if (!args.empty())
      res += delimiter;
  }
  return res;
}

bool CmdShowSetInfoHelp::parse(std::string str_args) {
  ParseCmd::Args tokens = tokenize(str_args);
  if (tokens.empty()) {
    return false;
  }

  Mode mode = DIRECT;
  std::string cmd = tokens.front();
  tokens.pop_front();

  if (!tokens.empty()) {
    if (match(cmd, "set")) {
      mode = SET;
    } else if (match(cmd, "show")) {
      mode = SHOW;
    } else if (match(cmd, "info")) {
      mode = INFO;
    } else if (match(cmd, "help")) {
      mode = HELP;
    }
  }
  if (mode == DIRECT) {
    if (compare_name(cmd) == -1) {
      return false;
    }
    if (tokens.empty()) {
      return directnoarg();
    }
    return direct(tokens);
  }

  if (compare_name(tokens.front()) == -1) {
    return false;
  }
  tokens.pop_front();

  switch (mode) {
  case SET:
    return set(tokens);
  case SHOW:
    return show(tokens);
  case INFO:
    return info(tokens);
  case HELP:
    return help(tokens);
  }
  return false;
}

/** Compares s to the name associated with this command
	parsing stops at first space or '/'
	\returns -1 if no match otherwise the length of characters of the tag
*/
int CmdShowSetInfoHelp::compare_name(std::string s) {
  // upper case letters in name at the start represent the shortest form of the command supported.
  // we will only match is at least thase are correct and any following characters match
  int i = 0;
  if (s.length() == 0)
    return -1; // no match
  // match first part
  while (isupper(name[i])) {
    if (i >= s.length())
      return -1; // no match
    if (tolower(name[i]) != tolower(s[i]))
      return -1; // no match
    i++;
  }
  for (; i < s.length(); i++) {
    if (s[i] == ' ')
      return i;
    if (s[i] == '/')
      return i - 1;
    if (tolower(name[i]) != tolower(s[i]))
      return -1;
  }
  return s.length();
}
