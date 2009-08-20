// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef GNASH_FN_CALL_H
#define GNASH_FN_CALL_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "as_environment.h" // for inlines (arg)
#include "as_object.h" // for dtor visibility by boost::intrusive_ptr
#include "smart_ptr.h"
#include "VM.h"

#include <cassert> // for inlines (arg)
#include <ostream> // for inlines (dump_args)
#include <sstream> // for inlines (dump_args)

// Forward declarations
namespace gnash {
    class as_environment;
    class as_function;
    class as_object;
    class as_value;
    class movie_definition;
}

namespace gnash {

/// A class to contain transferable arguments for a fn_call.
//
/// The operators += and , are implemented for intuitive syntax:
//
/// FunctionArgs<as_value> args; args += 0.0, "string", NaN.
//
/// This may have unexpected side effects if it is used in unexpected ways,
/// so stick to using such lists, or use operator += repeatedly.
//
/// The arguments can be moved to another container, and this happens when
/// the FunctionArgs object is passed to fn_call. It will still be valid
/// afterwards, but will contain no arguments.
template<typename T>
class FunctionArgs
{
public:

    typedef typename std::vector<T>::size_type size_type;
    typedef std::vector<T> container_type;
    typedef T value_type;

    FunctionArgs() {}

    FunctionArgs& operator+=(const T& t) {
        _v.push_back(t);
        return *this;
    }

    FunctionArgs& operator,(const T& t) {
        _v.push_back(t);
        return *this;
    }

    void swap(std::vector<T>& to) {
        std::swap(_v, to);
    }

    size_type size() const {
        return _v.size();
    }

private:
    std::vector<T> _v;
};


/// \brief
/// Parameters/environment for builtin or user-defined functions
/// callable from ActionScript.
class fn_call
{
public:
    typedef FunctionArgs<as_value> Args;

	/// The as_object (or a pointer derived thereof) on which this call
	/// is taking place.
	as_object* this_ptr;

	/// The "super" object in this function call context
	as_object* super;

	/// Number of arguments to this ActionScript function call.
    Args::size_type nargs;

    /// Definition containing caller code. 0 if spontaneous (system event).
    const movie_definition* callerDef;

    fn_call(const fn_call& fn)
        :
        this_ptr(fn.this_ptr),
        super(fn.super),
		nargs(fn.nargs),
        callerDef(fn.callerDef),
        _new(false),
        _env(fn._env),
        _args(fn._args)
	{
	}

	fn_call(const fn_call& fn, as_object* this_in, as_object* sup = 0)
		:
        this_ptr(this_in),
        super(sup),
        nargs(fn.nargs),
        callerDef(fn.callerDef),
        _new(false),
		_env(fn._env),
        _args(fn._args)
	{
	}

	fn_call(as_object* this_in, const as_environment& env_in,
			int nargs_in, size_t first_in, as_object* sup = 0)
		:
		this_ptr(this_in),
		super(sup),
		nargs(nargs_in),
        callerDef(0),
        _new(false),
		_env(env_in)
	{
		assert(first_in + 1 == env_in.stack_size());
		readArgs(env_in, first_in, nargs);
	}

	fn_call(as_object* this_in, const as_environment& env_in,
            Args& args, as_object* sup = 0, bool isNew = false)
		:
		this_ptr(this_in),
		super(sup),
		nargs(args.size()),
        callerDef(0),
        _new(isNew),
		_env(env_in)
	{
        args.swap(_args);
	}

	fn_call(as_object* this_in, const as_environment& env_in)
		:
		this_ptr(this_in),
		super(0),
		nargs(0),
        callerDef(0),
        _new(false),
		_env(env_in)
	{
	}

    /// Return the VM this fn_call is running from
    VM& getVM() const
    {
        return _env.getVM();
    }

	/// Return true if this call is an object instantiation
	bool isInstantiation() const
	{
		// Currently the as_function::constructInstance
		// will set 'this_ptr' to NULL when calling a builtin
		// function, so we use this info to find out.
		// For the future, we might use an explicit flag instead
		// as I belive there are some cases in which 'this' is
		// undefined even in a normal function call.
		return _new;
	}

	/// Access a particular argument.
	const Args::value_type& arg(unsigned int n) const
	{
		assert(n < nargs);
		return _args[n]; 
	}

    const Args::container_type& getArgs() const {
        return _args;
    }

	void drop_bottom()
	{
		assert(!_args.empty());
        _args.erase(_args.begin());
		--nargs;
	}

	const as_environment& env() const
	{
		return _env;
	}

	/// Dump arguments to given output stream
	void dump_args(std::ostream& os) const
	{
		for (size_t i = 0; i < nargs; ++i)
		{
			if ( i ) os << ", ";
			os << arg(i).toDebugString();
		}
	}

	/// Return arguments as a string (for debugging)
	std::string dump_args() const
	{
		std::stringstream ss;
		dump_args(ss);
		return ss.str();
	}

	void resetArgs()
	{
		nargs = 0;
		_args.clear();
	}

	void pushArg(const Args::value_type& arg)
	{
		++nargs;
		_args.push_back(arg);
	}

private:

    bool _new;

	/// The ActionScript environment in which the function call is taking
	/// place. This contains, among other things, the function arguments.
	const as_environment& _env;

	/// The actual arguments
    Args::container_type _args;

	void readArgs(const as_environment& env, int first_in, size_t nargs)
	{
		_args.clear();
		for (size_t i = 0; i < nargs; ++i) {
			_args.push_back(env.bottom(first_in - i));
        }
	}

};

/// Check whether the currently executing code is AS3 (ABC)
//
/// This is a non-member, non-friend function for better encapsulation.
/// TODO: drop these when there is a better design!
inline bool
isAS3(const fn_call& fn)
{
    return fn.getVM().getAVMVersion() == VM::AVM2;
}

inline string_table&
getStringTable(const fn_call& fn)
{
    return fn.getVM().getStringTable();
}

inline movie_root&
getRoot(const fn_call& fn)
{
    return fn.getVM().getRoot();
}

inline int
getSWFVersion(const fn_call& fn)
{
    return fn.getVM().getSWFVersion();
}

inline VM&
getVM(const fn_call& fn)
{
    return fn.getVM();
}

inline Global_as*
getGlobal(const fn_call& fn)
{
    return fn.getVM().getGlobal();
}

} // namespace gnash


#endif 


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
