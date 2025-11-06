#!/bin/bash
MEM_MANAGER_ARG=${1:-first_fit}
CONTAINER_NAME="TP2-SO-2025-G27-65510-65625-65148"   # unify name

# (same docker checks/creation as your compile.sh)
# ...
docker start "$CONTAINER_NAME" >/dev/null

echo "Compiling DEBUG build with MM=${MEM_MANAGER_ARG}..."
docker exec -it "$CONTAINER_NAME" make clean -C /root/ && \
docker exec -it "$CONTAINER_NAME" make all -C /root/Toolchain && \
docker exec -it "$CONTAINER_NAME" make all -C /root/ DEBUG=1 MM=${MEM_MANAGER_ARG} || exit 1

# If your build drops a symbol file (e.g., Kernel.elf), copy it out if needed:
# docker cp "$CONTAINER_NAME":/root/Kernel/Kernel.elf ./Image/Kernel.elf
