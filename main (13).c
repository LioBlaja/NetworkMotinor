#include <stdio.h>
#include <omp.h>
#include <ncurses.h>
#include "simpletables.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>   
#include <netinet/in.h>  
#include <ctype.h>
#include <unistd.h>      

#include <locale.h>   // for setlocale
#include <ncurses.h>  // for ncurses
#include <sys/time.h>

#define INTERFACE "eth0"  
#define BUFFER_SIZE 256

// compile and link: gcc <program file> -lncurses
// gcc -Wall -o output -fopenmp main.c -lncurses
//initiate test:
//  wget http://speedtest.tele2.net/1GB.zip &
#define INET_ADDRSTRLEN 16
#define PATH_MAX 4096  // Maximum length for socket paths

typedef struct {
    char source_ip[INET_ADDRSTRLEN];    // Source IP address as a string
    int source_port;                    // Source port
    char dest_ip[INET_ADDRSTRLEN];      // Destination IP address as a string
    int dest_port;                      // Destination port
    char protocol[10];                  // Protocol used (TCP/UDP/ICMP)
    char state[32];                     // Connection state
    int pid;                            // Process ID
    char program[256];                  // Program name
} Connection;


typedef struct {
    char protocol[10];      // Protocol type (e.g., "unix")
    int refCount;           // Reference count
    char flags[10];         // Flags (e.g., "[ ACC ]")
    char type[10];          // Socket type (e.g., "STREAM", "SEQPACKET")
    char state[15];         // State (e.g., "LISTENING")
    int inode;              // I-Node number
    char path[PATH_MAX];    // Path to the socket
} Socket;

typedef struct{
    long rxRate; // Receive rate (bytes/sec)
    long txRate; // Transmit rate (bytes/sec)
} TrafficStats;



void printSockets(Socket* socketsInfo, int socketsCount) {
    printf("READ SOCKET DATA:\n");
    for(int i = 0;i < socketsCount;i++){
        printf("Protocol: %s\t", socketsInfo[i].protocol);
        printf("Ref Count: %d\t", socketsInfo[i].refCount);
        printf("Flags: %s\t", socketsInfo[i].flags);
        printf("Type: %s\t", socketsInfo[i].type);
        printf("State: %s\t", socketsInfo[i].state);
        printf("I-Node: %d\t", socketsInfo[i].inode);
        printf("Path: %s\t\n", socketsInfo[i].path);
    }
}

void printConnections(Connection* connections, int connectionsCount){
    printf("READ CONNECTIONS DATA:\n");
    for (int i = 0;i < connectionsCount; i++) {
        printf("Connection %d: %s:%d -> %s:%d (%s)\n", i + 1,
               connections[i].source_ip, connections[i].source_port,
               connections[i].dest_ip, connections[i].dest_port,
               connections[i].protocol);
    }
}

void printWithHiddenChars(const char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        if (isprint(line[i])) {
            printf("'%c' ", line[i]); // Afișăm caracterul dacă este printabil
        } else {
            printf("\\x%02X ", (unsigned char)line[i]); // Afișăm codul hexazecimal pentru caracterele ascunse
        }
    }
    printf("\n");
}

void getConnections(Connection **connections, int *connectionCount) {
    FILE *fp;
    char line[1024];

    // Deschidem fluxul netstat

    //     Argumente
    // -a: Afișează toate conexiunile și porturile de ascultare

    // Listează atât conexiunile active (stabilite, în așteptare etc.), cât și porturile pe care sistemul ascultă pentru noi conexiuni (listening).
    // -n: Afișează adresele și porturile în format numeric

    // În loc să traducă adresele IP în nume de gazdă și porturile în servicii (cum ar fi 80 în http), afișează adresele și porturile exact așa cum sunt (e.g., 192.168.0.1:80).
    // -p: Afișează procesele asociate

    // Arată PID-ul (Process ID) și numele procesului care folosește conexiunea sau portul respectiv. Această opțiune necesită permisiuni de root (sau sudo).
    // -l: Afișează doar porturile de ascultare (listening)

    // Filtrează afișarea pentru a include doar porturile care sunt în starea de ascultare, ignorând conexiunile active.

    fp = popen("netstat -anpl", "r");
    if (fp == NULL) {
        perror("popen failed");
        return;
    }


    while (fgets(line, sizeof(line), fp)) {
        // Detectăm secțiunea UNIX domain sockets
        if (strstr(line, "Active UNIX domain sockets") != NULL) {
            continue;
        }

            // Procesare conexiuni de rețea
            Connection conn;
            memset(&conn, 0, sizeof(Connection));

            char *token = strtok(line, " ");
            int field = 0;

            while (token != NULL) {
                switch (field) {
                    case 0:
                        strncpy(conn.protocol, token, sizeof(conn.protocol));
                        break;
                    case 3:
                        sscanf(token, "%15[^:]:%d", conn.source_ip, &conn.source_port);
                        break;
                    case 4:
                        sscanf(token, "%15[^:]:%d", conn.dest_ip, &conn.dest_port);
                        break;
                    case 5:
                        strncpy(conn.state, token, sizeof(conn.state));
                        break;
                    case 6:
                        sscanf(token, "%d", &conn.pid);
                        break;
                    case 7:
                        strncpy(conn.program, token, sizeof(conn.program));
                        break;
                }
                token = strtok(NULL, " ");
                field++;
            }

            // Realocăm spațiu pentru conexiunea procesată
            *connections = realloc(*connections, sizeof(Connection) * (*connectionCount + 1));
            if (*connections == NULL) {
                perror("Memory allocation failed");
                pclose(fp);
                return;
            }
                // printf("PR:%s",conn.protocol);

            if(strstr(conn.protocol,"udp") != NULL || 
               strstr(conn.protocol,"tcp") != NULL ||
               strstr(conn.protocol,"icmp") != NULL){
                (*connections)[*connectionCount] = conn;
                (*connectionCount)++;
            }
        }

    pclose(fp);
    // printf("Citite %d conexiuni\n", *connectionCount);
}

void getSocketsInfo(Socket **listeningSockets, int *socketCount) {
    FILE *fp;
    char line[1024];

    // Deschidem fluxul `ss -l`

    //     Descompunerea comenzii
    // ss: Este utilitarul care afișează informații despre sockets (conexiuni de rețea și porturi). Este considerat mai performant decât netstat și înlocuiește această comandă în distribuțiile Linux moderne.

    // -l: Afișează doar socket-urile care sunt în starea de ascultare (listening). Aceste porturi sunt disponibile pentru a accepta conexiuni de la clienți.

    fp = popen("ss -l", "r");
    if (fp == NULL) {
        perror("popen failed");
        return;
    }

    // Ignorăm linia de antet
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        // Eliminăm liniile goale
        if (line[0] == '\n' || line[0] == '\0') continue;

        // Tokenizare linie
        char *token = strtok(line, " ");
        int field = 0;

        Socket sock;
        memset(&sock, 0, sizeof(Socket));

        int isListening = 0;

        while (token != NULL) {
            // Eliminăm spațiile goale multiple
            while (*token == ' ') token++;

            switch (field) {
                case 0:
                    strncpy(sock.protocol, token, sizeof(sock.protocol));
                    break;
                case 1:
                    if (strcmp(token, "LISTEN") == 0) {
                        isListening = 1;
                        strncpy(sock.state, token, sizeof(sock.state));
                    }
                    break;
                case 4:
                    // Adresa locală
                    if (strchr(token, '/') != NULL) {
                        strncpy(sock.path, token, sizeof(sock.path));
                    }
                    break;
                case 5:
                    // PID / Process, dacă există
                    if (isdigit(token[0])) {
                        sscanf(token, "%d", &sock.inode);
                    }
                    break;
            }
            token = strtok(NULL, " ");
            field++;
        }

        // Salvăm doar socketurile în stare LISTEN
        if (isListening) {
            *listeningSockets = realloc(*listeningSockets, sizeof(Socket) * (*socketCount + 1));
            if (*listeningSockets == NULL) {
                perror("Memory allocation failed");
                pclose(fp);
                return;
            }
            (*listeningSockets)[*socketCount] = sock;
            (*socketCount)++;
        }
    }

    pclose(fp);
    // printf("Citite %d socket-uri LISTENING.\n", *socketCount);
}

void getNetworkStats(unsigned long *rx_bytes, unsigned long *tx_bytes) {
    FILE *file = fopen("/proc/net/dev", "r");
    if (!file) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, INTERFACE)) {
            sscanf(line, "%*s %lu %*u %*u %*u %*u %*u %*u %*u %lu",
                   rx_bytes, tx_bytes);
            break;
        }
    }

    fclose(file);
}

void getTransferRate(TrafficStats* trafficMetrics){
    unsigned long old_rx_bytes = 0, old_tx_bytes = 0;
    unsigned long rx_bytes, tx_bytes;
    struct timeval old_time, new_time;

    getNetworkStats(&old_rx_bytes, &old_tx_bytes);
    gettimeofday(&old_time, NULL);

    sleep(5); 

    getNetworkStats(&rx_bytes, &tx_bytes);
    gettimeofday(&new_time, NULL);

    double elapsed_time = new_time.tv_sec - old_time.tv_sec +
                          (new_time.tv_usec - old_time.tv_usec) / 1e6;

    trafficMetrics->rxRate = (rx_bytes - old_rx_bytes) / elapsed_time;
    trafficMetrics->txRate = (tx_bytes - old_tx_bytes) / elapsed_time;

}

void startNcursesScreen() {
    // Initializează ncurses
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_CYAN); // Fundal albastru, text negru

    // Setează fundalul întregii ferestre principale
    wbkgd(stdscr, COLOR_PAIR(1));

    // Activează derularea pentru fereastra principală
    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);

    refresh();
    clear();
}

void initiateSocketsNcursesScreen(Socket* listeningSockets, int socketCount) {
    startNcursesScreen();

    // addSocketsRows(listeningSockets, socketCount);
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Obține dimensiunile ecranului

    int header_lines = 3;           // Liniile de antet (inclusiv separatoare)
    int footer_lines = 1;           // Linie pentru mesajul "Press 'q' to quit"
    int rows_per_page = max_y - header_lines - footer_lines; // Liniile vizibile pentru date
    int current_start = 0;          // Prima linie afișată

    // Dimensiuni fixe pentru coloane
    const int col_widths[] = {10, 10, 10, 10, 10, 10, 40}; // Dimensiuni fixe
    // const int col_count = sizeof(col_widths) / sizeof(col_widths[0]);

    // Crează fereastra pentru antet
    WINDOW* header_win = newwin(header_lines, max_x, 0, 0);
    wbkgd(header_win, COLOR_PAIR(1)); // Fundal albastru pentru antet
    wclear(header_win);

    // Afișează antetul
    mvwprintw(header_win, 0, 0, "Listening Sockets Table");
    mvwprintw(header_win, 1, 0, 
              "%-*s %-*s %-*s %-*s %-*s %-*s %-*s",
              col_widths[0], "Protocol",
              col_widths[1], "RefCount",
              col_widths[2], "Flags",
              col_widths[3], "Type",
              col_widths[4], "State",
              col_widths[5], "Inode",
              col_widths[6], "Path");
    mvwhline(header_win, 2, 0, '-', max_x); // Linie separatoare
    wrefresh(header_win);

    // Crează fereastra pentru conținutul derulabil
    WINDOW* scrollwin = newwin(rows_per_page, max_x, header_lines, 0);
    keypad(scrollwin, TRUE);
    scrollok(scrollwin, TRUE);
    wbkgd(scrollwin, COLOR_PAIR(1)); // Fundal albastru pentru fereastra de scroll

    // Crează o fereastră pentru footer (mesajul "Press 'q' to quit")
    WINDOW* footer_win = newwin(footer_lines, max_x, header_lines + rows_per_page, 0);
    wbkgd(footer_win, COLOR_PAIR(1));
    wclear(footer_win);
    mvwprintw(footer_win, 0, 0, "Press 'q' to quit");
    wrefresh(footer_win);

    while(1){
        wclear(scrollwin);

        // Afișează liniile curente în subfereastra derulabilă
        for (int i = 0; i < rows_per_page && (current_start + i) < socketCount; i++) {
            char refCount[20], inode[20];

            snprintf(refCount, sizeof(refCount), "%d", listeningSockets[current_start + i].refCount);
            snprintf(inode, sizeof(inode), "%d", listeningSockets[current_start + i].inode);

            if (strlen(listeningSockets[current_start + i].path) == 0) {
                continue; // Omite liniile fără date valide
            }

            // Afișează fiecare rând cu lățimi fixe
            mvwprintw(scrollwin, i, 0, 
                      "%-*s %-*s %-*s %-*s %-*s %-*s %-*s",
                      col_widths[0], listeningSockets[current_start + i].protocol,
                      col_widths[1], refCount,
                      col_widths[2], listeningSockets[current_start + i].flags,
                      col_widths[3], listeningSockets[current_start + i].type,
                      col_widths[4], listeningSockets[current_start + i].state,
                      col_widths[5], inode,
                      col_widths[6], listeningSockets[current_start + i].path);
        }

        wrefresh(scrollwin);

        // Gestionează derularea prin taste
        int ch = wgetch(scrollwin);
        if (ch == 'q') break; // Ieșire
        else if (ch == KEY_DOWN && current_start < socketCount - rows_per_page) {
            current_start++;
        } else if (ch == KEY_UP && current_start > 0) {
            current_start--;
        }
    }

    delwin(scrollwin);
    delwin(header_win);
    delwin(footer_win);

    endwin();
}

void initiateConnectionsNcursesScreen(Connection* connections, int connectionsCount, TrafficStats trafficStats) {
    startNcursesScreen();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x); // Obține dimensiunile ecranului

    int header_lines = 3;           // Liniile de antet (inclusiv separatoare)
    int footer_lines = 1;           // Linie pentru mesajul "Press 'q' to quit"
    int rows_per_page = max_y - header_lines - footer_lines; // Liniile vizibile pentru date
    int current_start = 0;          // Prima linie afișată

    // Dimensiuni fixe pentru coloane
    const int col_widths[] = {20, 20, 20, 20, 20}; // Dimensiuni fixe

    // Crează fereastra pentru antet
    WINDOW* header_win = newwin(header_lines, max_x, 0, 0);
    wbkgd(header_win, COLOR_PAIR(1)); // Fundal albastru pentru antet
    wclear(header_win);

    // Afișează antetul
    mvwprintw(header_win, 0, 0, "Connections Table");
    mvwprintw(header_win, 1, 0, 
              "%-*s %-*s %-*s %-*s %-*s",
              col_widths[0], "Source IP",
              col_widths[1], "Source Port",
              col_widths[2], "Destination IP",
              col_widths[3], "Destination Port",
              col_widths[4], "Protocol");
    mvwhline(header_win, 2, 0, '-', max_x); // Linie separatoare
    wrefresh(header_win);

    // Crează fereastra pentru conținutul derulabil
    WINDOW* scrollwin = newwin(rows_per_page, max_x, header_lines, 0);
    keypad(scrollwin, TRUE);
    scrollok(scrollwin, TRUE);
    wbkgd(scrollwin, COLOR_PAIR(1)); // Fundal albastru pentru fereastra de scroll

    // Crează o fereastră pentru footer (mesajul "Press 'q' to quit")
    WINDOW* footer_win = newwin(footer_lines, max_x, header_lines + rows_per_page, 0);
    wbkgd(footer_win, COLOR_PAIR(1));
    wclear(footer_win);
    mvwprintw(footer_win, 0, 0, "Press 'q' to quit");
    wrefresh(footer_win);

    while(1){
        wclear(scrollwin);
        int display_row = 0; // This will track the actual row number for valid lines

        // Afișează liniile curente în subfereastra derulabilă
        for (int i = current_start; i < connectionsCount && display_row < rows_per_page; i++) {

            // Verifică dacă ambele porturi sunt 0
            if (connections[i].source_port == 0 && connections[i].dest_port == 0) {
                continue; // Sari peste această linie
            }

            char source_port[20], dest_port[20];
            snprintf(source_port, sizeof(source_port), "%d", connections[i].source_port);
            snprintf(dest_port, sizeof(dest_port), "%d", connections[i].dest_port);

            // Afișează fiecare rând cu lățimi fixe
            mvwprintw(scrollwin, display_row, 0, 
                      "%-*s %-*s %-*s %-*s %-*s",
                      col_widths[0], connections[i].source_ip,
                      col_widths[1], source_port,
                      col_widths[2], connections[i].dest_ip,
                      col_widths[3], dest_port,
                      col_widths[4], connections[i].protocol);

            // Increment display_row only when a valid line is displayed
            display_row++;
        }

        mvwprintw(scrollwin, display_row, 0, "\nTransfer Rate:\t%ld\t|\t%ld",trafficStats.rxRate,trafficStats.txRate);
        display_row++;

        wrefresh(scrollwin);

        // Gestionează derularea prin taste
        int ch = wgetch(scrollwin);
        if (ch == 'q') break; // Ieșire
        else if (ch == KEY_DOWN && current_start < connectionsCount - rows_per_page) {
            current_start++;
        } else if (ch == KEY_UP && current_start > 0) {
            current_start--;
        }
    }

    delwin(scrollwin);
    delwin(header_win);
    delwin(footer_win);

    endwin();
}

int main(void) {

    Connection* connections = NULL;
    Socket* listeningSockets = NULL;
    TrafficStats trafficData;
    int connectionsCount = 0;
    int socketCount = 0;

    #pragma omp parallel num_threads(2) shared(connections, connectionsCount)
    {
        int thread_id = omp_get_thread_num();
        if(thread_id == 0){
            //this will read data from netstat
            getConnections(&connections, &connectionsCount);
        }else{
            // get transferRate
            getTransferRate(&trafficData);
        }
    }

    getSocketsInfo(&listeningSockets, &socketCount);
    #pragma omp barrier

    printConnections(connections, connectionsCount);
    printSockets(listeningSockets, socketCount);

    printf("RX Rate: %lu bytes/sec, TX Rate: %lu bytes/sec\n", trafficData.rxRate, trafficData.txRate);

    initiateConnectionsNcursesScreen(connections,connectionsCount, trafficData);
    initiateSocketsNcursesScreen(listeningSockets,socketCount);
    // initiateSocketNcursesScreen(listeningSockets,socketCount);
    // endwin();

    free(listeningSockets);
    free(connections);

    return 0;
}