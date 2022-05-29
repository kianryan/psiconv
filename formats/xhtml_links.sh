#! /bin/sh

generate_links()
{
#local index_file name file lineno

  index_file="$1"
  
  command='sed '
  { 
    while read file lineno name; do
      command="$command -e "\''s,\['"$name"'\],<a name="'"$name"'"/>,g'\'
      command="$command -e "\''s,&#xab;'"$name"'&#xbb;,<a href="'"$file"\#"$name"'">'"$name"'</a>,g'\'
    done
  } < "$index_file"

  eval "$command"
}

generate_headers()
{
#  local index_file name this_file

  index_file_generate_headers="$1"
  this_file_generate_headers=`echo $2 | sed 's,.*/,,' | sed 's,\..*$,,'`

  name_generate_headers=`grep "^$this_file_generate_headers" "$index_file_generate_headers" | head -1 | sed s,'^[^ ]* [^ ]* ,,'`
  sed -e 's,<title>.*</title>,<title>'"$name_generate_headers"'</title>,'
}


cat "$2" | generate_links "$1"  | generate_headers "$1" "$2"
