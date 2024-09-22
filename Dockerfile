FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && \
	apt-get install -y \
		git \
		perl \
		python3 \
		python3-pip \
		gperf \
		autoconf \
		bc \
		bison \
		gcc \
		clang \
		make \
		flex \
		build-essential \
		ca-certificates \
		ccache \
		libgoogle-perftools-dev \
		numactl \
		perl-doc \
		libfl2 \
		libfl-dev \
		zlib1g \
		zlib1g-dev \
		gtkwave \
		jq \
		verilator \
		vim \
		sudo
