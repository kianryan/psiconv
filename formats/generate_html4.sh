#! /bin/sh

# Work around a BASH bug (prints a directory even in a non-interactive shell)
unset CDPATH

if test "$#" -lt 3 ; then
  echo "Syntax: $0 psiconv_dir output_dir files..."
  exit 1
fi

if test ! -d "$1"/program/psiconv ; then
  echo "First parameter should be base psiconv directory!"
  exit 1
fi
basedir=`cd $1; pwd`

if test ! -d "$2" ; then
  echo "Output directory does not exist!"
  exit 1
fi
outputdir=`cd $2; pwd`


shift
shift
echo "Generating html4 docs..."

libtool=$basedir/libtool
psiconv=$basedir/program/psiconv/psiconv
indexfile=$outputdir/index
tempdir=$outputdir/.temp
mkindex=$basedir/formats/index_html.sh
index=$tempdir/index
mkdef=$basedir/formats/html4_links.sh
config="$basedir/formats/psiconv.conf"


echo "Going to create the intermediate files..."
rm -rf $tempdir
mkdir $tempdir
for file in "$@"; do
  echo "Going to process $file..."
  outputfile=$tempdir/`basename $file|sed s,'.psi$','.html,'`
  echo $libtool --mode=execute $psiconv -c $config -o $outputfile -Thtml4 -eASCII $file 
  $libtool --mode=execute $psiconv -c $config -o $outputfile -Thtml4 -eASCII $file 
done

echo "Going to produce the index..."
(
  cd $tempdir
  files=
  for file in "$@"; do
    files="$files `basename $file|sed s,'.psi$','.html',`"
  done
  $mkindex $index $files
)

echo "Going to produce the final files..."
for file in "$@"; do
  echo "Going to process $file..."
  inputfile=$tempdir/`basename $file|sed s,'.psi$','.html,'`
  outputfile=$outputdir/`basename $file|sed s,'.psi$','.html,'`
  rm -f $outputfile
  echo $mkdef $index $inputfile \> $outputfile
  $mkdef $index $inputfile > $outputfile
done
