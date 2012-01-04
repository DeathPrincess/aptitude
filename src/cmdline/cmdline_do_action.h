// cmdline_do_action.h                            -*-c++-*-
//
//  Copyright 2004 Daniel Burrows

#ifndef CMDLINE_DO_ACTION_H
#define CMDLINE_DO_ACTION_H

#include "cmdline_util.h"

#include <apt-pkg/cmndline.h>

/** \file cmdline_do_action.h
 */

enum resolver_mode_tp
  {
    resolver_mode_default,
    resolver_mode_safe,
    resolver_mode_full
  };

bool cmdline_do_action(CommandLine &cmdl);

#endif // CMDLINE_DO_ACTION_H
