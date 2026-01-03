FROM debian:13

WORKDIR /app

COPY server-app /app/server-app

RUN apt-get update && apt-get install -y \
    libarchive13 \
    libssl3 \
    libc6 \
    libstdc++6 \
    libssl-dev \
    libarchive-dev \
    ca-certificates \
    libpqxx-7.10 \
    libgcc-s1 \
    zlib1g \
    libzstd1 \
    libpq5 \
    libnettle8 \
    libacl1 \
    liblzma5 \
    liblz4-1 \
    libbz2-1.0 \
    libxml2 \
    libgssapi-krb5-2 \
    libxxhash0 \
    libkrb5-3 \
    libk5crypto3 \
    libcom-err2 \
    libkrb5support0 \
    libsasl2-2 \
    libkeyutils1 \
    && rm -rf /var/lib/apt/lists/*

COPY security/ca.pem     /app/ca.pem
COPY security/server.crt /app/server.crt
COPY security/server.key /app/server.key

CMD ["/app/server-app", "--ca", "/app/ca.pem", "--cert", "/app/server.crt", "--key", "/app/server.key", "--test"]



