#!/bin/bash
IP=$(hostname -I | awk '{print $1}')
BASE=${SKYLARK_BASE:-/skylark}
mkdir -p ${BASE}/data/certs
openssl req -x509 -newkey rsa:2048 \
  -keyout ${BASE}/data/certs/key.pem \
  -out ${BASE}/data/certs/cert.pem \
  -days 365 -nodes \
  -subj "/CN=skylark" \
  -addext "subjectAltName=IP:${IP}"
echo "Cert generated for IP: ${IP}"