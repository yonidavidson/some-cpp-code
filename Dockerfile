#
# Super simple example of a Dockerfile
#
FROM ubuntu:latest
MAINTAINER Yoni Davidson

RUN apt-get update
RUN apt-get install -y build-essential

WORKDIR /src
