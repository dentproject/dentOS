FROM opennetworklinux/builder9:1.6
MAINTAINER Steven Noble <snoble@netdef.org>

#
# The purpose of this image is fix issues with a expired ca-certificate used by lets encrypt
#

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install libgnutls30 pxz -y &&  rm -rf /var/lib/apt/lists/* && rm -rf /var/cache/apt/*

COPY docker_shell /bin/docker_shell
COPY container-id /bin/container-id
