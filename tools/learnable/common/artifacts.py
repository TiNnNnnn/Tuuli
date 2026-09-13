"""Content-checked artifact IO shared by collection, encoding and training."""
from pathlib import Path
import zlib

def artifact_snapshot(paths: dict[str, Path]) -> dict:
    """Content provenance, not a cryptographic identity or a lock against concurrent installs."""
    snapshot = {}
    for name, path in paths.items():
        item = {"path": str(path.resolve())}
        try:
            checksum = size = 0
            with path.open("rb") as stream:
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    checksum = zlib.crc32(chunk, checksum)
                    size += len(chunk)
            item.update(size=size, crc32=f"{checksum:08x}")
        except OSError as error:
            item["error"] = str(error)
        snapshot[name] = item
    return snapshot

def read_snapshot(snapshot):
    raw = Path(snapshot['path']).read_bytes()
    if len(raw) != snapshot['size'] or f'{zlib.crc32(raw):08x}' != snapshot['crc32']:
        raise ValueError('snapshot content changed: ' + snapshot['path'])
    return raw
