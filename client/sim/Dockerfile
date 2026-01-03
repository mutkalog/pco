FROM debian:13

WORKDIR /app

COPY client-app /app/client-app

RUN mkdir -p /var/pco
RUN mkdir -p /opt/keys

RUN apt-get update && apt-get install -y \
    libarchive13 \
    libssl3 \
    libnettle8 \
    liblzma5 \
    libzstd1 \
    liblz4-1 \
    libbz2-1.0 \
    libxml2 \
    libxxhash0 \
    libc6 \
    libstdc++6 \
    libssl-dev \
    libarchive-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY devconfig.json /var/pco/devconfig.json
COPY security/ca.pem /opt/keys/ca.pem
COPY security/client.crt /opt/keys/client.crt
COPY security/client.key /opt/keys/client.key
COPY security/public.pem /opt/keys/public.pem

CMD ["/app/client-app"]



