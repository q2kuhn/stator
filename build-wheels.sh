#!/bin/bash
DOCKER_IMAGE=quay.io/pypa/manylinux2014_x86_64
PLAT=manylinux2014_x86_64
docker run --rm -e PLAT=$PLAT -v `pwd`:/io $DOCKER_IMAGE $PRE_CMD /io/scripts/manylinux_entrypoint.sh
