# duino-coin-cpu

![Version](https://img.shields.io/badge/version-0.2beta-orange)
![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/platform-Linux-green)

---

## Project Structure
```text
.
├── CMakeLists.txt        # CMake configuration
├── b.sh                  # Build script
├── i.sh                  # Install dependencies
├── LICENSE
├── include/              # Header files
│   ├── benchmark.h
│   ├── config.h
│   ├── hasher.h
│   ├── http_client.h
│   ├── json.h
│   ├── logger.h
│   ├── miner.h
│   └── network.h
├── src/                  # Source code
│   ├── benchmark.cpp
│   ├── hasher.cpp
│   ├── http.cpp
│   ├── json.cpp
│   ├── logger.cpp
│   ├── main.cpp
│   ├── miner.cpp
│   └── network.cpp
└── build/                # Build output (ignored)


---

## Setup & Build

### Step 1: Install dependencies

```bash
bash i.sh
```

### Step 2: Build project

```bash
bash b.sh
```

After building, go to:

```bash
cd build
```

You will see the binary:

```text
duino-cpu
```

---

## Usage

```bash
./duino-cpu -help
```

```text
-u, --user <username>       Duino-Coin username (required)
-k, --key <mining_key>      Mining key (optional)
-t, --threads <number>      Number of threads (default: auto)
-i, --intensity <1-100>     Mining intensity (default: 95)
-d, --difficulty <type>     LOW, MEDIUM, NET (default: NET)
-r, --rig <identifier>      Rig identifier
-p, --pool <host:port>      Custom pool
-b, --benchmark             Run benchmark and exit
--invisible                 Hide process from htop/btop
--nolog                     Disable console logging
-h, --help                  Show help
```

### Example

```bash
./duino-cpu -u your_username -k abc -t 4
```

---

## Demo

![Demo 1](https://github.com/Mytai20100/duino-coin-cpu/raw/main/demo1.png)
![Demo 2](https://github.com/Mytai20100/duino-coin-cpu/raw/main/demo2.png)

---

## Credit

Based on the **Duino-Coin** project
 [https://github.com/duino-coin/duino-coin](https://github.com/duino-coin/duino-coin)
