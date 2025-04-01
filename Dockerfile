FROM ubuntu:22.04 AS build_plugins
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake pkg-config libglm-dev libglfw3-dev libssl-dev libcurl4-openssl-dev gcc-13 g++-13
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && CC=gcc-13 CXX=g++-13 cmake .. && make

FROM python:3.11-alpine

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=8000 \
    PLUGIN_DIR=plugins

WORKDIR /app

COPY . .

RUN addgroup -S appgroup && adduser -S appuser -G appgroup
RUN chown -R appuser:appgroup /app
USER appuser

EXPOSE ${PORT}

CMD ["python3", "server.py", "/app/build"]