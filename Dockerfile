
FROM --platform=linux/amd64 alpine:latest

RUN apk update && apk add curl

ADD ./Release/alpine/http-speedtest /http-speedtest

EXPOSE 8888

ENTRYPOINT [ "/http-speedtest" ]
CMD [ "server", "8888" ]


