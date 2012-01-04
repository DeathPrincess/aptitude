// cmdline_clean.cc
//
// Copyright (C) 2004, 2010 Daniel Burrows
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


// Local includes:
#include "cmdline_clean.h"

#include "text_progress.h"
#include "terminal.h"

#include <aptitude.h>

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>


// System includes:
#include <apt-pkg/acquire.h>
#include <apt-pkg/clean.h>
#include <apt-pkg/error.h>
#include <apt-pkg/cmndline.h>
#include <apt-pkg/strutl.h>

#include <iostream>

#include <stdio.h>
#include <sys/stat.h>

using namespace std;

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;
using aptitude::cmdline::terminal_locale;
using boost::shared_ptr;

bool cmdline_clean(CommandLine &cmdl)
{
  const shared_ptr<terminal_io> term = create_terminal();

  _error->DumpErrors();

  if(cmdl.FileSize() != 1)
    return _error->Error(_("The clean command takes no arguments"));

  shared_ptr<OpProgress> progress = make_text_progress(false, term, term, term);

  apt_init(progress.get(), false);

  if(_error->PendingError())
    return false;

  const bool simulate = aptcfg->FindB(PACKAGE "::Simulate", false);

  // In case we aren't root.
  if(!simulate)
    apt_cache_file->GainLock();
  else
    apt_cache_file->ReleaseLock();

  if(_error->PendingError())
    return false;

  if(aptcfg)
    {
      if(simulate)
	printf(_("Del %s* %spartial/*\n"),
	       aptcfg->FindDir("Dir::Cache::archives").c_str(),
	       aptcfg->FindDir("Dir::Cache::archives").c_str());
      else
	{
	  pkgAcquire fetcher;
	  fetcher.Clean(aptcfg->FindDir("Dir::Cache::archives"));
	  fetcher.Clean(aptcfg->FindDir("Dir::Cache::archives")+"partial/");
	}
    }

  if(_error->PendingError())
    return false;

  return true;
}

// Shamelessly stolen from apt-get:
class LogCleaner : public pkgArchiveCleaner
{
  bool simulate;

  long total_size;

protected:
  virtual void Erase(const char *File,string Pkg,string Ver,struct stat &St) 
  {
    printf(_("Del %s %s [%sB]\n"),
	   Pkg.c_str(),
	   Ver.c_str(),
	   SizeToStr(St.st_size).c_str());

    if (!simulate)
      {
	if(unlink(File)==0)
	  total_size+=St.st_size;
      }
    else
      total_size+=St.st_size;
  };
public:
  LogCleaner(bool _simulate):simulate(_simulate), total_size(0) { }

  long get_total_size() {return total_size;}
};

bool cmdline_autoclean(CommandLine &cmdl)
{
  const shared_ptr<terminal_io> term = create_terminal();

  _error->DumpErrors();

  if(cmdl.FileSize() != 1)
    return _error->Error(_("The autoclean command takes no arguments"));

  shared_ptr<OpProgress> progress = make_text_progress(false, term, term, term);

  apt_init(progress.get(), false);

  if(_error->PendingError())
    return false;

  const bool simulate = aptcfg->FindB(PACKAGE "::Simulate", false);

  // In case we aren't root.
  if(!simulate)
    apt_cache_file->GainLock();
  else
    apt_cache_file->ReleaseLock();

  if(_error->PendingError())
    return false;

  LogCleaner cleaner(simulate);
  bool rval = true;
  if(!(cleaner.Go(aptcfg->FindDir("Dir::Cache::archives"), *apt_cache_file) &&
       cleaner.Go(aptcfg->FindDir("Dir::Cache::archives")+"partial/",
		  *apt_cache_file)) ||
     _error->PendingError())
    rval = false;

  _error->DumpErrors();

  if(simulate)
    printf(_("Would free %sB of disk space\n"),
	   SizeToStr(cleaner.get_total_size()).c_str());
  else
    printf(_("Freed %sB of disk space\n"),
	   SizeToStr(cleaner.get_total_size()).c_str());

  exit(rval);
}

