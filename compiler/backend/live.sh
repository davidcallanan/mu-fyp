shopt -s nullglob

/app/bin/backend || exit 1

if [ -f /app/out/hello.o ]; then
  echo "Shell script is linking..."
  clang /app/out/hello.o -o /app/out/hello
  chmod +x /app/out/hello
  echo "Executable ready, let's run it for fun"
  echo "=#=#=#=#=#=#=#=#=#=#=#=#=#=#"
  /app/out/hello
  echo "=#=#=#=#=#=#=#=#=#=#=#=#=#=#"
fi

files=(/app/out/*)
if (( ${#files[@]} )); then
  cp -r "${files[@]}" /volume/out/
fi
