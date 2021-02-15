/*
**      cdecl -- C gibberish translator
**      src/prompt.c
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
 * Defines global variables and functions for the prompt.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "prompt.h"
#include "c_lang.h"
#include "cdecl.h"
#include "color.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if WITH_READLINE
#include <readline/readline.h>          /* for rl_gnu_readline_p */

#if !HAVE_DECL_RL_PROMPT_START_IGNORE
# define RL_PROMPT_START_IGNORE   '\1'
# define RL_PROMPT_END_IGNORE     '\2'
#endif /* !HAVE_DECL_RL_PROMPT_START_IGNORE */
#endif /* WITH_READLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
char const         *cdecl_prompt[2];

// local variable definitions
static char        *prompt_buf[2];      ///< Buffers for prompts.

////////// inline functions ///////////////////////////////////////////////////

#ifdef WITH_READLINE
/**
 * Checks to see whether we're running genuine GNU readline and not some other
 * library emulating it.
 *
 * Some readline emulators, e.g., editline, have a bug that makes color prompts
 * not work correctly.  So, unless we know we're using genuine GNU readline,
 * use this function to disable color prompts.
 *
 * @return Returns `true` only if we're running genuine GNU readline.
 *
 * @sa [The GNU Readline Library](https://tiswww.case.edu/php/chet/readline/rltop.html)
 * @sa http://stackoverflow.com/a/31333315/99089
 */
PJL_WARN_UNUSED_RESULT
static inline bool have_genuine_gnu_readline( void ) {
#if HAVE_DECL_RL_GNU_READLINE_P
  return rl_gnu_readline_p == 1;
#else
  return false;
#endif /* HAVE_DECL_RL_GNU_READLINE_P */
}
#endif /* WITH_READLINE */

////////// local functions ////////////////////////////////////////////////////

/**
 * Creates a prompt.
 *
 * @param suffix The prompt suffix character to use.
 * @return Returns a prompt string.  The caller is responsible for freeing the
 * memory.
 */
PJL_WARN_UNUSED_RESULT
static char* prompt_create( char suffix ) {
  size_t prompt_len = strlen( CPPDECL "> " );

#ifdef WITH_READLINE
  if ( have_genuine_gnu_readline() && sgr_prompt != NULL ) {
    prompt_len +=
      1 /* RL_PROMPT_START_IGNORE */ +
      (sizeof( SGR_START SGR_EL ) - 1/*null*/) +
      (strlen( sgr_prompt )) +
      2 /* RL_PROMPT_END_IGNORE */ +
      (sizeof( SGR_END SGR_EL ) - 1/*null*/);
  }
#endif /* WITH_READLINE */

  char *const buf = MALLOC( char, prompt_len + 1/*null*/ );
  char *p = buf;

#ifdef WITH_READLINE
  char color_buf[20];

  if ( have_genuine_gnu_readline() && sgr_prompt != NULL ) {
    *p++ = RL_PROMPT_START_IGNORE;
    SGR_SSTART_COLOR( color_buf, prompt );
    p = strcpy_end( p, color_buf );
    *p++ = RL_PROMPT_END_IGNORE;
  }
#endif /* WITH_READLINE */

  p = strcpy_end( p, opt_lang_is_cpp() ? CPPDECL : PACKAGE );
  *p++ = suffix;

#ifdef WITH_READLINE
  if ( have_genuine_gnu_readline() && sgr_prompt != NULL ) {
    *p++ = RL_PROMPT_START_IGNORE;
    SGR_SEND_COLOR( color_buf );
    p = strcpy_end( p, color_buf );
    *p++ = RL_PROMPT_END_IGNORE;
  }
#endif /* WITH_READLINE */

  *p++ = ' ';
  *p = '\0';

  return buf;
}

////////// extern functions ///////////////////////////////////////////////////

void cdecl_prompt_enable( bool enable ) {
  if ( enable ) {
    cdecl_prompt[0] = prompt_buf[0];
    cdecl_prompt[1] = prompt_buf[1];
  } else {
    cdecl_prompt[0] = cdecl_prompt[1] = "";
  }
}

void cdecl_prompt_init( void ) {
  free( prompt_buf[0] );
  free( prompt_buf[1] );

  cdecl_prompt[0] = prompt_buf[0] = prompt_create( '>' );
  cdecl_prompt[1] = prompt_buf[1] = prompt_create( '+' );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
