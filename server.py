#!/usr/bin/env python3
import sys
import hashlib
import http.server
import os
import json
from urllib.parse import urlparse

PORT = 8000

# Directory that holds plugin files
PLUGIN_DIR = os.getenv("PLUGIN_DIR", os.path.join(os.getcwd(), "server/plugins"))
WEB_BUILD_DIR = os.getenv("WEB_BUILD_DIR", os.path.join(os.getcwd(), "server/web"))


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

class PluginServerHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers
        self.send_header("Access-Control-Allow-Origin", "*")  # Allow all origins
        self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

        # Add COOP and COEP headers
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")

        # Add Cross-Origin-Resource-Policy for plugin files
        parsed_path = urlparse(self.path)
        if parsed_path.path.startswith("/plugins"):
            self.send_header("Cross-Origin-Resource-Policy", "same-origin")

        http.server.SimpleHTTPRequestHandler.end_headers(self)

    def do_OPTIONS(self):
        # Handle preflight requests
        self.send_response(200, "OK")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        parsed_path = urlparse(self.path)
        if parsed_path.path.startswith("/plugins"):
            self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.end_headers()

    def do_GET(self):
        # TODO: cache
        parsed_path = urlparse(self.path)
        if parsed_path.path == "/plugins":
            files = []
            if os.path.isdir(PLUGIN_DIR):
                for f in os.listdir(PLUGIN_DIR):
                    if f.endswith(".plugin") or f.endswith(".wasm"):
                        version = "fixme"
                        files.append(dict(filename=f, size=os.path.getsize(os.path.join(PLUGIN_DIR, f)), sha1=get_sha1(os.path.join(PLUGIN_DIR, f)), version=version))
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(files).encode("utf-8"))

        elif parsed_path.path == "/web":
            # Serve the web directory
            web_path = os.path.join(WEB_BUILD_DIR, parsed_path.path[5:])
            if os.path.isdir(web_path):
                self.send_response(200)
                self.send_header("Content-type", "text/html")
                self.end_headers()
                with open(os.path.join(web_path, "index.html"), "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.send_error(404, "File not found")
        else:
            # Serve static files (including /plugins/<filename>)
            super().do_GET()


if __name__ == "__main__":
    # Change working directory to serve files correctly
    os.chdir(os.path.abspath(sys.argv[1]))

    # Ensure the plugins directory exists
    if not os.path.isdir(PLUGIN_DIR):
        os.makedirs(PLUGIN_DIR)
        print(f"Created plugins directory at {PLUGIN_DIR}")

    print(
        f"Starting server on port {PORT}, serving from {os.path.abspath(sys.argv[1])}"
    )
    print(f"Plugins directory: {os.path.abspath(PLUGIN_DIR)}")
    print(f"plugins: {[x for x in os.listdir(PLUGIN_DIR) if x.endswith('.plugin') or x.endswith('.wasm')]}")
    print("Press Ctrl+C to stop the server.")

    server_address = ("", PORT)
    httpd = http.server.HTTPServer(server_address, PluginServerHandler)
    print(f"Open http://localhost:{PORT}/web/index.html to access the server.")
    httpd.serve_forever()
