# 🚂 Multithreaded Railway Server

A robust, distributed **Client-Server application** built in **C++** that simulates a real-time railway station management system. It allows multiple users to check schedules, view live departures, and report delays that are instantly synchronized across all connected clients.

The project demonstrates advanced systems programming concepts: **TCP/IP Sockets**, **Multithreading** (Producer-Consumer), **Synchronization** (Mutexes/Condition Variables), and **Design Patterns**.

---

## 🚀 Key Features

* **Concurrent Architecture:** Uses a **Thread-per-Client** model to handle multiple connections simultaneously without blocking.
* **Producer-Consumer Pattern:** Decouples network I/O from logic processing using a thread-safe `CommandQueue`. A dedicated **Worker Thread** processes requests efficiently.
* **Thread Safety:** Critical sections (shared database access) are protected using `std::mutex` to prevent race conditions.
* **Cross-Platform Client:** The client runs natively on **Linux** and includes support for **Windows** (via Winsock).
* **Data Persistence:** Train schedules and delays are stored in **XML files** (parsed with `tinyxml2`), ensuring data survives server restarts.
* **Custom Protocol:** Implements a length-prefixed application protocol to handle TCP stream fragmentation.

---

## 🛠️ Tech Stack

* **Language:** C++17
* **Networking:** BSD Sockets (Linux), Winsock2 (Windows)
* **Build System:** Make
* **Concurrency:** `std::thread`, `std::mutex`, `std::condition_variable`
* **Data Format:** XML (TinyXML-2)
* **OS:** Linux (Server/Client), Windows (Client only)

---

## 📂 Project Structure

- **server.cpp**: Main Server entry point
- **client.cpp**: Main Client entry point
- **Makefile**: Automated build script
- **TrainManager/**: Database Logic & XML handling
- **Commands/**: Command Pattern Implementation
- **xml_parser/**: External library (TinyXML-2)
- **TrainSchedule/**: Database Files (`schedule_org.xml`, `schedule_mod.xml`)
- **README.md**: Documentation

---

## 📥 Getting Started

Follow these steps to set up the project on your local machine.

### Prerequisites
* **Git**
* **G++ Compiler** (supporting C++17)
* **Make**

### 1. Clone the Repository
Open your terminal and run the following commands to download the code:

git clone https://github.com/cataCatau/Multithreaded-Train-Server.git
cd Multithreaded-Train-Server

### 2. Build the Project (Linux)
Use the included Makefile to compile both the Server and the Client automatically.

Run command: `make`

To clean up build files (executables and objects), run: `make clean`

### 3. Cross-Compile Client for Windows (Optional)
To generate a .exe client for Windows users while working on Linux:

Run command: `x86_64-w64-mingw32-g++ client.cpp -o client_windows.exe -lws2_32 -static`

---

## 🚦 Running the Application

**Step 1:** Start the Server.
Run: `./server`
*(The server listens on port 54000. Ensure orarorg.xml exists in the OrarTrenuri folder.)*

**Step 2:** Start the Client (in a new terminal).
Run: `./client`
*(You can open multiple terminals and run ./client to simulate concurrent users).*

---

## 📋 Available Commands

Once connected, use the following commands:

| Command | Description | Example |
| :--- | :--- | :--- |
| `GET_SCHEDULE <From> <To>` | Finds trains between two cities. | `GET_SCHEDULE Iasi Bucuresti` |
| `GET_DEPARTURES [Station]` | Lists trains leaving in the next hour. | `GET_DEPARTURES` |
| `GET_ARRIVALS [Station]` | Lists trains arriving in the next hour. | `GET_ARRIVALS Roman` |
| `REPORT_DELAY <ID> <Min> <Est>` | Reports a delay for a train ID. | `REPORT_DELAY 1661 10 Delayed` |
| `GET_TRAIN_INFO <ID>` | Shows full route details for a train. | `GET_TRAIN_INFO 1661` |
| `help` | Displays the list of commands. | `help` |
| `exit` | Disconnects from the server. | `exit` |

---

## 🧠 System Architecture

1.  **Network Layer:** The server accepts connections and spawns a `handleClient` thread for each user.
2.  **Protocol Parsing:** Raw TCP bytes are reassembled into strings using a custom length-header protocol to ensure complete message delivery.
3.  **Command Pattern:** The string is parsed into a concrete `Command` object (e.g., `GetScheduleCommand`) and pushed into the `CommandQueue`.
4.  **Worker Thread:** A separate thread (Consumer) wakes up when the queue is not empty, pops the command, and executes it against the `TrainManager`.
5.  **Synchronization:** Since multiple commands might try to write to the XML database simultaneously (e.g., reporting delays), a `std::mutex` ensures only one thread modifies the data at a time.

---

## 📜 License

This project is open-source and available for educational purposes.