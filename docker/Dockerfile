# This docker file is to build a zero training environment
FROM nvidia/cuda:10.1-cudnn7-devel-ubuntu16.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
	vim \
	gdb \
	curl \
	less \
	htop \
	cmake \
	unzip \
	gconf2 \
	default-jre \
	libboost-all-dev && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

# install pytorch
RUN curl -o ~/miniconda.sh https://repo.anaconda.com/miniconda/Miniconda3-py37_4.8.3-Linux-x86_64.sh && \
	chmod +x ~/miniconda.sh && \
	~/miniconda.sh -b -p /opt/conda && \
	rm ~/miniconda.sh && \
	/opt/conda/bin/conda install -y pytorch torchvision torchaudio cudatoolkit=10.1 -c pytorch && \
	/opt/conda/bin/conda clean -ya
ENV PATH /opt/conda/bin:$PATH

# pybind
RUN pip install pybind11[global]

# install gogui
COPY gogui-1.4.9.zip .
RUN \
  unzip gogui-1.4.9.zip && \
  cd gogui-1.4.9 && \
  ./install.sh && \
  cd .. && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/* && \
  rm -rf gogui-1.4.9 gogui-1.4.9.zip

WORKDIR /workspace
