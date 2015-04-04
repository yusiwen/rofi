/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball  Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include "rofi.h"
#include "dialogs/dmenu.h"
#include "helper.h"


static char **get_dmenu ( int *length )
{
    char buffer[1024];
    char **retv = NULL;

    *length = 0;

    while ( fgets_s ( buffer, 1024, stdin, (char) config.separator ) != NULL ) {
        retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
        retv[( *length )]     = g_strdup ( buffer );
        retv[( *length ) + 1] = NULL;

        // Filter out line-end.
        if ( retv[( *length )][strlen ( buffer ) - 1] == '\n' ) {
            retv[( *length )][strlen ( buffer ) - 1] = '\0';
        }

        ( *length )++;
        // Stop when we hit 2³¹ entries.
        if ( ( *length ) == INT_MAX ) {
            return retv;
        }
    }

    return retv;
}


static unsigned int row_urgent = 0xFFFFFFFF;
static unsigned int row_active = 0xFFFFFFFF;

static const char *get_display_data ( unsigned int index, void *data, G_GNUC_UNUSED int *state )
{
    char **retv = (char * *) data;
    if ( index == row_urgent ) {
        *state |= URGENT;
    }
    if ( index == row_active ) {
        *state |= ACTIVE;
    }
    return retv[index];
}

int dmenu_switcher_dialog ( char **input )
{
    char *dmenu_prompt = "dmenu ";
    int  selected_line = 0;
    int  retv          = FALSE;
    int  length        = 0;
    char **list        = get_dmenu ( &length );
    int  restart       = FALSE;

    int  number_mode = FALSE;
    // Check if the user requested number mode.
    if ( find_arg (  "-i" ) >= 0 ) {
        number_mode = TRUE;
    }
    // Check prompt
    find_arg_str (  "-p", &dmenu_prompt );
    find_arg_int (  "-l", &selected_line );
    find_arg_uint (  "-u", &row_urgent );
    find_arg_uint (  "-a", &row_active );

    do {
        int mretv = menu ( list, length, input, dmenu_prompt,
                           token_match, NULL, &selected_line, FALSE, get_display_data, list );

        // We normally do not want to restart the loop.
        restart = FALSE;
        if ( ( mretv & MENU_OK ) && list[selected_line] != NULL ) {
            if ( number_mode ) {
                fprintf ( stdout, "%d", selected_line );
            }
            else {
                fputs ( list[selected_line], stdout );
            }
            fputc ( '\n', stdout );
            fflush ( stdout );
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( selected_line + 1, length - 1 );
            }
            retv = TRUE;
        }
        else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
            if ( !number_mode ) {
                fputs ( *input, stdout );
                fputc ( '\n', stdout );
                fflush ( stdout );
            }
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( selected_line + 1, length - 1 );
            }
            retv = TRUE;
        }
    } while ( restart );

    g_strfreev ( list );

    return retv;
}

