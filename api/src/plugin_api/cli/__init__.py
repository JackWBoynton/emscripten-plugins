import enum
import logging

import click

import uvicorn

from .. import __version__
from ..app import app

from .utils import create_registry

log = logging.getLogger("plugin_api.cli")

@click.group(
    help=f"Emscripten Plugin Registry API v{__version__}",
    context_settings={"help_option_names": ["-h", "--help"], "show_default": True},
)
@click.option("-v", "--verbose", is_flag=True, help="verbose output")
def cli(verbose: bool) -> None:
    logging.captureWarnings(True)


@cli.command()
@click.option(
    "--host",
    default="0.0.0.0",
    help="host to listen on",
)
@click.option(
    "--port",
    default=8000,
    help="port to listen on",
)
def run(host: str, port: int) -> None:
    log.info(f"Starting server on {host}:{port}")
    uvicorn.run(app, host=host, port=port)

cli.add_command(create_registry)