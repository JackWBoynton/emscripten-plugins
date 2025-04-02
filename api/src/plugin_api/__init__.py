"""plugin_api"""

import logging
from importlib.metadata import PackageNotFoundError, version

PACKAGE_NAME = "plugin_api"

try:
    __version__: str | None = version(PACKAGE_NAME)
except PackageNotFoundError:
    __version__ = None


logger = logging.getLogger(__name__)
