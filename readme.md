

https://github.com/user-attachments/assets/36d41ec4-1629-4e9b-99a3-79c8345cf8b5


# Packet Inspector

A lightweight cross platform network packet analyzer tool. Captures live network traffic, decodes protocol headers in real-time, provides an interactive hex viewer with byte level highlighting.

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)
![Qt](https://img.shields.io/badge/Qt-5.15-green)
![C++](https://img.shields.io/badge/C++-17-purple)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

##  Features

* **Live Packet Capture** 
* **3-Pane UI**:
  * **Top:** Scrollable packet list (No., Time, Source, Destination, Protocol, Length, Info).
  * **Middle:** Decoded protocol hierarchy tree (Ethernet → IPv4 → TCP/UDP).
  * **Bottom:** Custom virtual-scrolling hex viewer.
* **Interactive Byte Highlighting** — Click any field in the protocol tree to highlight the corresponding bytes in the hex dump.
* **Protocol Support**:
  * Layer 2: Ethernet II
  * Layer 3: IPv4
  * Layer 4: TCP, UDP, ICMP
* **PCAP Persistence** — Save captures to standard `.pcap` files.
* **Multithreaded Architecture** 

---

##  Flow

1. Launch the app → select a network interface from the dropdown.
2. Generate traffic (browse the web`).
3. Watch packets appear in real-time in the top pane.
4. Click any packet → see the decoded Ethernet/IPv4/TCP tree in the middle pane.
5. Click a field (e.g., "Src Port") → watch those exact bytes highlight yellow in the hex view.
6. `File → Save As...` → save to `capture.pcap`.
7. Open `capture.pcap` to verify compatibility.

---

##  Build Instructions

### Linux

```bash
# 1. Install Dependencies

# Ubuntu / Debian
sudo apt update
sudo apt install build-essential qtbase5-dev libpcap-dev pkg-config

# Fedora
sudo dnf install gcc-c++ make qt5-qtbase-devel libpcap-devel pkgconf-pkg-config

# Arch Linux
sudo pacman -S base-devel qt5-base libpcap pkgconf

# 2. Generate Makefile
qmake ../PacketInspector.pro   # or ~/Qt/5.15.2/gcc_64/bin/qmake if using custom Qt

# 3. Compile
make -j$(nproc)

# 4. Run with elevated privileges (required for promiscuous mode)
sudo ./PacketInspector

````powershell
# 1. Generate Makefile
qmake ..\PacketInspector.pro

# 2. Compile
mingw32-make -j8

# 3. Run application
.\release\PacketInspector.exe
