# RequestHub

RequestHub is a desktop application written entirely in C for crafting, sending, and inspecting HTTP requests.
It aims to provide a lightweight and native alternative to tools like Postman and Insomnia, with a focus on performance, reliability, and a clear user interface.

RequestHub is built using C and includes features for composing requests with custom methods, headers, bodies, and viewing responses.

## Motivation

Existing API clients are often built on platforms like Electron or require external runtimes, which can increase resource usage and complexity.
RequestHub demonstrates how a native C application with a modern GUI can deliver a responsive and lightweight experience for developers who need to test and explore APIs without relying on cloud services or heavy runtimes.

## Getting Started

### Building

Clone the repository:

```sh
  git clone https://github.com/FinotiLucas/requesthub-c
  cd requesthub-c
```

### Build the application:

```sh
  make release
```

### Running

Run the app locally:

```sh
  make run:release
```

Built releases can be found under build/release.

## Contributing

Contributions are welcome. If you plan to add new features or fix issues, please open an issue first to discuss your approach. Ensure all changes include appropriate tests.

### License

RequestHub is licensed under the GPL-2.0-or-later.

Copyright (c) 2026 Lucas Finoti

[See more about the license][license]

[license]: https://github.com/FinotiLucas/Correios-Brasil/blob/master/LICENSE
