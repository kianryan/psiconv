#!/bin/sh
# 
# Run this before configure
#
# This file blatantly ripped off from subversion and Abiword

# To run this file, you need:
#   automake >= 1.6 (tested with 1.6.3, 1.7.8; older will probably not work)
#   libtool >= 1.5  (tested with 1.5; older will probably not work)
#   autoconf >= 2.50 (tested with 2.50, 2.57; older will not work)

# Set this to a specific version if you want to use a non-standard automake
#AUTOMAKE_VERSION=-1.6
# Set this to a specific version if you want to use a non-standard autoconf
#AUTOCONF_VERSION=2.50

set -e

echo "Libtool..."
libtoolize --copy --force

# Produce aclocal.m4, so autoconf gets the automake macros it needs
echo "Creating aclocal.m4..."
aclocal$AUTOMAKE_VERSION

autoheader$AUTOCONF_VERSION

# Produce all the `Makefile.in's, verbosely, and create neat missing things
# like `libtool', `install-sh', etc.
automake$AUTOMAKE_VERSION --add-missing --verbose --gnu --copy --force-missing

# If there's a config.cache file, we may need to delete it.  
# If we have an existing configure script, save a copy for comparison.
if [ -f config.cache ] && [ -f configure ]; then
  cp configure configure.$$.tmp
fi

# Produce ./configure
echo "Creating configure..."
autoconf$AUTOCONF_VERSION

echo ""
echo "You can run ./configure now."
echo ""

