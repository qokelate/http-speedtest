# A simple http speed test tool

## Usage:

```
usage: speedtest server <listen-port>
       speedtest client <server-url> [download-size] [upload-size]

eg:    speedtest server 8888
       speedtest client http://127.0.0.1:8888 300M 300M
or     curl 'http://192.168.0.2:8888/?size=300M' -o /dev/null
```

```
Server:

docker run -p 8888:8888 -d http-speedtest:latest
```


### `Release` for prebuilt binaries, macos/linux/windows

