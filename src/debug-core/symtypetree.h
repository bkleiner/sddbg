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
#ifndef SYMTYPETREE_H
#define SYMTYPETREE_H

#include <string>
#include <vector>

#include "contextmgr.h"
#include "dbgsession.h"
#include "memremap.h"

class Target;

/** Details about a single type.
	derived versions implement the basic types
*/
class SymType {
public:
  SymType(DbgSession *session, std::string name)
      : mSession(session)
      , m_name(name) {}
  virtual ~SymType() {}

  std::string name() { return m_name; }
  void set_name(std::string name) { m_name = name; }

  std::string file() { return m_filename; }
  void set_file(std::string name) { m_filename = name; }

  virtual bool terminal() = 0;
  virtual int32_t size() = 0;

  virtual std::string text() = 0;
  virtual char default_format() { return 'x'; }

  /** Print the symbol by using data from the specified ddress.
		\param fmt		GDB print format character that follows the slash.
		\param name		Name of the element
		\param address 	Address to begin printing from, will be updated with
		the address immediatly after the symbol	on return. (using flat remapped addrs)
		\returns the std::string representation of the symbol pretty printed.
	 */
  virtual std::string pretty_print(char fmt, target_addr addr) {
    return "not implemented";
  }

protected:
  DbgSession *mSession;
  std::string m_name;
  std::string m_filename;
};

/** This is a terminal type in that it is not made up of any other types
*/
class SymTypeChar : public SymType {
public:
  SymTypeChar(DbgSession *session)
      : SymType(session, "char") {}
  ~SymTypeChar() {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 1; }
  virtual char default_format() { return 'c'; }

  virtual std::string text() { return "char"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeUChar : public SymType {
public:
  SymTypeUChar(DbgSession *session)
      : SymType(session, "unsigned char") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 1; }
  virtual char default_format() { return 'c'; }

  virtual std::string text() { return "unsigned char"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeShort : public SymType {
public:
  SymTypeShort(DbgSession *session)
      : SymType(session, "short") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 1; }

  virtual std::string text() { return "short"; }

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeUShort : public SymType {
public:
  SymTypeUShort(DbgSession *session)
      : SymType(session, "unsigned short") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 1; }

  virtual std::string text() { return "unsigned short"; }

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeInt : public SymType {
public:
  SymTypeInt(DbgSession *session)
      : SymType(session, "int") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 2; }

  virtual std::string text() { return "int"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeUInt : public SymType {
public:
  SymTypeUInt(DbgSession *session)
      : SymType(session, "unsigned int") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 2; }

  virtual std::string text() { return "unsigned int"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeLong : public SymType {
public:
  SymTypeLong(DbgSession *session)
      : SymType(session, "long") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 4; }

  virtual std::string text() { return "long"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeULong : public SymType {
public:
  SymTypeULong(DbgSession *session)
      : SymType(session, "unsigned long") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 4; }

  virtual std::string text() { return "unsigned long"; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeFloat : public SymType {
public:
  SymTypeFloat(DbgSession *session)
      : SymType(session, "float") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 4; }

  virtual std::string text() { return "float"; }
  virtual char default_format() { return 'f'; }
  virtual std::string pretty_print(char fmt, target_addr addr);

protected:
};

/** This is a terminal type in that it is not made up of any other types
 */
class SymTypeSbit : public SymType {
public:
  SymTypeSbit(DbgSession *session)
      : SymType(session, "sbit") {}

  virtual bool terminal() { return true; }
  virtual int32_t size() { return 1; }

  virtual std::string text() { return "sbit"; }

protected:
};

/** This is a non terminal type in that is is made up of a list of type objects.
	@FIXME this shoulden't store other types, just names of other types???
	maybe still link to the other types if they are defined.
*/
class SymTypeStruct : public SymType {
public:
  struct Member {
    Member();
    Member(ADDR offset,
           std::string member_name,
           std::string type_name,
           uint32_t count)
        : offset(offset)
        , member_name(member_name)
        , type_name(type_name)
        , count(count) {}

    ADDR offset;
    std::string member_name;
    std::string type_name;
    uint32_t count;
  };

  SymTypeStruct(DbgSession *session)
      : SymType(session, "") {}

  ~SymTypeStruct() {}

  virtual bool terminal() { return false; }
  virtual int32_t size();
  virtual std::string text();

  void add_member(ADDR offset, std::string member_name, std::string type_name, uint32_t count);

  ADDR get_member_offset(std::string member_name);
  SymType *get_member_type(std::string member_name);

  const std::vector<Member> &get_members() {
    return m_members;
  }

protected:
  std::vector<Member> m_members;
};

/** Implements a tree of active data types in the project from the basic types 		
	through to nested structs.  The symbol table will referance this tree for
	extra information.
	This class provies a list of top level types, predefined are the terminal
	types.  All others are added as the cdb file is loaded.

	@author Ricky White <rickyw@neatstuff.co.nz>
*/
class SymTypeTree {
public:
  SymTypeTree(DbgSession *session);
  ~SymTypeTree();

  void dump();
  void dump(std::string type_name);

  bool add_type(SymType *ptype);

  //	bool get_type( std::string type_name,
  //				   std::string function,
  //				   std::string file,
  //				   SymType **type );
  SymType *get_type(std::string type_name, ContextMgr::Context context);

  /** Print out a type or part of a type starting at the specified address.
		\param ptype		Pointer to the type to begin with
		\param fmt			GDB print option char, 0 if auto
		\param flat_addr	Address of object in target memory
		\param subpath		path to the actual part to print, 
							This is used to print part of a structure
							eg a.b.c.
		\returns			std::string containing for formatted output.
		@FIXME do we really need this?  maybe this isn't the right place, maybe it is...
	if it stays here it should have an array option to print multiople copies 
	from consecutive memory locations.
	
		Maybe we need the context then this function can handle the lot...
	
		what about parsing user specified array indexeas such as:
			print a[3]
	
	*/
  std::string pretty_print(SymType *ptype, char fmt, uint32_t flat_addr, std::string subpath);
  virtual void clear();

protected:
  DbgSession *mSession;
  /// @FIXME TYPE_VEC needs extra information.
  ///  can have multiple types with same name in different scope.
  /// This needs some more thought and we can probably remove the file from
  /// the symtype class.  typeders are used in each file byt can also be
  /// within the scope fo a function.
  typedef struct
  {
    std::string function; // set if scope is function, null for file
    int block;            // block within function
  } TYPESCOPE;

  typedef std::vector<SymType *> TYPE_VEC;
  typedef TYPE_VEC::iterator TYPE_VEC_IT;
  typedef std::vector<TYPESCOPE> TYPE_SCOPE_VEC;
  typedef TYPE_SCOPE_VEC::iterator TYPE_SCOPE_VEC_IT;

  TYPE_VEC m_types;
  TYPE_SCOPE_VEC m_types_scope;
};

#endif
