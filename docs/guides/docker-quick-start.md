# docker quick start

## prerequisites
- docker desktop installed and running

## build & run

### 1. build docker image
```bash
DOCKER_BUILDKIT=1 docker build -t apr-system:latest .
```

### 2. start container
```bash
docker run --rm -it -v $(pwd):/workspace -w /workspace apr-system:latest bash
```

### 3. build project
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 4. run tests
```bash
./bin/apr_tests
```

### 5. run APR system
```bash
./bin/apr_system
```

### 6. exit container
```bash
exit
```

## one-line commands

**run tests:**
```bash
docker run --rm -v $(pwd):/workspace -w /workspace apr-system:latest bash -c "mkdir -p build && cd build && cmake .. && make -j\$(nproc) && ./bin/apr_tests"
```

**run system:**
```bash
docker run --rm -v $(pwd):/workspace -w /workspace apr-system:latest bash -c "mkdir -p build && cd build && cmake .. && make -j\$(nproc) && ./bin/apr_system"
```
