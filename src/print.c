/*
**      cdecl -- C gibberish translator
**      src/print.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines functions for printing error and warning messages.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "print.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_sname.h"
#include "cdecl.h"
#include "color.h"
#include "lexer.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

// extern variables
extern char const        *command_line;
extern size_t             command_line_len;
extern size_t             inserted_len;
extern bool               is_input_a_tty;

/// @cond DOXYGEN_IGNORE

// local constants
static char const *const  MORE[]     = { "...", "..." };
static size_t const       MORE_LEN[] = { 3,     3     };
#ifdef ENABLE_TERM_SIZE
static unsigned const     TERM_COLUMNS_DEFAULT = 80;
#endif /* ENABLE_TERM_SIZE */

/// @endcond

// local functions
static size_t             token_len( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints the error line (if not interactive) and a `^` (in color, if possible
 * and requested) under the offending token.
 *
 * @param error_column The zero-based column of the offending token.
 */
static void print_caret( size_t error_column ) {
  if ( error_column >= inserted_len )
    error_column -= inserted_len;
  size_t error_column_term = error_column;

  unsigned term_columns = 0;
#ifdef ENABLE_TERM_SIZE
  get_term_columns_lines( &term_columns, NULL );
  if ( term_columns == 0 )
    term_columns = TERM_COLUMNS_DEFAULT;
#endif /* ENABLE_TERM_SIZE */

  if ( is_input_a_tty || opt_interactive ) {
    //
    // If we're interactive, we can put the ^ under the already existing token
    // the user typed for the recent command, but we have to add the length of
    // the prompt.
    //
    error_column_term += strlen( OPT_LANG_IS(C_ANY) ? CDECL : CPPDECL )
      + 2 /* "> " */;
    if ( term_columns )
      error_column_term %= term_columns;
  } else {
    --term_columns;                     // more aesthetically pleasing
    //
    // Otherwise we have to print out the line containing the error then put
    // the ^ under that.
    //
    size_t input_line_len;
    char const *input_line = lexer_input_line( &input_line_len );
    assert( input_line != NULL );
    if ( input_line_len == 0 ) {        // no input? try command line
      input_line = command_line;
      assert( input_line != NULL );
      input_line_len = command_line_len;
    }
    if ( input_line_len >= inserted_len ) {
      input_line += inserted_len;
      input_line_len -= inserted_len;
    }
    assert( error_column <= input_line_len );

    //
    // Chop off whitespace (if any) so we can always print a newline ourselves.
    //
    while ( ends_with_any_chr( input_line, input_line_len, " \f\r\t\n\v" ) )
      --input_line_len;

    //
    // If the error is due to unexpected end of input, back up the error column
    // so it refers to a non-null character.
    //
    if ( error_column > 0 && input_line[ error_column ] == '\0' )
      --error_column;

    size_t const token_columns = token_len( input_line + error_column );
    size_t const error_end_column = error_column + token_columns - 1;

    //
    // Start with the number of printable columns equal to the length of the
    // line.
    //
    size_t print_columns = input_line_len;

    //
    // If the number of printable columns exceeds the number of terminal
    // columns, there is "more" on the right, so limit the number of printable
    // columns.
    //
    bool more[2];                       // [0] = left; [1] = right
    more[1] = print_columns > term_columns;
    if ( more[1] )
      print_columns = term_columns;

    //
    // If the error end column is past the number of printable columns, there
    // is "more" on the left since we will "scroll" the line to the left.
    //
    more[0] = error_end_column > print_columns;

    //
    // However, if there is "more" on the right but end of the error token is
    // at the end of the line, then we can print through the end of the line
    // without any "more."
    //
    if ( more[1] ) {
      if ( error_end_column < input_line_len - 1 )
        print_columns -= MORE_LEN[1];
      else
        more[1] = false;
    }

    //
    // If there is "more" on the left, we have to adjust the error column, the
    // offset into the input line that we start printing at, and the number of
    // printable columns to give the appearance that the input line has been
    // "scrolled" to the left.
    //
    size_t print_offset;                // offset into input_line to print from
    if ( more[0] ) {
      error_column_term = print_columns - token_columns;
      print_offset = MORE_LEN[0] + (error_column - error_column_term);
      print_columns -= MORE_LEN[0];
    } else {
      print_offset = 0;
    }

    EPRINTF( "%s%.*s%s\n",
      (more[0] ? MORE[0] : ""),
      (int)print_columns, input_line + print_offset,
      (more[1] ? MORE[1] : "")
    );
  }

  EPRINTF( "%*s", (int)error_column_term, "" );
  SGR_START_COLOR( stderr, caret );
  EPUTC( '^' );
  SGR_END_COLOR( stderr );
  EPUTC( '\n' );
}

/**
 * Gets the length of the first token in \a s.  Characters are divided into
 * three classes:
 *
 *  + Whitespace.
 *  + Alpha-numeric.
 *  + Everything else (e.g., punctuation).
 *
 * A token is composed of characters in exclusively one class.  The class is
 * determined by `s[0]`.  The length of the token is the number of consecutive
 * characters of the same class starting at `s[0]`.
 *
 * @param s The null-terminated string to use.
 * @return Returns the length of the token.
 */
PJL_WARN_UNUSED_RESULT
static size_t token_len( char const *s ) {
  assert( s != NULL );

  char const *const s0 = s;
  bool const is_s0_alnum = isalnum( *s0 );
  bool const is_s0_space = isspace( *s0 );
  while ( *++s != '\0' ) {
    if ( is_s0_alnum ) {
      if ( !isalnum( *s ) )
        break;
    }
    else if ( is_s0_space ) {
      if ( !isspace( *s ) )
        break;
    }
    else {
      if ( isalnum( *s ) || isspace( *s ) )
        break;
    }
  } // while
  return (size_t)(s - s0);
}

////////// extern functions ///////////////////////////////////////////////////

void fl_print_error( char const *file, int line, c_loc_t const *loc,
                     char const *format, ... ) {
  assert( file != NULL );
  assert( format != NULL );

  if ( loc != NULL ) {
    print_loc( loc );
    SGR_START_COLOR( stderr, error );
    EPUTS( "error" );
    SGR_END_COLOR( stderr );
    EPUTS( ": " );
  }

  print_debug_file_line( file, line );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
}

void fl_print_error_unknown_name( char const *file, int line,
                                  c_loc_t const *loc, c_sname_t const *sname ) {
  assert( sname != NULL );

  // Must dup this since c_sname_full_name() returns a temporary buffer.
  char const *const name = check_strdup( c_sname_full_name( sname ) );

  c_keyword_t const *const k = c_keyword_find( name, LANG_ANY, C_KW_CTX_ALL );
  if ( k != NULL ) {
    dym_kind_t  dym_kind = DYM_NONE;
    char const *what = NULL;

    switch ( c_type_id_tpid( k->type_id ) ) {
      case C_TPID_NONE:                 // e.g., "break"
      case C_TPID_STORE:                // e.g., "extern"
        dym_kind = DYM_C_KEYWORDS;
        what = "keyword";
        break;
      case C_TPID_BASE:                 // e.g., "char"
        dym_kind = DYM_C_TYPES;
        what = "type";
        break;
      case C_TPID_ATTR:
        dym_kind = DYM_C_ATTRIBUTES;    // e.g., "noreturn"
        what = "attribute";
        break;
    } // switch

    fl_print_error( file, line, loc,
      "\"%s\": unsupported %s%s", name, what, c_lang_which( k->lang_ids )
    );
    print_suggestions( dym_kind, name );
  }
  else {
    fl_print_error( file, line, loc, "\"%s\": unknown name", name );
    print_suggestions( DYM_C_KEYWORDS | DYM_C_TYPES, name );
  }

  EPUTC( '\n' );
  FREE( name );
}

void fl_print_warning( char const *file, int line, c_loc_t const *loc,
                       char const *format, ... ) {
  assert( file != NULL );
  assert( format != NULL );

  if ( loc != NULL )
    print_loc( loc );
  SGR_START_COLOR( stderr, warning );
  EPUTS( "warning" );
  SGR_END_COLOR( stderr );
  EPUTS( ": " );

  print_debug_file_line( file, line );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
}

void print_debug_file_line( char const *file, int line ) {
  assert( file != NULL );
#ifdef ENABLE_CDECL_DEBUG
  if ( opt_cdecl_debug )
    EPRINTF( "[%s:%d] ", file, line );
#else
  (void)file;
  (void)line;
#endif /* ENABLE_CDECL_DEBUG */
}

void print_hint( char const *format, ... ) {
  assert( format != NULL );
  EPUTS( "; did you mean " );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  EPUTS( "?\n" );
}

void print_loc( c_loc_t const *loc ) {
  assert( loc != NULL );
  print_caret( (size_t)loc->first_column );
  SGR_START_COLOR( stderr, locus );
  if ( opt_conf_file != NULL )
    EPRINTF( "%s:%d,", opt_conf_file, loc->first_line + 1 );
  size_t column = (size_t)loc->first_column;
  if ( column >= inserted_len )
    column -= inserted_len;
  EPRINTF( "%zu", column + 1 );
  SGR_END_COLOR( stderr );
  EPUTS( ": " );
}

bool print_suggestions( dym_kind_t kinds, char const *unknown_token ) {
  did_you_mean_t const *const dym = dym_new( kinds, unknown_token );
  if ( dym != NULL ) {
    EPUTS( "; did you mean " );
    for ( size_t i = 0; dym[i].token != NULL; ++i ) {
      EPRINTF( "%s\"%s\"",
        i == 0 ?
          "" : (dym[i+1].token != NULL ? ", " : (i > 1 ?  ", or " : " or ")),
        dym[i].token
      );
    } // for
    EPUTC( '?' );
    dym_free( dym );
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
