# FIX_parser

A low-latency FIX protocol decoder written in C++20. It connects to a TCP feed
as a client, parses FIX 4.4 session/application messages from the stream, and
either measures pure in-memory parse performance or emits structured
JSON-lines records per message.

Performance-driven design: the parser scans fields with a single pass over the
buffer, uses `memchr` to jump to the next `SOH`, and keeps allocation off the
hot path. The measurement and logging paths are separate loops so that logging
machinery never interrupts the measured decode time.

---

## Requirements

| Tool       | Version / Notes                                        |
|------------|--------------------------------------------------------|
| CMake      | >= 3.16                                                |
| C++ compiler | C++20 (GCC, Clang)                                   |
| Python 3   | only for `src/replay.py` (PCAP replay testing)         |
| scapy      | `pip install scapy` — only for `src/replay.py`         |

Linux is the target platform (the client uses POSIX sockets / `poll`).

## Build

### Quick build (script)

```sh
./scripts/build.sh
```

This is equivalent to:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is produced at `build/FIX_parser`. Release builds compile with `-O3`
(plus `-DNDEBUG`) and are stripped when installed.

### Clean rebuild

```sh
rm -rf build
./scripts/build.sh
```

### Install

```sh
cmake --install build --prefix /usr/local   # installs bin/FIX_parser
```

## Usage

```
FIX_parser [host] [port] [--log FILE]

  host        FIX feed host    (default 127.0.0.1)
  port        FIX feed port    (default 8080)
  --log FILE  write structured JSON-lines log to FILE
```

Two run modes:

- **Measurement mode (default).** Parses the stream in memory and prints
  statistics to stderr at the end only, no per-message output, no log I/O on
  the hot path:

  ```
  Messages parsed:   640
  Total parse time:  214042 ns (0.214 ms)
  Avg parse time:    334.4 ns/msg
  Throughput:        2990078 msg/s
  ```

- **Logging mode (`--log FILE`).** Same statistics on stderr, plus one JSON
  record per complete message:

  ```json
  {"msgtype":"A","name":"Logon","fields":{"BeginString":"FIXT.1.1","BodyLength":"77","SenderCompID":"DLD_TEX",...},"raw":"8=FIXT.1.1|9=77|35=A|49=DLD_TEX|..."}
  ```

Press `Ctrl-C` (SIGINT) at any time for a clean shutdown with statistics so far.

## Testing against the bundled PCAP

`tests/fix.pcap` contains plaintext FIX traffic. `src/replay.py` acts as the
**server** that replays the TCP payloads; the parser is the **client** that
connects to it, so start the server first.

Start the replay server in one terminal:

```sh
python3 src/replay.py tests/fix.pcap
```

Then run the parser in another:

```sh
./build/FIX_parser 127.0.0.1 8080                 # measurement mode
./build/FIX_parser 127.0.0.1 8080 --log fix.jsonl # logging mode
```

On a single terminal:

```sh
python3 src/replay.py tests/fix.pcap & sleep 1 && ./build/FIX_parser 127.0.0.1 8080; wait
```

`tests/fix.pcap` contains 640 messages (`wc -l fix.jsonl` should report 640).
Do **not** use `tests/fix-ssl.pcap` for this test, it is TLS-encrypted, so the
parser will not decode any FIX messages.

## Benchmarking and a note on CPU frequency

`scripts/bench.sh` repeats the measurement-mode run and reports min / median /
max parse time together with the CPU governor and current clock:

```sh
scripts/bench.sh [pcap] [iterations]
```

**Important:** these results are extremely sensitive to CPU clock speed. This
project was measured on an `Intel i7-9750H` (2.6 GHz base / 4.5 GHz turbo).

| CPU state                                 | Total parse time (640 msgs) |
|-------------------------------------------|-----------------------------|
| `performance` governor (turbo ~4.5 GHz)    | **~0.2 ms** (< 350 ns/msg)  |
| `powersave` governor (clock ~0.9 GHz)      | ~1.0–1.8 ms                 |

While on the `powersave` governor the clock sits near its floor (~0.9 GHz on
this machine), which makes the *same binary* appear ~4–5x slower — the
"regression" from 0.22 ms to ~1 ms was entirely frequency scaling. Restoring the `performance` governor returned the original ~0.2 ms:

```sh
sudo cpupower frequency-set -g performance
```

For reproducible numbers, set the governor to `performance` and/or pin to a
fixed core (`taskset -c 2 scripts/bench.sh`).

## Project layout

```
CMakeLists.txt
include/
  decoder_versions/           decoder loop implementation
    decoder.hpp               * split measurement + logging loops
    decoder_v1+v2.h           older reference implementation
  output_components/          FIX 4.4 tag/message tables
    AllMessageTypes.h         * msg type 35 codes -> names
    4.4 Tags.h                * tag id -> FIX name (unordered_map)
    44tag_array.hpp           tag name array (alternate representation)
scripts/
  build.sh                    configure + build Release
  bench.sh                    repeated measurement benchmark
src/
  main.cpp                    TCP client + CLI + statistics
  replay.py                   PCAP TCP-payload replay server
tests/
  fix.pcap                    plaintext FIX capture (640 messages)
  fix-ssl.pcap                TLS-encrypted capture (not for parsing)
```
