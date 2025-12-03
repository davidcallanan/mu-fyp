module_path="$1"

if [ -z "$module_path" ]; then
    echo "Usage: $0 <module_path>"
    exit 1
fi

mkdir -p "$(pwd)/in"
mkdir -p "$(pwd)/out"

cp "${module_path}/frontend.out.json" "$(pwd)/in/frontend.out.json"

docker run \
    -v "$(pwd)/in:/volume/in" \
    -v "$(pwd)/out:/volume/out" \
    davidcallanan--mu-fyp--compiler-backend
