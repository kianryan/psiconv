#! /bin/sh

make_targets_file()
{
#  local file line line_nr error targets_file files
  targets_file="$1"
  shift
  files="$@"
  printf "" > "$targets_file"

  for file in $files; do
    (
      line_nr=1
      while read line; do
        error=0
        while [ $error -eq 0 ] && echo $line | grep '\[' >/dev/null ; do
          if echo $line | grep '\[.*\]' >/dev/null; then
            printf "%s %s " "$file" "$line_nr" >> "$targets_file"
            echo $line | sed -e 's,^[^\[]*\[,,' -e 's,\].*$,,' \
                 >> "$targets_file"
          else 
            echo "In \`$file\' line $line_nr: target brackets inbalance" >&2
            error=1
          fi
          line=`echo $line | sed -e 's,^[^]]*\],,'`
        done
        if [ $error -eq 0 ] && echo $line | grep '\]' >/dev/null ; then
          echo "In \`$file\' line $line_nr: target brackets inbalance" >&2
          error=1
        fi
	line_nr=`echo "$line_nr + 1" | bc`
      done
    ) < $file
  done
}


make_targets_file "$@"
