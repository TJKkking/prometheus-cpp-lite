#!/usr/bin/env python3
"""
server_side_for_http_push.py — Test server for prometheus-cpp-lite HTTP push examples.

Accepts POST, PUT, and DELETE requests, prints their details to the console,
and returns a 200 OK response.  Designed to work with:

  - provide_via_http_push_simple.cpp   (simple periodic POST)
  - provide_via_http_push_advanced.cpp (POST, PUT, DELETE, async)

Usage:
  python server_side_for_http_push.py [port]

Default port is 9091.  Press Ctrl+C to stop.
"""

import os
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime


# =============================================================================
# ANSI color support
# =============================================================================

def _enable_ansi_colors():
    """
    Detect whether the terminal supports ANSI escape codes.
    On Windows 10+ (build 14393+), enable Virtual Terminal Processing
    via the Win32 API so that ANSI codes work in cmd.exe and PowerShell.
    Returns True if ANSI colors are available, False otherwise.
    """
    # If output is redirected to a file/pipe, disable colors.
    if not sys.stdout.isatty():
        return False

    # Unix terminals generally support ANSI natively.
    if os.name != "nt":
        return True

    # Windows: try to enable VT100 processing on the console handle.
    try:
        import ctypes
        from ctypes import wintypes

        kernel32 = ctypes.windll.kernel32

        STD_OUTPUT_HANDLE = -11
        ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004

        handle = kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
        if handle == -1:
            return False

        mode = wintypes.DWORD()
        if not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            return False

        if not (mode.value & ENABLE_VIRTUAL_TERMINAL_PROCESSING):
            new_mode = mode.value | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            if not kernel32.SetConsoleMode(handle, new_mode):
                return False

        return True
    except Exception:
        return False


_COLORS_ENABLED = _enable_ansi_colors()


def _c(code):
    """Return the ANSI escape code if colors are enabled, empty string otherwise."""
    return code if _COLORS_ENABLED else ""


# Color constants (resolve once at import time).
COLOR_RESET  = _c("\033[0m")
COLOR_GREEN  = _c("\033[32m")
COLOR_YELLOW = _c("\033[33m")
COLOR_RED    = _c("\033[31m")
COLOR_CYAN   = _c("\033[36m")
COLOR_DIM    = _c("\033[2m")

METHOD_COLORS = {
    "POST":   COLOR_GREEN,
    "PUT":    COLOR_YELLOW,
    "DELETE": COLOR_RED,
}


# =============================================================================
# HTTP handler
# =============================================================================

class PushgatewayHandler(BaseHTTPRequestHandler):
    """Handles POST, PUT, and DELETE requests, mimicking a Prometheus Pushgateway."""

    def _handle_request(self, method):
        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length).decode("utf-8", errors="replace") if content_length > 0 else ""

        color = METHOD_COLORS.get(method, COLOR_CYAN)
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

        print(f"\n{COLOR_DIM}[{timestamp}]{COLOR_RESET} "
              f"{color}{method}{COLOR_RESET} {self.path}")
        print(f"  Content-Length: {content_length}")

        if body:
            print(f"  Body:")
            for line in body.splitlines():
                if line.startswith("#"):
                    print(f"    {COLOR_DIM}{line}{COLOR_RESET}")
                else:
                    print(f"    {COLOR_CYAN}{line}{COLOR_RESET}")
        else:
            print(f"  {COLOR_DIM}(empty body){COLOR_RESET}")

        separator = "-" * 60
        print(f"  {COLOR_DIM}{separator}{COLOR_RESET}")

        # Send 200 OK response.
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(b"OK")

    def do_POST(self):
        self._handle_request("POST")

    def do_PUT(self):
        self._handle_request("PUT")

    def do_DELETE(self):
        self._handle_request("DELETE")

    def log_message(self, format, *args):
        """Suppress default access log — we print our own formatted output."""
        pass


# =============================================================================
# Main
# =============================================================================

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9091

    server = HTTPServer(("localhost", port), PushgatewayHandler)

    banner_line = "=" * 60
    print(banner_line)
    print(f"  Prometheus Pushgateway test server")
    print(f"  Listening on http://localhost:{port}")
    print(f"  Accepts {COLOR_GREEN}POST{COLOR_RESET}, "
          f"{COLOR_YELLOW}PUT{COLOR_RESET}, "
          f"{COLOR_RED}DELETE{COLOR_RESET} requests")
    print(f"  Press Ctrl+C to stop")
    print(banner_line)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print(f"\n{COLOR_DIM}Server stopped.{COLOR_RESET}")
        server.server_close()


if __name__ == "__main__":
    main()