FROM opennetworklinux/builder10:1.2

MAINTAINER Taras Chornyi <chornyi.taras@outlook.com>

RUN apt-get update --allow-releaseinfo-change && DEBIAN_FRONTEND=noninteractive apt-get upgrade -y  && DEBIAN_FRONTEND=noninteractive apt-get -y install apt-cacher-ng \
 libmnl-dev libmnl-dev  libelf-dev libcap-dev libxtables-dev  libdb-dev

RUN xapt -a arm64 libmnl-dev libelf-dev libcap-dev libxtables-dev libdb-dev

RUN echo "deb http://deb.debian.org/debian buster-backports main" | tee /etc/apt/sources.list.d/backports.list

RUN apt-get update --allow-releaseinfo-change && DEBIAN_FRONTEND=noninteractive apt-get -y install -t buster-backports git

RUN rm -rf /var/lib/apt/lists/* && rm -rf /var/cache/apt/*

COPY docker_shell /bin/docker_shell
COPY container-id /bin/container-id
