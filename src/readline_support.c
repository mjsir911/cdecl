/*
**      cdecl -- C gibberish translator
**      src/readline_support.c
*/

// local
#include "config.h"                     /* must go first */
#include "literals.h"
#include "util.h"

// standard
#include <assert.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

// subset of cdecl keywords that are commands
static char const *const CDECL_COMMANDS[] = {
  L_CAST,
  L_DECLARE,
  L_EXIT,
  L_EXPLAIN,
  L_HELP,
  L_QUIT,
  L_SET,
  NULL
};

// subset of cdecl keywords that are completable
static char const *const CDECL_KEYWORDS[] = {
  L_ARRAY,
//L_AS,                                 // too short
  L_AUTO,
  L_BLOCK,                              // Apple: English for '^'
  L___BLOCK,                            // Apple: storage class
  L_BOOL,
  L_CHAR,
  L_CHAR16_T,
  L_CHAR32_T,
  L_CLASS,
  L_COMPLEX,
  L_CONST,
  L_DOUBLE,
  L_ENUM,
  L_EXTERN,
  L_FLOAT,
  L_FUNCTION,
//L_INT,                                // special case (see below)
//L_INTO,                               // special case (see below)
  L_LONG,
  L_MEMBER,
  L_NORETURN,
//L_OF,                                 // too short
  L_POINTER,
  L_REFERENCE,
  L_REGISTER,
  L_RESTRICT,
  L_RETURNING,
  L_SHORT,
  L_SIGNED,
  L_STATIC,
  L_STRUCT,
//L_TO,                                 // too short
  L_THREAD_LOCAL,
  L_TYPEDEF,
  L_UNION,
  L_UNSIGNED,
  L_VOID,
  L_VOLATILE,
  L_WCHAR_T,
  NULL
};

// cdecl options
static char const *const CDECL_OPTIONS[] = {
  "ansi",
  "c89",
  "c95",
  "c99",
  "c11",
  "c++",
  "c++11",
  "create",
"nocreate",
  "knr"
  "options",
  "preansi",                            // synonym for "knr"
  "prompt",
"noprompt",
  NULL
};

// local functions
static char*  command_generator( char const*, int );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether the current line being read is a particular cdecl command.
 *
 * @param command The command to check for.
 * @return Returns \c true only if it is.
 */
static inline bool is_command( char const *command ) {
  return strncmp( rl_line_buffer, command, strlen( command ) ) == 0;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * TODO
 */
static char** attempt_completion( char const *text, int start, int end ) {
  assert( text );
  (void)end;
  //
  // If the word is at the start of the line (start == 0), then attempt to
  // complete only cdecl commands and not all keywords.
  //
  return start == 0 ? rl_completion_matches( text, command_generator ) : NULL;
}

/**
 * Attempts to match a cdecl command.
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 */
static char* command_generator( char const *text, int state ) {
  static unsigned index;
  static size_t text_len;

  if ( !state ) {                       // new word? reset
    index = 0;
    text_len = strlen( text );
  }

  for ( char const *command; (command = CDECL_COMMANDS[ index ]); ) {
    ++index;
    if ( strncmp( text, command, text_len ) == 0 )
      return check_strdup( command );
  } // for
  return NULL;
}

/**
 * TODO
 */
static char* keyword_completion( char const *text, int state ) {
  static unsigned index;
  static size_t text_len;

  static bool is_into;
  static bool is_set_command;

  if ( !state ) {                       // new word? reset
    index = 0;
    text_len = strlen( text );
    is_into = false;
    //
    // If we're doing the "set" command, complete options, not keywords.
    //
    is_set_command = is_command( L_SET );
  }

  if ( is_set_command ) {
    for ( char const *option; (option = CDECL_OPTIONS[ index ]); ) {
      ++index;
      if ( strncmp( text, option, text_len ) == 0 )
        return check_strdup( option );
    } // for
    return NULL;
  }

  // handle "int" and "into" as special cases
  if ( !is_into ) {
    is_into = true;
    if ( strncmp( text, L_INTO, text_len ) == 0 &&
         strncmp( text, L_INT, text_len ) != 0 ) {
      return check_strdup( L_INTO );
    }
    if ( strncmp( text, L_INT, text_len ) != 0 )
      return keyword_completion( text, is_into );

    /* normally "int" and "into" would conflict with one another when
      * completing; cdecl tries to guess which one you wanted, and it
      * always guesses correctly
      */
    return check_strdup(
      is_command( L_CAST ) && !strstr( rl_line_buffer, L_INTO ) ?
        L_INTO : L_INT
    );
  }

  for ( char const *keyword; (keyword = CDECL_KEYWORDS[ index ]); ) {
    ++index;
    if ( strncmp( text, keyword, text_len ) == 0 )
      return check_strdup( keyword );
  } // for
  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

void readline_init( void ) {
  rl_readline_name = (char*)PACKAGE;  // allow conditional .inputrc parsing
  rl_attempted_completion_function = attempt_completion;
  rl_completion_entry_function = (Function*)keyword_completion;
  rl_bind_key( '\t', rl_complete );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
