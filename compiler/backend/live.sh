shopt -s nullglob

/app/bin/backend

files=(/app/out/*)
if (( ${#files[@]} )); then
  cp -r "${files[@]}" /volume/out/
fi
