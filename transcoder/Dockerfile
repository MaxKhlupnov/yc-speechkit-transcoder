FROM restreamio/gstreamer:latest-dev 
# 
# https://hub.docker.com/r/restreamio/gstreamer
# 
MAINTAINER Yandex Cloud solution architects

ENV SK_HOME=/speechkit

#
# Create a user and home directory 
#
USER root
RUN groupadd -r speechkit -g 1001 && \
    useradd -u 1001 -r -g speechkit -m -d $SK_HOME -s /sbin/nologin -c "speechkit developer adoption service user" speechkit && \
    chmod 755 $SK_HOME
RUN apt-get update -y  && \
    apt-get upgrade -y 
RUN apt-get install -y build-essential autoconf libtool pkg-config automake curl cmake


# Set the working directory to the speechkit home directory
WORKDIR $SK_HOME
RUN git clone https://github.com/grpc/grpc grpc

WORKDIR $SK_HOME/grpc

RUN git submodule update --init
# Build and install protobuf
RUN cmake . && \
    make  && \
    make install