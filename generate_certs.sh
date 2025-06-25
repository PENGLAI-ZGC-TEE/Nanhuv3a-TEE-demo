#!/bin/bash

# Generate certificates for TLS mutual authentication

# 创建证书目录
CERTS_DIR="certs"
mkdir -p $CERTS_DIR

echo "Generating certificates in $CERTS_DIR directory..."

echo "Generating CA private key..."
openssl genrsa -out $CERTS_DIR/ca-key.pem 2048

echo "Generating CA certificate..."
openssl req -new -x509 -key $CERTS_DIR/ca-key.pem -out $CERTS_DIR/ca-cert.pem -days 365 -subj "/C=US/ST=CA/L=San Francisco/O=Test CA/CN=Test CA"

echo "Generating server private key..."
openssl genrsa -out $CERTS_DIR/server-key.pem 2048

echo "Generating server certificate signing request..."
openssl req -new -key $CERTS_DIR/server-key.pem -out $CERTS_DIR/server.csr -subj "/C=US/ST=CA/L=San Francisco/O=Test Server/CN=localhost"

echo "Generating server certificate..."
openssl x509 -req -in $CERTS_DIR/server.csr -CA $CERTS_DIR/ca-cert.pem -CAkey $CERTS_DIR/ca-key.pem -CAcreateserial -out $CERTS_DIR/server-cert.pem -days 365

echo "Generating client private key..."
openssl genrsa -out $CERTS_DIR/client-key.pem 2048

echo "Generating client certificate signing request..."
openssl req -new -key $CERTS_DIR/client-key.pem -out $CERTS_DIR/client.csr -subj "/C=US/ST=CA/L=San Francisco/O=Test Client/CN=client"

echo "Generating client certificate..."
openssl x509 -req -in $CERTS_DIR/client.csr -CA $CERTS_DIR/ca-cert.pem -CAkey $CERTS_DIR/ca-key.pem -CAcreateserial -out $CERTS_DIR/client-cert.pem -days 365

echo "Cleaning up CSR files..."
rm -f $CERTS_DIR/server.csr $CERTS_DIR/client.csr

echo "Certificates generated successfully in $CERTS_DIR/!"
echo "Files created:"
echo "  $CERTS_DIR/ca-cert.pem - CA certificate"
echo "  $CERTS_DIR/ca-key.pem - CA private key"
echo "  $CERTS_DIR/server-cert.pem - Server certificate"
echo "  $CERTS_DIR/server-key.pem - Server private key"
echo "  $CERTS_DIR/client-cert.pem - Client certificate"
echo "  $CERTS_DIR/client-key.pem - Client private key"

# 设置适当的权限
chmod 600 $CERTS_DIR/*-key.pem
chmod 644 $CERTS_DIR/*-cert.pem $CERTS_DIR/ca-cert.pem

echo "Certificate permissions set correctly."