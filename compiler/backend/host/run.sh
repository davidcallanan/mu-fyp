docker build -t davidcallanan--mu-fyp--compiler-backend . && \
docker run -it -v "$(pwd)/in:/volume/in" -v "$(pwd)/out:/volume/out" davidcallanan--mu-fyp--compiler-backend
