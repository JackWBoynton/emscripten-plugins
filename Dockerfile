FROM ubuntu:24.04 AS build_plugins
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential git cmake pkg-config libglm-dev libglfw3-dev libssl-dev libcurl4-openssl-dev gcc-13 g++-13
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && CC=gcc-13 CXX=g++-13 cmake .. && make

FROM scratch AS export_artifacts
WORKDIR /artifacts
COPY --from=build_plugins /app/build .


FROM python:3.12-slim-bookworm as server
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1
WORKDIR /app
COPY . .
RUN uv sync --frozen --project api

EXPOSE ${PORT}

CMD ["REGISTRY_BASE_PATH=./server/plugins EMSCRIPTEN_BUILD_WEBDIR=./server/web uv --project api run api run"]