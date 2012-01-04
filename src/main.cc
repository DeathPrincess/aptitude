// main.cc  (neé testscr.cc)
//
//  Copyright 1999-2011 Daniel Burrows
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.
//
//  Tests the various screen-output mechanisms

#include <signal.h>

#include "aptitude.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>

#include <generic/problemresolver/exceptions.h>

#include <generic/util/logging.h>
#include <generic/util/temp.h>
#include <generic/util/util.h>

#ifdef HAVE_GTK
#include <gtkmm.h>
#endif

#ifdef HAVE_QT
#include <QtCore/qglobal.h> // To get the Qt version number
#endif

#include <cwidget/config/keybindings.h>
#include <cwidget/generic/util/transcode.h>
#include <cwidget/toplevel.h>
#include <cwidget/dialogs.h>

#include <cmdline/cmdline_changelog.h>
#include <cmdline/cmdline_check_resolver.h>
#include <cmdline/cmdline_clean.h>
#include <cmdline/cmdline_common.h>
#include <cmdline/cmdline_do_action.h>
#include <cmdline/cmdline_download.h>
#include <cmdline/cmdline_dump_resolver.h>
#include <cmdline/cmdline_extract_cache_subset.h>
#include <cmdline/cmdline_forget_new.h>
#include <cmdline/cmdline_moo.h>
#include <cmdline/cmdline_prompt.h>
#include <cmdline/cmdline_search.h>
#include <cmdline/cmdline_show.h>
#include <cmdline/cmdline_update.h>
#include <cmdline/cmdline_user_tag.h>
#include <cmdline/cmdline_versions.h>
#include <cmdline/cmdline_why.h>
#include <cmdline/terminal.h>

#include <sigc++/functors/ptr_fun.h>

#include <apt-pkg/error.h>
#include <apt-pkg/cmndline.h>
#include <apt-pkg/init.h>

#include <boost/format.hpp>
#include <boost/optional.hpp>

#ifdef HAVE_GTK
#include "gtk/gui.h"
#include "gtk/init.h"
#endif

#ifdef HAVE_QT
#include "qt/qt_main.h"
#endif

#include <fstream>

#include "loggers.h"
#include "progress.h"
#include "pkg_columnizer.h"
#include "pkg_grouppolicy.h"
#include "pkg_view.h"
#include "ui.h"

namespace cw = cwidget;

using aptitude::Loggers;

using boost::optional;

using logging::DEBUG_LEVEL;
using logging::ERROR_LEVEL;
using logging::FATAL_LEVEL;
using logging::INFO_LEVEL;
using logging::OFF_LEVEL;
using logging::WARN_LEVEL;
using logging::TRACE_LEVEL;

using logging::Logger;
using logging::LoggerPtr;
using logging::log_level;

#if 0
// These are commented out so as to not punish users unduly for coding
// errors.  The cw::util::transcoder now substitutes conspicuous '?' characters
// into its output, which should be enough of a clue.


/** Handles a coding error using the apt error mechanism. */
std::wstring handle_mbtow_error(int error,
				const std::wstring &partial,
				const std::string &input)
{
  _error->Errno("iconv", _("Can't decode multibyte string after \"%ls\""),
		partial.c_str());
  return partial;
}

std::string handle_wtomb_error(int error,
			       const std::string &partial,
			       const std::wstring &input)
{
  _error->Errno("iconv", _("Can't decode wide-character string after \"%s\""),
		partial.c_str());
  return partial;
}
#endif

static void show_version()
{
  printf(_("%s %s compiled at %s %s\n"),
	   PACKAGE, VERSION, __DATE__, __TIME__);
#ifdef __GNUC__
  printf(_("Compiler: g++ %s\n"), __VERSION__);
#endif
  printf("%s", _("Compiled against:\n"));
  printf(_("  apt version %d.%d.%d\n"),
	 APT_PKG_MAJOR, APT_PKG_MINOR, APT_PKG_RELEASE);
#ifndef NCURSES_VERSION
  printf(_("  NCurses version: Unknown\n"));
#else
  printf(_("  NCurses version %s\n"), NCURSES_VERSION);
#endif
  printf(_("  libsigc++ version: %s\n"), SIGC_VERSION);
#ifdef HAVE_EPT
  printf(_("  Ept support enabled.\n"));
#else
  printf(_("  Ept support disabled.\n"));
#endif
#ifdef HAVE_GTK
  printf(_("  Gtk+ version %d.%d.%d\n"),
	 GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
  printf(_("  Gtk-- version %d.%d.%d\n"),
	 GTKMM_MAJOR_VERSION, GTKMM_MINOR_VERSION, GTKMM_MICRO_VERSION);
#else
  printf(_("  Gtk+ support disabled.\n"));
#endif
#ifdef HAVE_QT
  printf(_("  Compiled with Qt 4.6.2 %s\n"), QT_VERSION_STR);
  printf(_("  Running on Qt 4.6.2 %s\n"), qVersion());
#else
  printf(_("  Qt support disabled.\n"));
#endif
  printf("%s", _("\nCurrent library versions:\n"));
  printf(_("  NCurses version: %s\n"), curses_version());
  printf(_("  cwidget version: %s\n"), cwidget::version().c_str());
  printf(_("  Apt version: %s\n"), pkgLibVersion);
}

static void usage()
{
  printf(PACKAGE " " VERSION "\n");
  printf(_("Usage: aptitude [-S fname] [-u|-i]"));
  printf("\n");
  printf(_("       aptitude [options] <action> ..."));
  printf("\n");
  printf(_("  Actions (if none is specified, aptitude will enter interactive mode):\n\n"));
  printf(_(" install      - Install/upgrade packages.\n"));
  printf(_(" remove       - Remove packages.\n"));
  printf(_(" purge        - Remove packages and their configuration files.\n"));
  printf(_(" hold         - Place packages on hold.\n"));
  printf(_(" unhold       - Cancel a hold command for a package.\n"));
  printf(_(" markauto     - Mark packages as having been automatically installed.\n"));
  printf(_(" unmarkauto   - Mark packages as having been manually installed.\n"));
  printf(_(" forbid-version - Forbid aptitude from upgrading to a specific package version.\n"));
  printf(_(" update       - Download lists of new/upgradable packages.\n"));
  printf(_(" safe-upgrade - Perform a safe upgrade.\n"));
  printf(_(" full-upgrade - Perform an upgrade, possibly installing and removing packages.\n"));
  printf(_(" build-dep    - Install the build-dependencies of packages.\n"));
  printf(_(" forget-new   - Forget what packages are \"new\".\n"));
  printf(_(" search       - Search for a package by name and/or expression.\n"));
  printf(_(" show         - Display detailed information about a package.\n"));
  printf(_(" versions     - Displays the versions of specified packages.\n"));
  printf(_(" clean        - Erase downloaded package files.\n"));
  printf(_(" autoclean    - Erase old downloaded package files.\n"));
  printf(_(" changelog    - View a package's changelog.\n"));
  printf(_(" download     - Download the .deb file for a package.\n"));
  printf(_(" reinstall    - Download and (possibly) reinstall a currently installed package.\n"));
  printf(_(" why          - Show the manually installed packages that require a package, or\n"
           "                why one or more packages would require the given package.\n"));
  printf(_(" why-not      - Show the manually installed packages that lead to a conflict\n"
           "                with the given package, or why one or more packages would\n"
           "                lead to a conflict with the given package if installed.\n"));
  printf("\n");
  printf(_("  Options:\n"));
  printf(_(" -h             This help text.\n"));
#ifdef HAVE_GTK
  printf(_(" --gui          Use the GTK GUI even if disabled in the configuration.\n"));
#endif
  printf(_(" --no-gui       Do not use the GTK GUI even if available.\n"));
#ifdef HAVE_QT
  printf(_(" --qt           Use the Qt GUI.\n"));
  printf(_(" --no-qt        Do not use the Qt GUI even if enabled in the configuration.\n"));
#endif
  printf(_(" -s             Simulate actions, but do not actually perform them.\n"));
  printf(_(" -d             Only download packages, do not install or remove anything.\n"));
  printf(_(" -P             Always prompt for confirmation or actions.\n"));
  printf(_(" -y             Assume that the answer to simple yes/no questions is 'yes'.\n"));
  printf(_(" -F format      Specify a format for displaying search results; see the manual.\n"));
  printf(_(" -O order       Specify how search results should be sorted; see the manual.\n"));
  printf(_(" -w width       Specify the display width for formatting search results.\n"));
  printf(_(" -f             Aggressively try to fix broken packages.\n"));
  printf(_(" -V             Show which versions of packages are to be installed.\n"));
  printf(_(" -D             Show the dependencies of automatically changed packages.\n"));
  printf(_(" -Z             Show the change in installed size of each package.\n"));
  printf(_(" -v             Display extra information. (may be supplied multiple times).\n"));
  printf(_(" -t [release]   Set the release from which packages should be installed.\n"));
  printf(_(" -q             In command-line mode, suppress the incremental progress.\n"
           "                indicators.\n"));
  printf(_(" -c file        Read this configuration file.\n"));
  printf(_(" -o key=val     Directly set the configuration option named 'key'.\n"));
  printf(_(" --with(out)-recommends	Specify whether or not to treat recommends as.\n"
           "                strong dependencies.\n"));
  printf(_(" -S fname       Read the aptitude extended status info from fname.\n"));
  printf(_(" -u             Download new package lists on startup.\n"));
  printf(_("                  (terminal interface only)\n"));
  printf(_(" -i             Perform an install run on startup.\n"));
  printf(_("                  (terminal interface only)\n"));
  printf("\n");
  printf(_("                  This aptitude does not have Super Cow Powers.\n"));
}

static bool cmdline_help(CommandLine &cmdl)
{
  usage();
  return true;
}

static bool cmdline_nop(CommandLine &cmdl)
{
  const bool do_selections = strcasecmp(cmdl.FileList[0], "nop") == 0;
  OpTextProgress p(aptcfg->FindI("quiet", 0));
  _error->DumpErrors();
  apt_init(&p, do_selections);
  return true;
}

CommandLine::Args args[] = {
  {'h', "help", "help", 0},
  {0, "version", "version", 0},
  {'q', "quiet", "quiet", CommandLine::IntLevel},
  {'s', "simulate", PACKAGE "::Simulate", 0},
  {'r', "with-recommends", "APT:Install-Recommends", 0},
  {'R', "without-recommends", "without-recommends", 0},
  {'t', "target-release", "APT::Default-Release", CommandLine::HasArg},
  {'t', "default-release", "APT::Default-Release", CommandLine::HasArg},
  {'S', "", "status-fname", CommandLine::HasArg},
  {0, "purge-unused", PACKAGE "::Purge-Unused", 0},
  {0, "log-level", PACKAGE "::Logging::Levels::", CommandLine::HasArg},
  {0, "log-file", PACKAGE "::Logging::File", CommandLine::HasArg},
  {0, "log-resolver", "log-resolver", 0},
  {'F', "display-format", "display-format", CommandLine::HasArg},
  {'w', "width", PACKAGE "::CmdLine::Package-Display-Width", CommandLine::HasArg},
  {0, "allow-untrusted", PACKAGE "::CmdLine::Ignore-Trust-Violations", 0},
  {'d', "download-only", PACKAGE "::CmdLine::Download-Only", 0},
  {'y', "assume-yes", PACKAGE "::CmdLine::Assume-Yes", 0},
  {'v', "verbose", PACKAGE "::CmdLine::Verbose", CommandLine::IntLevel},
  {'V', "show-versions", PACKAGE "::CmdLine::Show-Versions", 0},
  {'D', "show-deps", PACKAGE "::CmdLine::Show-Deps", 0},
  {'W', "show-why", PACKAGE "::CmdLine::Show-Why", 0},
  {'P', "prompt", PACKAGE "::CmdLine::Always-Prompt", 0},
  {'O', "sort", PACKAGE "::CmdLine::Package-Sorting", CommandLine::HasArg},
  {0, "disable-columns", PACKAGE "::CmdLine::Disable-Columns", 0},
  {0, "no-new-installs", PACKAGE "::Safe-Resolver::No-New-Installs", 0},
  {0, "no-new-upgrades", PACKAGE "::Safe-Resolver::No-New-Upgrades", 0},
  {0, "allow-new-installs", PACKAGE "::Safe-Resolver::No-New-Installs", CommandLine::InvBoolean},
  {0, "allow-new-upgrades", PACKAGE "::Safe-Resolver::No-New-Upgrades", CommandLine::InvBoolean},
  {0, "safe-resolver", "use-full-resolver", CommandLine::InvBoolean},
  {0, "full-resolver", "use-full-resolver", CommandLine::Boolean},
  {0, "show-resolver-actions", PACKAGE "::Safe-Resolver::Show-Resolver-Actions", 0},
  {0, "visual-preview", PACKAGE "::CmdLine::Visual-Preview", 0},
  {0, "schedule-only", PACKAGE "::CmdLine::Schedule-Only", 0},
  // TODO: add-user-tag, add-user-tag-to, remove-user-tag, remove-user-tag-from
  {0, "arch-only", "APT::Get::Arch-Only", CommandLine::Boolean},
  {0, "not-arch-only", "APT::Get::Arch-Only", CommandLine::InvBoolean},
  {0, "show-summary", PACKAGE "::CmdLine::Show-Summary",  CommandLine::HasArg},
  {0, "group-by", PACKAGE "::CmdLine::Versions-Group-By", CommandLine::HasArg},
  {0, "show-package-names", PACKAGE "::CmdLine::Versions-Show-Package-Names", CommandLine::HasArg},
  {'f', "fix-broken", PACKAGE "::CmdLine::Fix-Broken", 0},
  {'Z', "show-size-changes", PACKAGE "::CmdLine::Show-Size-Changes", 0},
#ifdef HAVE_GTK
  {0, "gui", PACKAGE "::Start-Gui", CommandLine::Boolean},
  {0, "new-gui", "new-gui", 0},
#endif
  {0, "no-gui", PACKAGE "::Start-Gui", CommandLine::InvBoolean},
#ifdef HAVE_QT
  {0, "qt", "use-qt-gui", 0},
#endif
  {0, "autoclean-on-startup", PACKAGE "::UI::Autoclean-On-Startup", 0},
  {0, "clean-on-startup", PACKAGE "::UI::Clean-On-Startup", 0},
  {'u', "update-on-startup", PACKAGE "::UI::Update-On-Startup", 0},
  {'i', "install-on-startup", PACKAGE "::UI::Install-On-Startup", 0},
  {'c', "config-file", 0, CommandLine::ConfigFile},
  {'o', "option", 0, CommandLine::ArbItem},
  {0,0,0,0}
};

CommandLine::Dispatch cmds[] = {
  {"update", &cmdline_update},
  {"clean", &cmdline_clean},
  {"autoclean", &cmdline_autoclean},
  {"forget-new", &cmdline_forget_new},
  {"search", &cmdline_search},
  {"versions", &cmdline_versions},
  {"why", &cmdline_why},
  {"why-not", &cmdline_why},
  {"install", &cmdline_do_action},
  {"reinstall", &cmdline_do_action},
  {"dist-upgrade", &cmdline_do_action},
  {"full-upgrade", &cmdline_do_action},
  {"safe-upgrade", &cmdline_do_action},
  {"upgrade", &cmdline_do_action},
  {"remove", &cmdline_do_action},
  {"purge", &cmdline_do_action},
  {"hold", &cmdline_do_action},
  {"unhold", &cmdline_do_action},
  {"markauto", &cmdline_do_action},
  {"unmarkauto", &cmdline_do_action},
  {"forbid-version", &cmdline_do_action},
  {"keep", &cmdline_do_action},
  {"keep-all", &cmdline_do_action},
  {"build-dep", &cmdline_do_action},
  {"build-depends", &cmdline_do_action},
  {"add-user-tag", &aptitude::cmdline::cmdline_user_tag},
  {"remove-user-tag", &aptitude::cmdline::cmdline_user_tag},
  {"extract-cache-subset", &aptitude::cmdline::extract_cache_subset},
  {"download", &cmdline_download},
  {"changelog", &cmdline_changelog},
  {"moo", &cmdline_moo},
  {"show", &cmdline_show},
  {"dump-resolver", &cmdline_dump_resolver},
  {"check-resolver", &cmdline_check_resolver},
  {"help", &cmdline_help},
  {"nop", &cmdline_nop},
  {"nop-noselections", &cmdline_nop},
  {0,0}
};

const char *argv0;

namespace
{
  bool strncase_eq_with_translation(const std::string &s1, const char *s2)
  {
    if(strcasecmp(s1.c_str(), s2) == 0)
      return true;
    else if(strcasecmp(s1.c_str(), _(s2)) == 0)
      return true;
    else
      return false;
  }

  class log_level_map
  {
    std::map<std::string, log_level> levels;

    void add_level(const std::string &s, log_level level)
    {
      std::string tmp;
      for(std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	tmp.push_back(toupper(*it));

      levels[tmp] = level;
    }

  public:
    log_level_map()
    {
      using namespace logging;

      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("trace"), TRACE_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("debug"), DEBUG_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("info"), INFO_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("warn"), WARN_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("error"), ERROR_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("fatal"), FATAL_LEVEL);
      // ForTranslators: This is a log level that the user can pass on
      // the command-line or set in the configuration file.
      add_level(N_("off"), OFF_LEVEL);

      std::vector<std::pair<std::string, log_level> >
	tmp(levels.begin(), levels.end());

      for(std::vector<std::pair<std::string, log_level> >::const_iterator
	    it = tmp.begin(); it != tmp.end(); ++it)
	{
	  // Make sure that the untranslated entries always override
	  // the translated ones so that instructions in English
	  // always work.
	  std::map<std::string, log_level>::const_iterator found =
	    levels.find(it->first);

	  if(found == levels.end())
	    add_level(_(it->first.c_str()), it->second);
	}
    }

    typedef std::map<std::string, log_level>::const_iterator const_iterator;

    const_iterator find(const std::string &s) const
    {
      std::string tmp;
      for(std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	tmp.push_back(toupper(*it));

      return levels.find(tmp);
    }

    const_iterator end() const
    {
      return levels.end();
    }
  };

  log_level_map log_levels;

  /** \brief Parse a logging level.
   *
   *  Logging levels have the form [<logger>:]level, where
   *  <logger> is an optional logger name.
   */
  void apply_logging_level(const std::string &s)
  {
    std::string::size_type colon_loc = s.rfind(':');
    std::string level_name;
    std::string logger_name;

    if(colon_loc == std::string::npos)
      level_name = s;
    else
      {
	level_name = std::string(s, colon_loc + 1);
	logger_name = std::string(s, 0, colon_loc);
      }

    optional<log_level> level;

    log_level_map::const_iterator found =
      log_levels.find(level_name);
    if(found != log_levels.end())
      level = found->second;

    if(!level)
      {
	// ForTranslators: both the translated and the untranslated
	// log level names are accepted here.
	_error->Error(_("Unknown log level name \"%s\" (expected \"trace\", \"debug\", \"info\", \"warn\", \"error\", \"fatal\", or \"off\")."),
		      level_name.c_str());
	return;
      }

    LoggerPtr targetLogger = Logger::getLogger(logger_name);

    if(!targetLogger)
      {
	_error->Error(_("Invalid logger name \"%s\"."),
		      logger_name.c_str());
	return;
      }

    targetLogger->setLevel(*level);
  }

  /** \brief Apply logging levels from the configuration file. */
  void apply_config_file_logging_levels(Configuration *config)
  {
    const Configuration::Item *tree = config->Tree(PACKAGE "::Logging::Levels");
    if(tree == NULL)
      return;

    for(Configuration::Item *item = tree->Child; item != NULL;
	item = item->Next)
      apply_logging_level(item->Value);
  }

  /** \brief Set some standard logging levels to output log
   *  information about the resolver.
   *
   *  The information this generates is enough to let the log parser
   *  show a visualization of what happened.
   */
  void enable_resolver_log()
  {
    Loggers::getAptitudeResolverSearch()->setLevel(TRACE_LEVEL);
    Loggers::getAptitudeResolverSearchCosts()->setLevel(INFO_LEVEL);
  }
}

// Ensure that the cache is always closed when main() exits.  Without
// this, there might be dangling flyweights hanging around, and those
// can trigger aborts when the static flyweight pool is destroyed.
//
// TBH, I think it might be worth writing our own flyweight stand-in
// to avoid this particular bit of stupid.  On the other hand, it
// might be better to fully shut down the cache all the time, to
// better detect leaks and so on?  I'm undecided -- and it shouldn't
// take too long to clear out the cache.
struct close_cache_on_exit
{
  close_cache_on_exit()
  {
  }

  ~close_cache_on_exit()
  {
    apt_shutdown();
  }
};

void do_message_logged(std::ostream &out,
                       const char *sourceFilename,
                       int sourceLineNumber,
                       log_level level,
                       LoggerPtr logger,
                       const std::string &msg)
{
  time_t current_time = 0;
  struct tm local_current_time;

  time(&current_time);
  localtime_r(&current_time, &local_current_time);

  out << sstrftime("%F %T", &local_current_time)
      << " [" << pthread_self() << "] "
      << sourceFilename << ":" << sourceLineNumber
      << " " << describe_log_level(level)
      << " " << logger->getCategory()
      << " - " << msg << std::endl << std::flush;
}

void handle_message_logged(const char *sourceFilename,
                           int sourceLineNumber,
                           log_level level,
                           LoggerPtr logger,
                           const std::string &msg,
                           const std::string &filename)
{
  if(filename == "-")
    do_message_logged(std::cout,
                      sourceFilename,
                      sourceLineNumber,
                      level,
                      logger,
                      msg);
  else
    {
      std::ofstream f(filename.c_str(), std::ios::app);
      if(f)
        do_message_logged(f,
                          sourceFilename,
                          sourceLineNumber,
                          level,
                          logger,
                          msg);
      // Since logging is just for debugging, I don't do anything if
      // the log file can't be opened.
    }
}

void dump_errors ()
{
  _error->DumpErrors();
}

int main(int argc, const char *argv[])
{
  // Block signals that we want to sigwait() on by default and put the
  // signal mask into a known state.  This ensures that unless threads
  // deliberately ask for a signal, they don't get it, meaning that
  // sigwait() should work as expected.  (the alternative, blocking
  // all signals, is troublesome: we would have to ensure that fatal
  // signals and other things that shouldn't be blocked get removed)
  //
  // When aptitude used log4cxx, we ran into the fact that it doesn't
  // ensure that its threads don't block signals, so cwidget wasn't
  // able to sigwait() on SIGWINCH without this.  (cwidget is guilty
  // of the same thing, but that doesn't cause problems for aptitude)
  {
    sigset_t mask;

    sigemptyset(&mask);

    sigaddset(&mask, SIGWINCH);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);
  }

  srandom(time(0));

  // See earlier note
  //
  //cw::util::transcode_mbtow_err=handle_mbtow_error;
  //cw::util::transcode_wtomb_err=handle_wtomb_error;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  // An environment variable is used mostly because of the utterly
  // lame option parsing that aptitude does.  A better option parser
  // would pre-parse the options and remember them in a structure of
  // some sort, meaning that I could parse the command-line options
  // before I set up the apt configuration structures.
  const char * const rootdir = getenv("APT_ROOT_DIR");
  apt_preinit(rootdir);

  close_cache_on_exit close_on_exit;

  // NOTE: this can of course be spoofed.  Anyone bothering to is off their
  //      rocker.
  argv0=argv[0];

  // Backwards bug-compatibility; old versions used a misleading name
  // for this option.
  if(aptcfg->Find(PACKAGE "::Keep-Unused-Pattern", "") == "")
    {
      aptcfg->Set(PACKAGE "::Keep-Unused-Pattern",
		  aptcfg->Find(PACKAGE "::Delete-Unused-Pattern", ""));
      aptcfg->Set(PACKAGE "::Delete-Unused-Pattern", "");
    }
  else
    aptcfg->Set(PACKAGE "::Delete-Unused-Pattern", "");

  // By default don't log anything below WARN.
  Logger::getLogger("")->setLevel(WARN_LEVEL);

  // HACK: Support for optional values in CommandLine.
  for(int i = 1; i != argc; i++)
    {
      if(strcasecmp("--show-summary", argv[i]) == 0)
        argv[i] = "--show-summary=first-package";
    }

  // Parse the command line options.
  CommandLine cmdl(args, _config);
  if(cmdl.Parse(argc, argv) == false)
    {
      _error->DumpErrors(std::cerr, GlobalError::WARNING, false);
      usage();
      exit(1);
    }

  if(aptcfg->FindB("help", false) == true)
    {
      usage();
      exit(0);
    }

  if(aptcfg->FindB("version", false) == true)
    {
      show_version();
      exit(0);
    }

  // HACK: Handle arguments not directly supported by CommandLine.
  {
    string display_format = _config->Find("display-format", "");
    if(display_format.empty() == false)
      {
        _config->Set(PACKAGE "::CmdLine::Package-Display-Format", display_format);
        _config->Set(PACKAGE "::CmdLine::Version-Display-Format", display_format);
        _config->Clear("display-format");
      }

    if(_config->FindB("without-recommends", false) == true)
      {
        _config->Set("APT::Install-Recommends", false);
        _config->Set("APT::AutoRemove::Recommends-Important", true);
        _config->Clear("without-recommends");
      }

    if(_config->FindB("log-resolver", false) == true)
      {
        _config->Set(PACKAGE "::Logging::Levels::", "aptitude.resolver.search:trace");
        _config->Set(PACKAGE "::Logging::Levels::", "aptitude.resolver.search.costs:info");
        _config->Clear("log-resolver");
      }
  }

  // The filename to read status information from.
  char *status_fname=NULL;
  if(aptcfg->Find("status-fname", "").empty() == false)
    status_fname = strdup(aptcfg->Find("status-fname").c_str());
  // Set to a non-empty string to enable logging simplistically; set
  // to "-" to log to stdout.
  string log_file = aptcfg->Find(PACKAGE "::Logging::File", "");
  bool simulate = aptcfg->FindB(PACKAGE "::CmdLine::Simulate", false) ||
    aptcfg->FindB(PACKAGE "::Simulate", false);

  bool autoclean_only = aptcfg->FindB(PACKAGE "::UI::Autoclean-On-Startup", false);
  bool clean_only = aptcfg->FindB(PACKAGE "::UI::Clean-On-Startup", false);
  bool update_only = aptcfg->FindB(PACKAGE "::UI::Update-On-Startup", false);
  bool install_only = aptcfg->FindB(PACKAGE "::UI::Install-On-Startup", false);
  int quiet = aptcfg->FindI("quiet", 0);

#ifdef HAVE_GTK
  // TODO: this should be a configuration option.
  bool use_gtk_gui = aptcfg->FindB(PACKAGE "::Start-Gui", true);
  // Use the in-progress new GUI harness instead of the old code.
  bool use_new_gtk_gui = aptcfg->FindB("new-gui", false);
#endif

#ifdef HAVE_QT
  // Use Qt frontend.
  bool use_qt_gui = aptcfg->FindB("use-qt-gui", false);
#endif

  apply_config_file_logging_levels(_config);

  if(!log_file.empty())
    Logger::getLogger("")
      ->connect_message_logged(sigc::bind(sigc::ptr_fun(&handle_message_logged),
                                          log_file));

  temp::initialize("aptitude");

  if(quiet == 0 && !isatty(1))
    aptcfg->SetNoUser("quiet", 1);

  if(simulate)
    aptcfg->SetNoUser(PACKAGE "::Simulate", true);

  // Sanity-check
  {
    int num_startup_actions = 0;
    if(update_only)
      ++num_startup_actions;
    if(install_only)
      ++num_startup_actions;
    if(autoclean_only)
      ++num_startup_actions;
    if(clean_only)
      ++num_startup_actions;

    if(num_startup_actions > 1)
      {
	fprintf(stderr, "%s",
		_("Only one of --auto-clean-on-startup, --clean-on-startup, -i, and -u may be specified\n"));
	usage();
	exit(1);
      }
  }

  // Possibly run off and do other commands.
  if(cmdl.FileSize() > 0)
    {
      try
	{
	  // Connect up the "please consume errors" routine for the
	  // command-line.
	  consume_errors.connect(sigc::mem_fun(_error, (void (GlobalError::*)()) &GlobalError::DumpErrors));

	  if(update_only || install_only || autoclean_only || clean_only)
	    {
	      fprintf(stderr, "%s\n",
		      _("-u, -i, --clean-on-startup, and --autoclean-on-startup may not be specified with a command"));
	      usage();
	      exit(1);
	    }

	  // TODO: warn the user if they passed --full-resolver to
	  // something other than "upgrade" or do_action.

          atexit(dump_errors);
          atexit(apt_shutdown);

          cmdl.DispatchArg(cmds);

          return _error->PendingError() == true ? -1 : 0;
	}
      catch(StdinEOFException)
	{
	  printf("%s", _("Abort.\n"));
	  return -1;
	}
      catch(const cwidget::util::Exception &e)
	{
	  fprintf(stderr, _("Uncaught exception: %s\n"), e.errmsg().c_str());

	  std::string backtrace = e.get_backtrace();
	  if(!backtrace.empty())
	    fprintf(stderr, _("Backtrace:\n%s\n"), backtrace.c_str());
	  return -1;
	}
    }

#ifdef HAVE_QT
  if(use_qt_gui)
    {
      if(aptitude::gui::qt::main(argc, argv))
        return 0;

      // Otherwise, fall back to trying to start a curses interface
      // (assume that we can't contact the X server, or maybe that we
      // can't load the UI definition)
    }
#endif

#ifdef HAVE_GTK
  if(use_gtk_gui)
    {
      if(use_new_gtk_gui)
        {
          if(gui::init(argc, argv))
            return 0;
        }
      else if(gui::main(argc, argv))
	return 0;
      // Otherwise, fall back to trying to start a curses interface
      // (assume that we can't contact the X server, or maybe that we
      // can't load the UI definition)
    }
#endif

    {
      ui_init();

      try
        {
          progress_ref p=gen_progress_bar();
          // We can avoid reading in the package lists in the case that
          // we're about to update them (since they'd be closed and
          // reloaded anyway).  Obviously we still need them for installs,
          // since we have to get information about what to install from
          // somewhere...
          if(!update_only)
            apt_init(p->get_progress().unsafe_get_ref(), true, status_fname);
          if(status_fname)
            free(status_fname);
          check_apt_errors();

          file_quit.connect(sigc::ptr_fun(cw::toplevel::exitmain));

          if(apt_cache_file)
            {
              (*apt_cache_file)->package_state_changed.connect(sigc::ptr_fun(cw::toplevel::update));
              (*apt_cache_file)->package_category_changed.connect(sigc::ptr_fun(cw::toplevel::update));
            }

	  if(!aptcfg->FindB(PACKAGE "::UI::Flat-View-As-First-View", false))
	    do_new_package_view(*p->get_progress().unsafe_get_ref());
	  else
	    do_new_flat_view(*p->get_progress().unsafe_get_ref());

          p->destroy();
          p = NULL;

          if(update_only)
            do_update_lists();
          else if(install_only)
            do_package_run_or_show_preview();
	  else if(autoclean_only)
	    do_autoclean();
	  else if(clean_only)
	    do_clean();

          ui_main();
        }
      catch(const cwidget::util::Exception &e)
        {
          cw::toplevel::shutdown();

          fprintf(stderr, _("Uncaught exception: %s\n"), e.errmsg().c_str());

          std::string backtrace = e.get_backtrace();
          if(!backtrace.empty())
            fprintf(stderr, _("Backtrace:\n%s\n"), backtrace.c_str());

          return -1;
        }

      // The cache is closed when the close_on_exit object declared
      // above is destroyed.

      return 0;
    }
}
