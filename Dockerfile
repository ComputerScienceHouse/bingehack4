FROM ubuntu:15.04
MAINTAINER Liam Middlebrook <loothelion@csh.rit.edu>

# Install all of the dependencies needed to build Bingehack4

ENV compilers gcc-4.6 g++-4.6 gcc-4.7 g++-4.7 gcc-4.8 g++-4.8 clang-3.4 clang-3.4 clang-3.5 clang-3.5

RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 16126D3A3E5C1192
RUN apt-get update -qq
RUN apt-get install -qq binutils $compilers flex bison libbison-dev libjansson-dev postgresql-client libpq-dev libsdl2-dev libpng-dev
