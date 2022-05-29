#! /bin/sh

compute_command_line()
{
  index_file="$1"
  printf "sed "
  while read file lineno name; do
    printf "%s %s " -e \''s,\['"$name"'\],<A NAME="'"$name"'">,g'\'
    printf "%s %s " -e \''s,&#xab;'"$name"'&#xbb;,<A HREF="'"$file"\#"$name"'">'"$name"'</A>,g'\'
  done < "$index_file"
}

generate_links()
{
  command=`compute_command_line "$1"`
#echo $command 
  eval "$command"
}

generate_headers()
{
  index_file_generate_headers="$1"
  this_file_generate_headers=`echo $2 | sed 's,.*/,,' | sed 's,\..*$,,'`

  name_generate_headers=`grep "^$this_file_generate_headers" "$index_file_generate_headers" | head -1 | sed s,'^[^ ]* [^ ]* ,,'`
  sed -e 's,<TITLE>.*</TITLE>,<TITLE>'"$name_generate_headers"'</TITLE>,'
}


cat "$2" | generate_links "$1"  | generate_headers "$1" "$2"
