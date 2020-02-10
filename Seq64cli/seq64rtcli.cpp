/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seq64rtcli.cpp
 *
 *  This module declares/defines the main module for the application.
 *
 * \library       seq64rtcli application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-04-07
 * \updates       2020-02-09
 * \license       GNU GPLv2 or above
 *
 *  This application is seq64 without a GUI, control must be done via MIDI.
 */

#include <stdio.h>

#include "platform_macros.h"            /* determine the environment        */

#if defined PLATFORM_LINUX
#include <signal.h>
#include <unistd.h>
#endif

#include "cmdlineopts.hpp"              /* command-line functions           */
#include "daemonize.hpp"                /* seq64::daemonize()               */
#include "gui_assistant.hpp"            /* seq64::gui_assistant base class  */
#include "keys_perform.hpp"             /* seq64::keys_perform              */

#ifdef PLATFORM_LINUX
#include "lash.hpp"                     /* seq64::lash_driver functions     */
#endif

#include "midifile.hpp"                 /* seq64::midifile to open the file */
#include "perform.hpp"                  /* seq64::perform, the main object  */
#include "settings.hpp"                 /* seq64::usr() and seq64::rc()     */

/**
 *  Saves the current state in a MIDI file.  We let midifile tell the perform
 *  that saving worked, so that the "is modified" flag can be cleared.  The
 *  midifile class is already a friend of perform.
 *
 *  Note that we do not support saving files in the Cakewalk WRK format.
 *  Also note that we do not support saving an unnamed tune; there's no good way
 *  to prompt the user for a filename, especially when running as a daemon.
 *  Same for any error message [could log it to an error log at some point]. And
 *  no update to the recent-files settings.
 *
 * \return
 *      Returns true if the save succeeded or if there was no need to save.
 */

static bool
save_file (seq64::perform & p)
{
    bool result = ! p.is_modified();
    if (! result)
        result = seq64::rc().filename().empty();

    if (! result)
    {
        std::string errmsg;
        result = seq64::save_midi_file(p, seq64::rc().filename(), errmsg);
    }
    return result;
}

/**
 *  The standard C/C++ entry point to this application.  This first thing
 *  this function does is scan the argument vector and strip off all
 *  parameters known to GTK+.
 *
 *  The next thing is to set the various settings defaults, and then try to
 *  read the "user" and "rc" configuration files, in that order.  There are
 *  currently no options to change the names of those files.  If we add
 *  that code, we'll move the parsing code to where the configuration
 *  file-names are changed from the command-line.
 *
 *  The last thing is to override any other settings via the command-line
 *  parameters.
 *
 * Daemon support:
 *
 *  Apart from the usual daemon stuff, we need to handle the following issues:
 *
 *      -#  Detecting the need for daemonizing and doing it before all the
 *          normal configuration work is performed.
 *      -#  Loading the initial MIDI file.  Does this filename need to be
 *          grabbed before forking?  No, local variables are passed to the new
 *          process.
 *      -#  Setting the current-working directory.  Should it be grabbed from
 *          the "rc" file?
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns EXIT_SUCCESS (0) or EXIT_FAILURE, depending on the status of
 *      the run.
 */

int
main (int argc, char * argv [])
{
#ifdef PLATFORM_LINUX
    uint32_t usermask = 0;                  /* used only in daemonization   */
#endif
    bool stdio_rerouted = false;            /* used only in log-file option */
    seq64::set_app_name("seq64cli");
    seq64::rc().set_defaults();             /* start out with normal values */
    seq64::usr().set_defaults();            /* start out with normal values */
    if (seq64::parse_o_options(argc, argv))
    {
        std::string logfile = seq64::usr().option_logfile();
        if (seq64::usr().option_use_logfile() && ! logfile.empty())
        {
            (void) seq64::reroute_stdio(logfile);
            stdio_rerouted = true;
        }

#ifdef PLATFORM_LINUX
        if (seq64::usr().option_daemonize())
        {
            printf("Forking to background...\n");
            usermask = seq64::daemonize(seq64::seq_app_name(), ".");
        }
#endif
    }

    /**
     * We currently have a issue where the mastermidibus created by
     * the perform object gets the default PPQN value, because the "user"
     * configuration file has not been read at that point.  See the
     * perform::launch() function.
     */

    seq64::keys_perform keys;                   /* keystroke support        */
    seq64::gui_assistant cli(keys);             /* keystroke support only   */
    seq64::perform p(cli);                      /* main performance object  */
    (void) seq64::parse_command_line_options(p, argc, argv);
    bool is_help = seq64::help_check(argc, argv);
    bool ok = true;
    int optionindex = SEQ64_NULL_OPTION_INDEX;
    if (! stdio_rerouted)                           /* not done already?    */
    {
        std::string logfile = seq64::usr().option_logfile();
        if (seq64::usr().option_use_logfile() && ! logfile.empty())
            (void) seq64::reroute_stdio(logfile);
    }
    if (! is_help)
    {
        std::string extant_errmsg = "unspecified error";
        bool extant_msg_active = false;             /* a kludge             */

        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the mainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        ok = seq64::parse_options_files(p, errmessage, argc, argv);
        optionindex = seq64::parse_command_line_options(p, argc, argv);
        p.launch(seq64::usr().midi_ppqn());         /* set up performance   */
        if (ok)
        {
            if (! seq64::usr().option_daemonize())
            {
                /*
                 * Show information on the busses to help the user diagnose
                 * any configuration issues.  Still has ISSUES!
                 */

                p.print_busses();
            }
            std::string playlistname = seq64::rc().playlist_filespec();
            if (seq64::rc().playlist_active() && ! playlistname.empty())
            {
                ok = p.open_playlist(playlistname, seq64::rc().verbose_option());
                if (ok)
                {
                    ok = p.open_current_song();
                }
                else
                {
                    extant_errmsg = p.playlist_error_message();
                    extant_msg_active = true;
                }
            }
            if (ok && optionindex < argc)           /* MIDI filename given?   */
            {
                int ppqn = 0;
                std::string fn = argv[optionindex];
                ok = open_midi_file(p, fn, ppqn, extant_errmsg);
                if (! ok)
                    extant_msg_active = true;
            }
            if (ok)
            {
#if defined PLATFORM_LINUX
                if (seq64::rc().lash_support())
                    seq64::create_lash_driver(p, argc, argv);

                seq64::session_setup();
#endif
                while (! seq64::session_close())
                {
                    if (seq64::session_save())
                        save_file(p);

                    usleep(1000000);
                }
                p.finish();                         /* tear down performer  */
                if (seq64::rc().auto_option_save())
                {
                    if (ok)                         /* don't write bad data */
                        ok = seq64::write_options_files(p);
                }
                else
                    printf("[auto-option-save off, not saving config files]\n");

#ifdef PLATFORM_LINUX
                if (seq64::rc().lash_support())
                    seq64::delete_lash_driver();    /* deleted only exists  */
#endif
            }
        }
        else
        {
            if (extant_msg_active)
                errprint(extant_errmsg.c_str());
            else
                errprint(errmessage.c_str());

            (void) seq64::write_options_files(p, "erroneous");
        }

#ifdef PLATFORM_LINUX
        if (seq64::usr().option_daemonize())
            seq64::undaemonize(usermask);
#endif
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * seq64rtcli.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

