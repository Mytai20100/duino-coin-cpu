# duino-coin-cpu

![Version](https://img.shields.io/badge/version-0.4-orange)
![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/platform-Linux-green)

---

## Project Structure
```text
.
├── CMakeLists.txt        # CMake configuration
├── b.sh                  # Build script
├── i.sh                  # Install dependencies
├── ChangeLog             # Version history
├── LICENSE
├── README.md
├── include/              # Header files
│   ├── benchmark.h
│   ├── config.h
│   ├── config_yaml.h
│   ├── hasher.h
│   ├── http_client.h
│   ├── json.h
│   ├── logger.h
│   ├── miner.h
│   ├── network.h
│   └── stats.h
├── src/                  # Source code
│   ├── benchmark.cpp
│   ├── config_yaml.cpp
│   ├── hasher.cpp
│   ├── http.cpp
│   ├── json.cpp
│   ├── logger.cpp
│   ├── main.cpp
│   ├── miner.cpp
│   ├── network.cpp
│   └── stats.cpp
├── img/                  # img
│   ├── demo1.png
│   └── demo2.png
└── build/                # Build output (ignored)
```

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
-c, --config <file.yml>     Load configuration from YAML file
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

### Using config file

```bash
./duino-cpu -c config.yml
```

On first run without arguments, a default `config.yml` will be created.

---

## Demo

![Demo 1](https://raw.githubusercontent.com/Mytai20100/duino-coin-cpu/refs/heads/main/img/demo1.png)
![Demo 2](https://raw.githubusercontent.com/Mytai20100/duino-coin-cpu/refs/heads/main/img/demo2.png)

---

## Credit

Based on the **Duino-Coin** project
 [https://github.com/duino-coin/duino-coin](https://github.com/duino-coin/duino-coin)