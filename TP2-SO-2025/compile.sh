# # Validates the existance of the TPE-ARQ container, starts it up & compiles the project
# CONTAINER_NAME="TP2-SO-2025-g02-64018-64288-64646"

# # COLORS
# RED='\033[0;31m'
# YELLOW='\033[1;33m'
# GREEN='\033[0;32m'
# NC='\033[0m'

# docker ps -a &> /dev/null

# if [ $? -ne 0 ]; then
#     echo "${RED}Docker is not running. Please start Docker and try again.${NC}"
#     exit 1
# fi

# # Check if container exists
# if [ ! "$(docker ps -a | grep "$CONTAINER_NAME")" ]; then
#     echo "${YELLOW}Container $CONTAINER_NAME does not exist. ${NC}"
#     echo "Pulling image..."
#     docker pull agodio/itba-so-multi-platform:3.0
#     echo "Creating container..."
#     # Note: ${PWD}:/root. Using another container to compile might fail as the compiled files would not be guaranteed to be at $PWD
#     # Always use TPE-ARQ to compile
#     docker run -d -v ${PWD}:/root --security-opt seccomp:unconfined -it --name "$CONTAINER_NAME" agodio/itba-so-multi-platform:3.0
#     echo "${GREEN}Container $CONTAINER_NAME created.${NC}"
# else
#     echo "${GREEN}Container $CONTAINER_NAME exists.${NC}"
# fi

# # Start container
# docker start "$CONTAINER_NAME" &> /dev/null
# echo "${GREEN}Container $CONTAINER_NAME started.${NC}"

# # Compiles
# docker exec -it "$CONTAINER_NAME" make clean -C /root/ && \
# docker exec -it "$CONTAINER_NAME" make all -C /root/Toolchain && \
# docker exec -it "$CONTAINER_NAME" bash -c "echo '--- CONTENIDO DEL MAKEFILE EN DOCKER ---'; cat /root/Kernel/Makefile; echo '--- FIN DEL MAKEFILE ---'" && \
# docker exec -it "$CONTAINER_NAME" make all -C /root/


# if [ $? -ne 0 ]; then
#     echo "${RED}Compilation failed.${NC}"
#     exit 1
# fi

# echo "${GREEN}Compilation finished.${NC}"

#!/bin/bash

# --- SELECCIÓN DEL MEMORY MANAGER ---
# El primer argumento de este script será el MM.
# Si no se pasa argumento (ej. ./compile.sh), usa 'first_fit' por defecto.
# Uso:
#   ./compile.sh first_fit
#   ./compile.sh buddy
MEM_MANAGER_ARG=${1:-first_fit}
# ------------------------------------


# Validates the existance of the TPE-ARQ container, starts it up & compiles the project
CONTAINER_NAME="TP2-SO-2025-g02-64018-64288-64646"

# COLORS
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

docker ps -a &> /dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}Docker is not running. Please start Docker and try again.${NC}"
    exit 1
fi

# (El resto de la lógica de Docker es idéntica)
# Check if container exists
if [ ! "$(docker ps -a | grep "$CONTAINER_NAME")" ]; then
    echo -e "${YELLOW}Container $CONTAINER_NAME does not exist. ${NC}"
    echo "Pulling image..."
    docker pull agodio/itba-so-multi-platform:3.0
    echo "Creating container..."
    docker run -d -v ${PWD}:/root --security-opt secomp:unconfined -it --name "$CONTAINER_NAME" agodio/itba-so-multi-platform:3.0
    echo -e "${GREEN}Container $CONTAINER_NAME created.${NC}"
else
    echo -e "${GREEN}Container $CONTAINER_NAME exists.${NC}"
fi

# Start container
docker start "$CONTAINER_NAME" &> /dev/null
echo -e "${GREEN}Container $CONTAINER_NAME started.${NC}"

# Compiles
echo -e "${YELLOW}Compilando con Memory Manager: ${MEM_MANAGER_ARG}${NC}"

docker exec -it "$CONTAINER_NAME" make clean -C /root/ && \
docker exec -it "$CONTAINER_NAME" make all -C /root/Toolchain && \
docker exec -it "$CONTAINER_NAME" bash -c "echo '--- CONTENIDO DEL MAKEFILE EN DOCKER ---'; cat /root/Kernel/Makefile; echo '--- FIN DEL MAKEFILE ---'" && \

# --- ¡ESTA ES LA LÍNEA MODIFICADA! ---
# Pasamos la variable MM=${MEM_MANAGER_ARG} al comando 'make' principal.
# El makefile de la raíz (el que llama a cd Kernel; make all) pasará esta
# variable al sub-make.
docker exec -it "$CONTAINER_NAME" make all -C /root/ MM=${MEM_MANAGER_ARG}


if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed.${NC}"
    exit 1
fi

echo -e "${GREEN}Compilation finished.${NC}"
