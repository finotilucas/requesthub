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

## Features

### Core Functionality

- **High-performance HTTP client** with connection pooling
- **Keep-alive connections** for lightning-fast consecutive requests
- **HTTP/2 support** with multiplexing
- **SSL/TLS verification** enabled by default
- **Multiple HTTP methods** (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
- **Custom headers** and bearer token authentication
- **Request body** support for POST/PUT/PATCH
- **Query parameter** encoding
- **Configurable timeouts** and redirects
- **Response metadata** (status, headers, timing)

## Getting Started

### Prerequisites

- **C Compiler**: GCC or Clang with C11 support
- **libcurl**: Version 7.64.0 or higher (with HTTP/2 support)
- **pthread**: For thread-safe pool operations
- **GTK 4**: For the UI
- **Make**: For building

#### Installing Dependencies

**Ubuntu/Debian:**

```sh
sudo apt-get install build-essential libcurl4-openssl-dev libgtk-4-dev
```

**Fedora/RHEL:**

```sh
sudo dnf install gcc libcurl-devel 	gtk4-devel
```

### Building

1. Clone the repository:

```sh
git clone https://github.com/FinotiLucas/requesthub-c
cd requesthub-c
```

2. Build the application:

```sh
make release
```

### Running

Run the application:

```sh
make run
```

_Tests performed on the same network conditions_

## Known Limitations

- No graphical user interface yet (library only)
- Pool size is fixed at compile time
- No built-in retry logic
- No automatic rate limiting
- Bearer tokens stored in plaintext memory

## Roadmap

- [ ] Add GUI using GTK4
- [ ] Implement request/response history
- [ ] Add request collections
- [ ] Environment variables support
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

### Testing

```sh
# Run with Valgrind to check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./build/release/requesthub

# Static analysis
cppcheck --enable=all src/
```

## 📄 License

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

**Lucas Finoti**

- Email: lucas.finoti@protonmail.com
- GitHub: [@FinotiLucas](https://github.com/FinotiLucas)

## Support

- [Report bugs](https://github.com/FinotiLucas/requesthub-c/issues)
- [Request features](https://github.com/FinotiLucas/requesthub-c/issues)
- [Discussions](https://github.com/FinotiLucas/requesthub-c/discussions)

---

<p align="center">
  Made with ❤️ and C by <a href="https://github.com/FinotiLucas">Lucas Finoti</a>
</p>

<p align="center">
  <i>If you find RequestHub useful, please consider giving it a ⭐ on GitHub!</i>
</p>
