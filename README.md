# Movesense Offline Configurator

This is a tool to configure Movesense sensor devices equipped with a custom offline tracking firmware. Its intended use case is testing and validation of the firmware.

## Features

- Read and write offline mode configuration
  - Measurements to record on-device
  - Wake up and sleep conditions
  - Options to choose enable optional features
- Listing and downloading logs

## Related Projects

- [Movesense Offline Tracking](https://github.com/niko-j/movesense-offline-tracking) project contains the offline tracking firmware.
- [Movesense Offline SBEM Decoder](https://github.com/niko-j/movesense-offline-sbem-decoder) project contains a utility to read the log files and export measurements into CSV files.

## Compiling

Make sure you have the firmware repository cloned into the same directory as this project (the protocol directory is symbolically linked), or copy over the `src/protocol` from the firmware into the root of the repository.

Either open the project in Qt Creator and build it or use CMake as in the following example.

```sh
mkdir build
cd build
cmake ..
make
```
