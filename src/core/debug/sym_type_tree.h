#pragma once

#include <string>
#include <vector>

#include "context_mgr.h"
#include "dbg_session.h"
#include "mem_remap.h"

namespace debug::core {

  class target;

  /** Details about a single type.
	derived versions implement the basic types
*/
  class sym_type {
  public:
    sym_type(dbg_session *session, std::string name)
        : session(session)
        , m_name(name) {}

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
    dbg_session *session;
    std::string m_name;
    std::string m_filename;
  };

  /** This is a terminal type in that it is not made up of any other types
*/
  class sym_type_char : public sym_type {
  public:
    sym_type_char(dbg_session *session)
        : sym_type(session, "char") {}
    ~sym_type_char() {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 1; }
    virtual char default_format() { return 'c'; }

    virtual std::string text() { return "char"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_uchar : public sym_type {
  public:
    sym_type_uchar(dbg_session *session)
        : sym_type(session, "unsigned char") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 1; }
    virtual char default_format() { return 'c'; }

    virtual std::string text() { return "unsigned char"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_short : public sym_type {
  public:
    sym_type_short(dbg_session *session)
        : sym_type(session, "short") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 1; }
    virtual char default_format() { return 'd'; }

    virtual std::string text() { return "short"; }

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_ushort : public sym_type {
  public:
    sym_type_ushort(dbg_session *session)
        : sym_type(session, "unsigned short") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 1; }
    virtual char default_format() { return 'u'; }

    virtual std::string text() { return "unsigned short"; }

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_int : public sym_type {
  public:
    sym_type_int(dbg_session *session)
        : sym_type(session, "int") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 2; }
    virtual char default_format() { return 'd'; }

    virtual std::string text() { return "int"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_uint : public sym_type {
  public:
    sym_type_uint(dbg_session *session)
        : sym_type(session, "unsigned int") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 2; }
    virtual char default_format() { return 'u'; }

    virtual std::string text() { return "unsigned int"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_long : public sym_type {
  public:
    sym_type_long(dbg_session *session)
        : sym_type(session, "long") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 4; }
    virtual char default_format() { return 'd'; }

    virtual std::string text() { return "long"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_ulong : public sym_type {
  public:
    sym_type_ulong(dbg_session *session)
        : sym_type(session, "unsigned long") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 4; }
    virtual char default_format() { return 'u'; }

    virtual std::string text() { return "unsigned long"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_float : public sym_type {
  public:
    sym_type_float(dbg_session *session)
        : sym_type(session, "float") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 4; }
    virtual char default_format() { return 'f'; }

    virtual std::string text() { return "float"; }
    virtual std::string pretty_print(char fmt, target_addr addr);

  protected:
  };

  /** This is a terminal type in that it is not made up of any other types
 */
  class sym_type_sbit : public sym_type {
  public:
    sym_type_sbit(dbg_session *session)
        : sym_type(session, "sbit") {}

    virtual bool terminal() { return true; }
    virtual int32_t size() { return 1; }

    virtual std::string text() { return "sbit"; }

  protected:
  };

  /** This is a non terminal type in that is is made up of a list of type objects.
	@FIXME this shoulden't store other types, just names of other types???
	maybe still link to the other types if they are defined.
*/
  class sym_type_struct : public sym_type {
  public:
    struct member {
      member();
      member(ADDR offset,
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

    sym_type_struct(dbg_session *session)
        : sym_type(session, "") {}

    ~sym_type_struct() {}

    virtual bool terminal() { return false; }
    virtual int32_t size();
    virtual std::string text();

    void add_member(ADDR offset, std::string member_name, std::string type_name, uint32_t count);

    ADDR get_member_offset(std::string member_name);
    sym_type *get_member_type(std::string member_name);

    const std::vector<member> &get_members() {
      return m_members;
    }

  protected:
    std::vector<member> m_members;
  };

  class sym_type_tree {
  public:
    sym_type_tree(dbg_session *session);
    ~sym_type_tree();

    void dump();
    void dump(std::string type_name);

    bool add_type(sym_type *ptype);

    sym_type *get_type(std::string type_name, context ctx);

    std::string pretty_print(sym_type *ptype, char fmt, uint32_t flat_addr, std::string subpath);
    virtual void clear();

  protected:
    dbg_session *session;

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

    typedef std::vector<sym_type *> TYPE_VEC;
    typedef TYPE_VEC::iterator TYPE_VEC_IT;
    typedef std::vector<TYPESCOPE> TYPE_SCOPE_VEC;
    typedef TYPE_SCOPE_VEC::iterator TYPE_SCOPE_VEC_IT;

    TYPE_VEC m_types;
    TYPE_SCOPE_VEC m_types_scope;
  };

} // namespace debug::core
