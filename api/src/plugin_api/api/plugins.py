"""Endpoint that hosts binary plugins for all platforms and provides them to the client to download."""

import os
from io import BytesIO

import hashlib

from pydantic import BaseModel

from litestar.exceptions import NotFoundException
from litestar import get
from litestar.response import Stream

# the plugins are all build in gitlab CI.
# the build artifacts are uploaded to the registry

# registry:
# - plugins
#   - pluginA
#     - version1
#       - windows
#         - pluginA.instroplug | pluginA.instroplug_lib
#       - linux
#         - pluginA.instroplug | pluginA.instroplug_lib
#     - dev
#       - feature/some-branch
#         - windows
#           - pluginA.instroplug | pluginA.instroplug_lib
#         - linux
#           - pluginA.instroplug | pluginA.instroplug_lib
#   - pluginB

class Plugin(BaseModel):
    name: str
    size: int
    sha1: str
    version: str

    def __dict__(self):
        return {
            "name": self.name,
            "size": self.size,
            "sha1": self.sha1,
            "version": self.version,
        }

def get_sha1(file_path) -> str:
    BUF_SIZE = 65536
    sha1 = hashlib.sha1()

    with open(file_path, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha1.update(data)

    return sha1.hexdigest()

REGISTRY_BASE_PATH = os.getenv("REGISTRY_BASE_PATH", "./plugins")
print(os.listdir(REGISTRY_BASE_PATH))

def get_file_from_registry(arch: str, plugin_name: str) -> BytesIO:
    """
    Retrieve a specific plugin file from the registry.

    Args:
        arch: The target architecture (e.g., "windows", "linux").
        plugin_name: The name of the plugin file (e.g., "pluginA.instroplug").

    Returns:
        BytesIO: A stream of the plugin file.
    """
    path = os.path.join(REGISTRY_BASE_PATH, arch, plugin_name)
    if not os.path.exists(path):
        raise NotFoundException(f"Plugin {plugin_name} for {arch} not found.")
    
    with open(path, "rb") as f:
        return BytesIO(f.read())
    
def get_plugin_list() -> dict:
    """
    Retrieve the list of all plugins in the registry.

    Returns:
        dict: A dictionary containing plugin names and their respective architectures.
    """
    plugins = {}
    if not os.path.exists(REGISTRY_BASE_PATH):
        return plugins

    for arch in os.listdir(REGISTRY_BASE_PATH):
        arch_path = os.path.join(REGISTRY_BASE_PATH, arch)
        if os.path.isdir(arch_path):
            plugins[arch] = []
            for plugin_name in os.listdir(arch_path):
                if not os.path.isdir(os.path.join(arch_path, plugin_name)):
                    plugin = Plugin(name=plugin_name, size=os.path.getsize(os.path.join(arch_path, plugin_name)), sha1=get_sha1(os.path.join(arch_path, plugin_name)), version=plugin_name)
                    plugins[arch].append(plugin)
    return plugins

print(get_plugin_list())


@get("/plugins/{arch:str}/{plugin_name:str}")
async def fetch_plugin(
    arch: str,
    plugin_name: str
) -> Stream:
    """
    Fetch and stream a specific plugin to the client.

    Args:
        arch: The target architecture (e.g., "windows", "linux").
        plugin_name: The name of the plugin file (e.g., "pluginA.instroplug").

    Returns:
        Stream: A streaming response of the plugin file.
    """
    return Stream(get_file_from_registry(arch, plugin_name))

@get("/plugins")
async def list_plugins() -> dict:
    """
    List all available plugins in the registry.

    Returns:
        dict: The entire plugin registry.
    """
    return get_plugin_list()

@get("/plugins/{arch:str}")
async def list_plugins_arch(arch: str) -> dict:
    """
    List all available plugins in the registry.

    Returns:
        dict: The entire plugin registry.
    """
    return get_plugin_list().get(arch, {})