FROM ubuntu:22.04

WORKDIR /app

COPY pco-client-ota /app/pco-client-ota

RUN mkdir -p /var/pco
RUN mkdir -p /opt/keys

COPY devconfig.json /var/pco/devconfig.json
COPY ca.pem /opt/keys/ca.pem
COPY client.crt /opt/keys/client.crt
COPY client.key /opt/keys/client.key
COPY public.pem /opt/keys/public.pem

CMD ["/app/pco-client-ota"]



