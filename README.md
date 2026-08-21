# RequestHub (v0.1.1)

A native HTTP client for Linux, written in C against GTK 4 and libcurl.
RequestHub provides the request/response workflow familiar from tools like
Postman or Insomnia without the JavaScript runtime, packaged as a small
native binary that integrates with the host desktop.

## Status

RequestHub is pre-1.0. The on-disk history format and the public command-line
surface are subject to change between releases until the first tagged
version. See [Roadmap](#roadmap) for tracked work.

## Overview

The implementation is built around a few explicit choices:

- A long-lived libcurl connection pool keeps TCP, TLS and HTTP/2 state hot
  across requests issued in the same session.
- The UI is built on native GTK 4 widgets with libadwaita for the
  adaptive shell (`AdwOverlaySplitView` sidebar, `AdwBreakpoint`-driven
  responsive layout, `AdwViewStack` tabs), and GtkSourceView 5 for
  syntax-highlighted request/response editing.
- Persistent state is written under `$XDG_DATA_HOME/requesthub/` in
  human-readable JSON, with conservative file permissions and explicit
  retention bounds.

Supported platforms: Linux on `x86_64`. Both X11 and Wayland sessions are
supported through GTK.

## Features

- HTTP/1.1 and HTTP/2 with multiplexing and keep-alive reuse
- `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, `OPTIONS`
- Custom request headers and percent-encoded query parameters
- Request bodies with syntax-highlighted editing and live validation for
  JSON, XML and YAML
- TLS verification, sane timeouts and redirect following on by default
- Per-request response metadata (status, timing, transferred bytes)
- Local request history with one-click replay, deduplication and per-entry
  deletion

## Build dependencies

All dependencies are resolved through `pkg-config`. RequestHub requires:

| Component       | Minimum | Notes                                  |
| --------------- | ------- | -------------------------------------- |
| C toolchain     | C11     | `zig cc` when available, gcc otherwise |
| GTK             | 4.0     | required                               |
| libadwaita      | 1.4     | adaptive shell                         |
| GtkSourceView   | 5.0     | required                               |
| GLib            | 2.76    | used transitively                      |
| libcurl         | 7.78    | HTTP/2 must be enabled at build time   |
| libxml2         | 2.9     | XML response parsing                   |
| libyaml         | 0.2     | YAML response parsing                  |
| cJSON           | 1.7     | JSON parsing                           |

Examples of installing the development packages:

```sh
# Debian / Ubuntu
sudo apt install build-essential pkg-config libgtk-4-dev libadwaita-1-dev \
            libgtksourceview-5-dev libcurl4-openssl-dev libxml2-dev \
            libyaml-dev libcjson-dev

# Arch Linux
sudo pacman -S base-devel gtk4 libadwaita gtksourceview5 curl libxml2 libyaml cjson

# Fedora
sudo dnf install gcc make pkgconf-pkg-config gtk4-devel libadwaita-devel \
            gtksourceview5-devel libcurl-devel libxml2-devel libyaml-devel \
            cjson-devel

# Void Linux
sudo xbps-install -S base-devel pkg-config gtk4-devel libadwaita-devel \
                gtksourceview5-devel libcurl-devel libxml2-devel libyaml-devel \
                cjson-devel
```

You can verify that the toolchain sees every dependency with:

```sh
make deps-check
make info          # show which compilers and flags the build resolved
```

## Building from source

```sh
make                # debug build at build/requesthub
make release        # optimised build at build/release/requesthub
make test           # GLib-based unit tests
make install        # PREFIX=/usr/local by default; DESTDIR supported
make clean          # remove obj/ and build/
```

Release builds default to `zig cc` when zig is installed and fall back to
gcc otherwise. Debug builds and tests are compiled by `SAN_CC` (clang or
gcc) with AddressSanitizer and UndefinedBehaviorSanitizer, because zig does
not ship the ASan runtime. Both are plain make variables:

```sh
make CC=gcc SAN_CC=gcc              # force a single toolchain
make release ARCH_FLAGS='-march=x86-64-v3'   # opt-in CPU baseline
```

`make run` and `make run-release` build and launch the corresponding
binary. `make run` disables the LeakSanitizer end-of-process report —
fontconfig and dbus keep process-lifetime caches that trip it in every GTK
app; leak checking lives in `make test` and `make valgrind`.

### AppImage

A self-contained, distributable AppImage can be produced with:

```sh
make appimage                       # build/RequestHub-<version>-x86_64.AppImage
make appimage VERSION=0.1.1         # override the version string
```
 
The version is otherwise derived from `git describe --tags --always --dirty`.

The bundled binary targets the compiler baseline (portable x86-64) by
default; pass `ARCH_FLAGS='-march=x86-64-v3'` to trade portability for a
newer CPU baseline.

The packaging entry point is `packaging/appimage/build-appimage.sh`. It
fetches `linuxdeploy` and its GTK plugin into `build/appimage-tools/` on
first run and reuses them on subsequent builds.

### Flatpak

A Flatpak manifest targeting the GNOME 49 runtime lives in
`packaging/flatpak/`. GTK 4, libadwaita, GtkSourceView, libcurl and libxml2
come from the runtime; cJSON and libyaml are built as modules. Build and
install it with flatpak-builder — or its Flathub-packaged form, which
requires no root:

```sh
flatpak install flathub org.flatpak.Builder
flatpak run org.flatpak.Builder --user --install-deps-from=flathub \
    --disable-rofiles-fuse --force-clean --install \
    build/flatpak-builddir packaging/flatpak/io.github.finotilucas.requesthub.yml
flatpak run io.github.finotilucas.requesthub
```

## Continuous integration and releases

Every push builds and tests on Arch, Fedora and Ubuntu (gcc) and on macOS
via Homebrew (Apple clang). Pushing a `v*` tag additionally builds the
AppImage and publishes it to a GitHub Release with generated notes:

```sh
git tag v0.2.0 && git push origin v0.2.0
```

The macOS artifact is a bare binary used for build validation; Linux is
the supported desktop target.

## Configuration and data

RequestHub follows the XDG Base Directory Specification:

| Path                                       | Purpose                              |
| ------------------------------------------ | ------------------------------------ |
| `$XDG_DATA_HOME/requesthub/history.json`   | Request history (file mode `0600`)   |

The history file is written atomically. It is capped at 200 entries with a
512 KB upper bound per cached response body; the oldest entry is evicted
when the bound is reached. The `Authorization` header is stripped before
persistence, and binary or non-UTF-8 response bodies are not cached.

## Security considerations

- Credentials typed into headers are held as plaintext in process memory
  for the lifetime of the request editor. There is no `mlock`/secure-memory
  layer.
- The on-disk history file is mode `0600` but is **not encrypted at rest**.
  Cookies, custom authentication headers (`X-API-Key`, etc.) and request
  bodies are persisted as written; only `Authorization` is filtered.
- `CURLOPT_SSL_VERIFYPEER` and `CURLOPT_SSL_VERIFYHOST` are enabled by
  default and are not exposed as a UI toggle yet.

History encryption via the system keyring (`libsecret`) and configurable
redaction rules are tracked in the roadmap.

## Roadmap

Tracked, in no particular order:

- Request collections and per-environment variable resolution
- History encryption via the system keyring (libsecret)
- Configurable redaction rules for sensitive headers and body patterns
- Multi-handle asynchronous requests (`CURLM`)
- Automatic retry with exponential backoff
- Response caching that respects `Cache-Control`
- WebSocket support
- GraphQL support
- HTTP/3 (QUIC)

## Contributing

Bug reports and patches are welcome through the GitHub issue tracker and
pull-request workflow. Before sending a non-trivial change, please open an
issue to discuss the proposed direction.

When sending a patch:

- Match the existing code style: 2-space indentation, `snake_case` for
  functions and variables, prefixed by module name (`http_pool_*`,
  `history_*`, …).
- Add or update unit tests under `tests/` where applicable. Tests use the
  GLib test framework and run in isolated XDG directories via
  `G_TEST_OPTION_ISOLATE_DIRS`.
- Verify the change is leak-free with `make valgrind` for debug builds.
- Keep commits focused and reference the relevant issue in the message.

## License

RequestHub is distributed under the GNU General Public License, version 2
or later. The full text is available in [`LICENSE`](LICENSE) and at
<https://spdx.org/licenses/GPL-2.0-or-later.html>.

```
SPDX-License-Identifier: GPL-2.0-or-later
Copyright (C) 2026 Lucas Finoti <lucas.finoti@protonmail.com>
```

## Resources

- Source repository: <https://github.com/finotilucas/requesthub>
- Issue tracker:     <https://github.com/finotilucas/requesthub/issues>
- Discussions:       <https://github.com/finotilucas/requesthub/discussions>
