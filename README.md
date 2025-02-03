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
