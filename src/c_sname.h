/*
**      cdecl -- C gibberish translator
**      src/c_sname.h
**
**      Copyright (C) 2019-2021  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_c_sname_H
#define cdecl_c_sname_H

/**
 * @file
 * Declares functions for dealing with "sname" (C++ scoped name) objects, e.g.,
 * `S::T::x`.
 *
 * + An sname also has a type (one of of #TB_CLASS, #TB_NAMESPACE, #TB_SCOPE,
 *   #TB_STRUCT, or #TB_UNION) for each scope.
 * + The "local" of an sname is the innermost scope, e.g., `x`.  A non-empty
 *   sname always has a local.
 * + The "scope" of an sname is all but the innermost scope, e.g., `S::T`.  A
 *   non-empty sname may or may not have a scope.
 *
 * @note For C, an sname is simply a single (unscoped) name, e.g., `x`.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_type.h"
#include "slist.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef C_SNAME_INLINE
# define C_SNAME_INLINE _GL_INLINE
#endif /* C_SNAME_INLINE */

/// @endcond

/**
 * @defgroup sname-group Scoped Names
 * Functions for accessing and manipulating C++ scoped names.
 * @{
 */

/**
 * Creates an sname variable \a VAR on the stack having a local \a NAME.
 *
 * @param VAR The name for the sname variable.
 * @param NAME The name.
 */
#define SNAME_VAR_INIT(VAR,NAME)                  \
  c_scope_data_t VAR##_data = { (NAME), T_NONE }; \
  SLIST_VAR_INIT( VAR, NULL, &VAR##_data )

/**
 * Gets the data associated with \a SCOPE.
 *
 * @param SCOPE The <code>\ref c_scope_data</code> to get the data of.
 *
 * @note This is a macro instead of an inline function so it'll work with
 * either `const` or non-`const` \a SCOPE.
 */
#define c_scope_data(SCOPE) \
  REINTERPRET_CAST( c_scope_data_t*, (SCOPE)->data )

/**
 * Convenience macro for iterating over all scopes of an sname.
 *
 * @param VAR The `c_scope_t` loop variable.
 * @param SNAME The `sname` to iterate over the scopes of.
 * @param END The scope to end before; may be NULL.
 */
#define FOREACH_SCOPE(VAR,SNAME,END) \
  FOREACH_SLIST( VAR, SNAME, END )

///////////////////////////////////////////////////////////////////////////////

/**
 * Data for each scope of an sname.
 */
struct c_scope_data {
  /**
   * The scope's name.
   */
  char const *name;

  /**
   * The scope's type, one of: #TB_CLASS, #TB_STRUCT, #TB_UNION, [#TS_INLINE]
   * #TB_NAMESPACE, or #TB_SCOPE.
   */
  c_type_t type;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Compares two <code>\ref c_scope_data</code>s.
 *
 * @param i_data The first <code>\ref c_scope_data</code> to compare.
 * @param j_data The second <code>\ref c_scope_data</code> to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_data is
 * less than, equal to, or greater than \a j_data, respectively.
 */
PJL_WARN_UNUSED_RESULT
int c_scope_data_cmp( c_scope_data_t *i_data, c_scope_data_t *j_data );

/**
 * Duplicates \a data.  The caller is responsible for calling
 * c_scope_data_free() on the duplicate.
 *
 * @param data The <code>\ref c_scope_data</code> to duplicate.
 * @return Returns a duplicate of \a data.
 */
PJL_WARN_UNUSED_RESULT
c_scope_data_t* c_scope_data_dup( c_scope_data_t const *data );

/**
 * Frees all memory associated with \a data _including_ \a data itself.
 *
 * @param data The <code>\ref c_scope_data</code> to free.  If NULL, does
 * nothing.
 */
void c_scope_data_free( c_scope_data_t *data );

/**
 * Appends \a name onto the end of \a sname.
 *
 * @param sname The scoped name to append to.
 * @param name The name to append.  Ownership is taken.
 *
 * @sa c_sname_append_sname()
 * @sa c_sname_prepend_sname()
 */
void c_sname_append_name( c_sname_t *sname, char *name );

/**
 * Appends \a src onto the end of \a dst.
 *
 * @param dst The scoped name to append to.
 * @param src The scoped name to append.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_prepend_sname()
 */
C_SNAME_INLINE
void c_sname_append_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_tail( dst, src );
}

/**
 * Compares two scoped names.
 *
 * @param i_sname The first scoped name to compare.
 * @param j_sname The second scoped name to compare.
 * @return Returns a number less than 0, 0, or greater than 0 if \a i_sname is
 * less than, equal to, or greater than \a j_sname, respectively.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
int c_sname_cmp( c_sname_t const *i_sname, c_sname_t const *j_sname ) {
  return slist_cmp(
    i_sname, j_sname, (slist_node_data_cmp_fn_t)&c_scope_data_cmp
  );
}

/**
 * Gets the number of names of \a sname, e.g., `S::T::x` is 3.
 *
 * @param sname The scoped name to get the number of names of.
 * @return Returns said number of names.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
size_t c_sname_count( c_sname_t const *sname ) {
  return slist_len( sname );
}

/**
 * Duplicates \a sname.  The caller is responsible for calling c_sname_free()
 * on the duplicate.
 *
 * @param sname The scoped name to duplicate.
 * @return Returns a duplicate of \a sname.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
c_sname_t c_sname_dup( c_sname_t const *sname ) {
  return slist_dup(
    sname, -1, NULL, (slist_node_data_dup_fn_t)&c_scope_data_dup
  );
}

/**
 * Gets whether \a name is empty.
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname is empty.
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
bool c_sname_empty( c_sname_t const *sname ) {
  return slist_empty( sname );
}

/**
 * Frees all memory associated with \a sname but _not_ \a sname itself.
 *
 * @param sname The scoped name to free.  If NULL, does nothing; otherwise,
 * reinitializes it upon completion.
 */
C_SNAME_INLINE
void c_sname_free( c_sname_t *sname ) {
  slist_free( sname, NULL, (slist_data_free_fn_t)&c_scope_data_free );
}

/**
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The scoped name to get the full name of.
 * @return Returns said name or the empty string if \a sname is empty.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name()
 */
PJL_WARN_UNUSED_RESULT
char const* c_sname_full_name( c_sname_t const *sname );

/**
 * Initializes \a sname.
 *
 * @param sname The scoped name to initialize.
 *
 * @note This need not be called for either global or `static` scoped names.
 *
 * @sa c_sname_free()
 * @sa c_sname_init_name()
 */
C_SNAME_INLINE
void c_sname_init( c_sname_t *sname ) {
  slist_init( sname );
}

/**
 * Initializes \a sname with \a name.
 *
 * @param sname The scoped name to initialize.
 * @param name The name to set to.  Ownership is taken.
 *
 * @sa c_sname_free()
 * @sa c_sname_init()
 */
C_SNAME_INLINE
void c_sname_init_name( c_sname_t *sname, char *name ) {
  slist_init( sname );
  c_sname_append_name( sname, name );
}

/**
 * Gets whether \a sname is a constructor name, i.e., whether the last two
 * names match, e.g. `S::T::T`.
 *
 * @note This can also be used to check for destructor names since cdecl elides
 * the `~` when parsing them.  (An AST's kind is #K_DESTRUCTOR.)
 *
 * @param sname The scoped name to check.
 * @return Returns `true` only if \a sname has at least two names and the last
 * two names match.
 */
PJL_WARN_UNUSED_RESULT
bool c_sname_is_ctor( c_sname_t const *sname );

/**
 * Gets the local (last) name of \a sname, e.g., the local name of `S::T::x` is
 * `x`.
 *
 * @param sname The scoped name to get the local name of.
 * @return Returns said name or the empty string if \a sname is empty.
 *
 * @sa c_sname_full_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_name()
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
char const* c_sname_local_name( c_sname_t const *sname ) {
  c_scope_data_t const *const local_data = slist_peek_tail( sname );
  return local_data == NULL ? "" : local_data->name;
}

/**
 * Gets the local scope type of \a sname (which is the type of the innermost
 * scope).
 *
 * @param sname The scoped name to get the scope type of.
 * @return Returns the local scope type.
 * @return Returns the local scoped type or #T_NONE if \a sname is empty.
 *
 * @sa c_sname_scope_type()
 * @sa c_sname_set_local_type()
 */
c_type_t const* c_sname_local_type( c_sname_t const *sname );

/**
 * Checks whether \a sname matches \a glob where \a glob is glob-like in that
 * `*` matches zero or more characters.  However, `*` matches only within a
 * single scope.  Examples:
 *
 *  + `foo*` matches all names starting with `foo` in the global scope.
 *  + `s::&zwj;*foo` matches all names ending with `foo` only within the top-
 *    level scope `s`.
 *  + `s*::&zwj;foo` matches all names equal to `foo` in all top-level scopes
 *    starting with `s`.
 *  + `s::*::&zwj;foo` matches all names equal to `foo` in any scope within the
 *    top-level scope `s`.
 *
 * Additionally, a leading `**` is used to match within any scope.  Examples:
 *
 *  + `**::&zwj;foo` matches all names equal to `foo` in any scope.
 *
 * @param sname The scoped name to match against.
 * @param glob The glob-like pattern to match.  It _must_ be a valid glob.
 * @return Returns `true` only if \a sname matches \a glob.
 */
PJL_WARN_UNUSED_RESULT
bool c_sname_match( c_sname_t const *sname, char const *glob );

/**
 * Gets the name at \a roffset of \a sname.
 *
 * @param sname The scoped name to get the name at \a roffset of.
 * @param roffset The reverse offset (starting at 0) of the name to get.
 * @return Returns the name at \a roffset or the empty string if \a roffset
 * &gt;= c_sname_count().
 *
 * @sa c_sname_full_name()
 * @sa c_sname_scope_name()
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
char const* c_sname_name_atr( c_sname_t const *sname, size_t roffset ) {
  c_scope_data_t const *const data = slist_peek_atr( sname, roffset );
  return data != NULL ? data->name : "";
}

/**
 * Parses a scoped name, e.g. `a::b::c`.
 *
 * @param s The string to parse.
 * @param sname The `c_sname` to parse into.
 * @return Returns `true` only if the scoped name was successfully parsed.
 */
PJL_WARN_UNUSED_RESULT
bool c_sname_parse( char const *s, c_sname_t *sname );

/**
 * Prepends \a src onto the beginning of \a dst.
 *
 * @param dst The scoped name to prepend to.
 * @param src The name to prepend.
 *
 * @sa c_sname_append_name()
 * @sa c_sname_append_sname()
 */
C_SNAME_INLINE
void c_sname_prepend_sname( c_sname_t *dst, c_sname_t *src ) {
  slist_push_list_head( dst, src );
}

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The scoped name to get the scope name of.
 * @return Returns said name or the empty string if \a sname is empty or not
 * within a scope.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_full_name()
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 */
PJL_WARN_UNUSED_RESULT
char const* c_sname_scope_name( c_sname_t const *sname );

/**
 * Gets the scope type of the scope of \a sname.
 *
 * @param sname The scoped name to get the scope type of the scope of.
 * @return Returns the scope type or #T_NONE if \a sname is empty or not within
 * a scope.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_set_scope_type()
 */
C_SNAME_INLINE PJL_WARN_UNUSED_RESULT
c_type_t const* c_sname_scope_type( c_sname_t const *sname ) {
  c_scope_data_t const *const data = slist_peek_atr( sname, 1 );
  return data != NULL ? &data->type : &T_NONE;
}

/**
 * Sets the scope type of \a sname (which is the type of the innermost scope).
 *
 * @param sname The scoped name to set the scope type of.
 * @param type The type.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_set_scope_type()
 */
C_SNAME_INLINE
void c_sname_set_local_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data( sname->tail )->type = *type;
}

/**
 * Sets the scope type \a sname (which is the type of the next-most innermost
 * scope) or does nothing if \a sname has no scope.
 *
 * @param sname The scoped name to set the scope type of.
 * @param type The type.
 *
 * @sa c_sname_local_type()
 * @sa c_sname_set_scope_type()
 */
C_SNAME_INLINE
void c_sname_set_scope_type( c_sname_t *sname, c_type_t const *type ) {
  c_scope_data_t *const data = slist_peek_atr( sname, 1 );
  if ( data != NULL )
    data->type = *type;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_sname_H */
/* vim:set et sw=2 ts=2: */
