# cmpe275-mini2
CMPE275 Mini 2: Chunks

### Python codegen
```
python3 -m grpc_tools.protoc \
  -I ./proto \
  --python_out=./python/src \
  --grpc_python_out=./python/src \
  ./proto/basecamp.proto
```

### For python venv
```
pip install protobuf grpcio grpcio-tools
```

### To build:
```sh
mkdir build && cd build
cmake ..
make
```

### Start server
ex: A.conf
```
./basecamp_server --config ../config/<Config File>
```

### Client commands:
ex: 127.0.0.1:50051
```
./basecamp_client --target <server_location> --ping
./basecamp_client --target <server_location> --neighbors
./basecamp_client --target <server_location> --probe --ttl 1 --payload hello
```