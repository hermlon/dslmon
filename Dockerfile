FROM alpine:latest AS builder
WORKDIR /build/
RUN apk --no-cache add g++ cmake git make zlib-dev curl-dev
COPY .git .git
COPY packages packages
COPY src src
COPY CMakeLists.txt ./
RUN git submodule update --init --recursive
RUN cmake . && make

FROM alpine:latest
WORKDIR /root/
RUN apk --no-cache add libstdc++ zlib curl openssh
COPY --from=builder /build/src/dslmon ./
CMD ["./dslmon"]
