#!/bin/bash

# Generate certificates for TLS mutual authentication

echo "Generating CA private key..."
openssl genrsa -out ca-key.pem 2048

echo "Generating CA certificate..."
openssl req -new -x509 -key ca-key.pem -out ca-cert.pem -days 365 -subj "/C=US/ST=CA/L=San Francisco/O=Test CA/CN=Test CA"

echo "Generating server private key..."
openssl genrsa -out server-key.pem 2048

echo "Generating server certificate signing request..."
openssl req -new -key server-key.pem -out server.csr -subj "/C=US/ST=CA/L=San Francisco/O=Test Server/CN=localhost"

echo "Generating server certificate..."
openssl x509 -req -in server.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out server-cert.pem -days 365

echo "Generating client private key..."
openssl genrsa -out client-key.pem 2048

echo "Generating client certificate signing request..."
openssl req -new -key client-key.pem -out client.csr -subj "/C=US/ST=CA/L=San Francisco/O=Test Client/CN=client"

echo "Generating client certificate..."
openssl x509 -req -in client.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out client-cert.pem -days 365

echo "Cleaning up CSR files..."
rm -f server.csr client.csr

echo "Certificates generated successfully!"
echo "Files created:"
echo "  ca-cert.pem - CA certificate"
echo "  ca-key.pem - CA private key"
echo "  server-cert.pem - Server certificate"
echo "  server-key.pem - Server private key"
echo "  client-cert.pem - Client certificate"
echo "  client-key.pem - Client private key"