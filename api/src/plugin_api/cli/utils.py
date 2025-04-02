
import click

@click.command()
@click.argument("registry_url", type=str)
def create_registry(registry_url: str) -> None:
    """
    Create a plugin registry from the repository URL.
    """
    ...

