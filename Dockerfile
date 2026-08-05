FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    python3 \
    python3-dev \
    python3-pip \
    bison \
    flex \
    libreadline-dev \
    libicu-dev \
    libssl-dev \
    libsasl2-dev \
    libzstd-dev \
    libpq-dev \
    libpqxx-dev \
    librdkafka-dev \
    nlohmann-json3-dev \
    libsqlite3-dev \
    debhelper \
    dpkg-dev \
    fakeroot \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN doxygen Doxyfile || echo "Doxygen warnings ignored"

RUN find debian -type f -exec dos2unix {} \; || true

RUN chmod +x debian/rules

RUN dpkg-buildpackage -b -us -uc

RUN mkdir -p /output && cp /*.deb /output/

VOLUME /output