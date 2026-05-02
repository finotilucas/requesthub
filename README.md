# RequestHub

[![License: GPL v2+](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](https://www.gnu.org/licenses/gpl-2.0)
[![C Standard](https://img.shields.io/badge/C-C11-blue.svg)](<https://en.wikipedia.org/wiki/C11_(C_standard_revision)>)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

A lightning-fast, native HTTP client written entirely in C. RequestHub provides a lightweight alternative to Electron-based API clients like Postman and Insomnia, delivering exceptional performance.

## Performance

RequestHub leverages advanced connection pooling and HTTP/2 to achieve remarkable speed:

- **First request:** ~495ms (cold start with DNS, TCP, SSL handshake)
- **Subsequent requests:** ~12ms (41x faster with connection reuse)
- **3x faster** than Insomnia for consecutive requests
- **50% faster** than Postman for consecutive requests
- **Zero overhead** - no JavaScript runtime, no Electron bloat

_Tests performed on the same network conditions_

## Features

### Core Functionality

- **High-performance HTTP client**
- **Keep-alive connections** for lightning-fast consecutive requests
- **HTTP/2 support** with multiplexing
- **SSL/TLS verification** enabled by default
- **Multiple HTTP methods** (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
- **Custom headers** and bearer token authentication
- **Request body** support for POST/PUT/PATCH
- **Query parameter** encoding
- **Configurable timeouts** and redirects
- **Response metadata** (status, headers, timing)

### Request History

- **Local-first sidebar** showing recent requests with method, URL, status, duration, size and relative timestamp
- **One-click replay**: click an entry to restore method, URL, body, headers and query params into the editor and re-render the cached response
- **Smart deduplication**: re-running the same URL+method updates the existing entry and bumps it to the top instead of cluttering the list
- **Per-entry deletion** plus full-history clear from the sidebar header
- **Persistence** at `~/.local/share/requesthub/history.json` (XDG data dir) with directory mode `0700` and file mode `0600`, written atomically
- **Bounded** at 200 entries (oldest evicted automatically) and capped at 512 KB per cached response body
- **Authorization headers are stripped** before persistence; binary / non-UTF-8 response bodies are not cached

## Known Limitations

- Pool size is fixed at compile time
- No built-in retry logic
- No automatic rate limiting
- Bearer tokens stored in plaintext memory
- Request and cached response history is stored in plaintext JSON on disk (file is `0600`, but no encryption-at-rest). Cookies, custom auth headers (`X-API-Key`, etc.) and login/response bodies are persisted as-is — only `Authorization` is filtered. A future release may use `libsecret` to encrypt the history file.

## Roadmap

- [x] Implement request/response history
- [ ] Add request collections
- [ ] Environment variables support
- [ ] Encrypt history file via system keyring (libsecret)
- [ ] Configurable redaction rules for sensitive headers/body patterns
- [ ] WebSocket support
- [ ] GraphQL support
- [ ] Request/response interceptors
- [ ] Automatic retry with exponential backoff
- [ ] Response caching with Cache-Control respect
- [ ] HTTP/3 (QUIC) support
- [ ] Multi-handle async requests (CURLM)

## Contributing

Contributions are welcome!
Whether it's bug fixes, new features, or documentation improvements.

### How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow the existing code style
- Add tests for new features
- Update documentation as needed
- Ensure no memory leaks (verify with `valgrind`)
- Run static analysis before submitting

### Tests

Unit tests for the `history` module (entry lifecycle, store operations, JSON round-trip) live under `tests/` and run via the GLib test framework. Each test runs in an isolated XDG directory tree (`G_TEST_OPTION_ISOLATE_DIRS`), so they never touch your real history file.

```bash
make test
```

## License

RequestHub is licensed under the **GNU General Public License v2.0 or later**.

```
Copyright (C) 2026 Lucas Finoti <lucas.finoti@protonmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
```

[Full License Text](https://spdx.org/licenses/GPL-2.0-or-later.html)

## Acknowledgments

- [libcurl](https://curl.se/libcurl/) - The foundation of RequestHub's HTTP functionality
- [gtk](https://www.gtk.org/) - By offering a complete set of UI elements.

## Contact

## Support

- [Report bugs](https://github.com/FinotiLucas/requesthub-c/issues)
- [Request features](https://github.com/FinotiLucas/requesthub-c/issues)
- [Discussions](https://github.com/FinotiLucas/requesthub-c/discussions)

---

<p align="center">
  Made by <a href="https://github.com/FinotiLucas">Lucas Finoti</a>
</p>

<p align="center">
  If you find RequestHub useful, please consider giving it a ⭐ on GitHub!
</p>
