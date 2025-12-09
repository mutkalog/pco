curl -v \
  --cacert ../../../mtls/ca.pem \
  --cert ../../../mtls/client/client.crt \
  --key ../../../mtls/client/client.key \
  -X POST "https://localhost:39024/upload" \
  -F "meta=@raw.json;type=application/json" \
  -F "archive=@app4/app.tar.gz;type=application/gzip"
