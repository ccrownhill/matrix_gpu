#!/bin/sh

container_name="$1"
if [ -z "$1" ]; then
    container_name="epic_accelerator_cont"
fi

docker rm "$container_name"
docker run -it -v "`pwd`:`pwd`" -w "`pwd`" --name "$container_name" epic_accelerator
