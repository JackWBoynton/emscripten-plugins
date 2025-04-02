from litestar import Router

from . import plugins
from . import emscripten_client

router = Router(
    path="/api",
    route_handlers=[
        plugins.fetch_plugin,
        plugins.list_plugins,
        plugins.list_plugins_arch,
        emscripten_client.static_router,
    ],
)

__all__ = ["router"]
