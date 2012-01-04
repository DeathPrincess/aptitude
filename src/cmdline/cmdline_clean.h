// cmdline_clean.h                   -*-c++-*-
//
//  Copyright 2004 Daniel Burrows

#ifndef CMDLINE_CLEAN_H
#define CMDLINE_CLEAN_H

#include <apt-pkg/cmndline.h>

/** \file cmdline_clean.h
 */

bool cmdline_clean(CommandLine &cmdl);
bool cmdline_autoclean(CommandLine &cmdl);

#endif // CMDLINE_CLEAN_H
