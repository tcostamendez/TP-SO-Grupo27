#!/bin/bash

CONTAINER_NAME="TP2-SO-2025-g02-64018-64288-64646"

echo "--- Iniciando GDB dentro del contenedor Docker ---"
docker exec -it "$CONTAINER_NAME" gdb