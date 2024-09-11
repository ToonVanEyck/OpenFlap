#!/usr/bin/env bash

echo "Usb event: $1 $2 $3 $4" >> /tmp/docker_tty.log
openflap_dev_container=$(docker ps -qf name=openflap_devcontainer-openflap*)
if [ ! -z "$openflap_dev_container" ]
then
if [ "$1" == "added" ]
    then
        docker exec -u 0 $openflap_dev_container mknod $2 c $3 $4
        docker exec -u 0 $openflap_dev_container chmod -R 777 $2
        echo "Adding $2 to openflap" >> /tmp/docker_tty.log
    else
        docker exec -u 0 $openflap_dev_container rm $2
        echo "Removing $2 from openflap" >> /tmp/docker_tty.log
    fi
fi
