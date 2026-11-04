import argparse
import functools
import http.server
import pathlib


class IsolatedRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description="以跨源隔离模式提供 Qt WebAssembly 构建产物")
    parser.add_argument("directory", type=pathlib.Path, help="包含 .html、.js、.wasm 的目录")
    parser.add_argument("--port", type=int, default=8000, help="监听端口")
    parser.add_argument("--bind", default="127.0.0.1", help="监听地址")
    args = parser.parse_args()
    directory = args.directory.resolve()
    if not directory.is_dir():
        parser.error(f"目录不存在: {directory}")
    handler = functools.partial(IsolatedRequestHandler, directory=str(directory))
    with http.server.ThreadingHTTPServer((args.bind, args.port), handler) as server:
        print(f"已启动: http://{args.bind}:{args.port}/ShadcnWidgetToolsExample.html，按 Ctrl+C 停止")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\n已停止")


if __name__ == "__main__":
    main()
