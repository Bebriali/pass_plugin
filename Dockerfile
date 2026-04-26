FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    clang \
    llvm \
    cmake \
    build-essential \
    python3 \
    python3-pip \
    python3-graphviz \
    libgraphviz-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir -p build prog log/json log/pic

RUN chmod +x ./run_all.sh

CMD ["./run_all.sh"]