FROM alpine:latest AS builder

RUN apk update && apk add gcc g++ cmake automake autoconf libtool make linux-headers git

WORKDIR /build

RUN git clone --depth=1 https://github.com/fumiama/simple-protobuf.git \
  && cd simple-protobuf \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make install

RUN rm -rf *

RUN git clone --depth=1 https://github.com/fumiama/simple-http-server.git \
  && cd simple-http-server \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make install

RUN rm -rf *

RUN git clone --depth=1 https://github.com/fumiama/C302.git \
  && cd C302 \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make install

RUN rm -rf *

COPY ./*.c .
COPY ./*.h .
COPY ./CMakeLists.txt .

RUN mkdir build \
  && cd build \
  && cmake .. \
  && make install

FROM alpine:latest

COPY --from=builder /usr/local/bin/simple-http-server /usr/bin/simple-http-server
COPY --from=builder /usr/local/bin/cmoe /data/cmoe
COPY --from=builder /usr/local/bin/c302 /data/c302
COPY --from=builder /usr/local/lib/libspb.so /usr/local/lib/libspb.so
RUN chmod +x /usr/bin/simple-http-server
RUN chmod +x /data/cmoe
RUN chmod +x /data/c302

WORKDIR /data
COPY ./assets/favicon.ico .
COPY ./assets/index.html .
COPY ./assets/style.css .

ENTRYPOINT [ "/usr/bin/simple-http-server" ]
