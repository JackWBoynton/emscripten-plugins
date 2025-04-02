from litestar import Litestar
from litestar.openapi.plugins import SwaggerRenderPlugin
from litestar.config.cors import CORSConfig

from plugin_api.api import router

swagger_plugin = SwaggerRenderPlugin(version="5.1.3", path="/swagger")
cors_config = CORSConfig(
    allow_origins="*",
    allow_methods=["*"],
    allow_headers=["*"],
)

app = Litestar(route_handlers=[router], plugins=[swagger_plugin], cors_config=cors_config)
