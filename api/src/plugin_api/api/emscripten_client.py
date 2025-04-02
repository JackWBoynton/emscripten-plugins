import os

from litestar.static_files import create_static_files_router


EMSCRIPTEN_BUILD_WEBDIR = os.getenv("EMSCRIPTEN_BUILD_WEBDIR", "./web")
static_router = create_static_files_router(
    path="/static", directories=[os.path.abspath(EMSCRIPTEN_BUILD_WEBDIR)], html_mode=True
)
