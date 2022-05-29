#! /bin/sh

echo "Generating ascii docs..."

basedir="$1"
outputdir="$2"
shift
shift

libtool="$basedir/libtool"
psiconv="$basedir/program/psiconv/psiconv"
config="$basedir/formats/psiconv.conf"

for file in "$@"; do
  echo "Going to process $file..."
  outputfile=$outputdir/`basename $file|sed s,'.psi$','.ascii',`
  echo $libtool --mode=execute $psiconv -c $config -o $outputfile -Tascii $file 
  $libtool --mode=execute $psiconv -c $config -o $outputfile -Tascii $file
done
