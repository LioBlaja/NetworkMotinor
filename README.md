# Network Monitor

## Detalii Proiect

- **Număr Studenți:** 2  (elaborat de 1 student)
- **Grad de Dificultate:** redus  

## Cerințe

1. Se va implementa un program cu interfață grafică realizată în **ncurses** pentru monitorizarea conexiunilor de rețea curente din sistem.
2. Pentru implementare, se vor utiliza informațiile disponibile în sistemul de fișiere **/proc**, în special:
   - **/proc/PID/net/netstat**
   - Programul utilitar **netstat**
3. Funcționalitățile principale ale programului:
   - Afișarea conexiunilor active:
     - **IP-ul** și **portul sursă**
     - **IP-ul** și **portul destinație**
     - **Protocolul** utilizat (TCP/UDP/ICMP)
   - Calcularea și afișarea **ratei de transfer** pentru fiecare conexiune.
4. Afișarea tuturor socket-urilor deschise din sistem care așteaptă conexiuni.

## Tehnologii Recomandate

- **C** cu biblioteca **ncurses**  
- Accesarea informațiilor din fișierele sistemului Linux

## Structură Proiect

- Monitorizarea conexiunilor în timp real
- Interfață simplă și intuitivă
- Optimizare pentru performanță redusă

## Scop

Acest proiect are ca scop dezvoltarea unei aplicații de monitorizare a conexiunilor de rețea, oferind o perspectivă detaliată și ușor de utilizat asupra activității sistemului.


# Network Monitor in C with ncurses and Multithreading

## **Overview**
This project implements a network monitor in **C**, using **ncurses** for interactive display and **multithreading** to efficiently collect and update network data in parallel. The program provides real-time information about:
- **Active network connections** (TCP, UDP, ICMP)
- **Listening sockets**
- **Traffic statistics** (bytes sent/received per second)

---

## **1. Data Collection and Multithreading**
The program utilizes **multiple threads** to perform data collection asynchronously, improving performance and responsiveness:
- **Thread 1**: Collects active network connections by parsing `netstat -anpl`.
- **Thread 2**: Retrieves socket information from `ss -l`.
- **Thread 3**: Monitors network traffic by reading `/proc/net/dev` and calculating transfer rates.
- **Thread 4**: Updates the ncurses interface at regular intervals.

Threads are managed using **pthread** to ensure concurrent execution, reducing blocking operations and keeping the interface responsive.

---

## **2. Data Structures**
To store the collected data efficiently, the program defines the following structures:

- **`Connection`** – Holds details about active connections (IP, port, protocol, state, PID, program).
- **`Socket`** – Stores information about sockets (protocol, type, state, inode, path).
- **`TrafficStats`** – Keeps track of network transfer speeds (bytes/sec for RX and TX).

---

## **3. Main Functions and Threads**

### **`void* getConnections(void* arg)`**
- Runs `netstat -anpl` and parses output to extract TCP/UDP/ICMP connections.
- Stores data in a shared `Connection[]` structure.

### **`void* getSocketsInfo(void* arg)`**
- Runs `ss -l` and parses output to identify listening sockets.
- Stores results in `Socket[]`.

### **`void* getNetworkStats(void* arg)`**
- Reads `/proc/net/dev` to obtain total bytes received and transmitted.
- Computes transfer rates over 5-second intervals.

### **`void* updateDisplay(void* arg)`**
- Uses **ncurses** to continuously refresh the display.
- Reads shared data from other threads to ensure real-time updates.

---

## **4. ncurses-Based User Interface**
The interface is designed to be interactive and user-friendly:
- **Header**: Displays column titles.
- **Scrollable View**: Shows real-time network statistics.
- **Footer**: Displays controls (e.g., press `'q'` to quit).
- Uses **wgetch()** for navigation with arrow keys.

---

## **5. Compilation and Execution**
### **Compiling the Program**
```sh
gcc -Wall -o monitor -fopenmp -pthread main.c -lncurses

### **Compiling the Program**
./monitor

### **To generate network traffic for testing:**
wget http://speedtest.tele2.net/1GB.zip &

## **5. Compilation and Execution**
This network monitor leverages multithreading to efficiently collect and update network data in parallel, preventing UI lag and ensuring real-time performance. By using ncurses, it provides an interactive, scrollable interface, making it a powerful tool for monitoring network activity in Linux environments.


