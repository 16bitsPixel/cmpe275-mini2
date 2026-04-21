# cmpe275-mini2
CMPE275 Mini 2: Chunks

Python codegen
```
python3 -m grpc_tools.protoc \
  -I ./proto \
  --python_out=./python/src \
  --grpc_python_out=./python/src \
  ./proto/basecamp.proto
```