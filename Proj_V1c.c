#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 2750
void logMenu()
{
    printf("\nMeniu:\n");
    printf("1.Autentificare (comanda: log <user> <pass>)\n");
    printf("2.Inregistrare (comanda: reg <user> <pass>)\n");
    printf("3.Iesi din aplicatie (comanda: EXIT)\n");
}
void displayMenu() {
    printf("\nMeniu:\n");
    printf("1. Parcheaza masina (comanda: Parchez <ID loc>)\n");
    printf("2. Pleaca masina (comanda: plec <ID loc>)\n");
    printf("3. Iesi din aplicatie (comanda: EXIT)\n");
    printf("4. Deconectare (comanda: disc)\n");
    printf("\nAlege o optiune si introdu comanda corespunzatoare:\n");
}

int main() {
    int sock = 0;
    struct sockaddr_in serverAddr;
    char buffer[1024] = {0};
    char command[1024];

    // Creeaza socketul
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Eroare la crearea socketului");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Conectare la server
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        perror("Adresa invalida");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Eroare la conectare");
        exit(EXIT_FAILURE);
    }

    printf("Conectat la server!\n");
    logMenu();
    while (1) {
        int ok = 0;
        printf("> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Elimina newline

        // Trimite comanda la server
        send(sock, command, strlen(command), 0);

        // Daca utilizatorul vrea sa iasa
        if (strncmp(command, "EXIT", 4) == 0) {
            printf("Deconectare...\n");
            close(sock);
            return 0;
        }

        // Primeste raspunsul de la server
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            printf("%s", buffer);
        } else {
            printf("Eroare la primirea raspunsului sau serverul s-a deconectat.\n");
            close(sock);
            return 1;
        }

        if (strncmp(buffer, "Autentificare reusita",21) == 0 || strncmp(buffer, "Inregistrare reusita",20) == 0) {
            while (1) {
                send(sock, "Despre", 6, 0);
                memset(buffer, 0, sizeof(buffer));
                valread = read(sock, buffer, sizeof(buffer));
                if (valread > 0) {
                    printf("%s", buffer);
                } else {
                    printf("Eroare la primirea raspunsului sau serverul s-a deconectat.\n");
                    break;
                }
                displayMenu();

                // Citeste comanda de la utilizator
                printf("> ");
                fgets(command, sizeof(command), stdin);
                command[strcspn(command, "\n")] = 0; // Elimina newline

                // Trimite comanda la server
                send(sock, command, strlen(command), 0);

                // Daca utilizatorul vrea sa iasa
                if (strncmp(command, "EXIT", 4) == 0) {
                    send(sock, "disc", 4, 0);
                    printf("Deconectare...\n");
                    ok = 1;
                    break;
                }
                if(ok == 1)
                {
                    break;
                }
                // Primeste raspunsul de la server
                memset(buffer, 0, sizeof(buffer));
                valread = read(sock, buffer, sizeof(buffer));
                if (valread > 0) {
                    printf("Raspuns server:\n%s\n", buffer);
                } else {
                    printf("Eroare la primirea raspunsului sau serverul s-a deconectat.\n");
                    break;
                }
                if (strncmp(buffer, "Deconectare reusita", 19) == 0) {
                    logMenu();
                    break;
                }
            }
        } else {
            logMenu();
        }
        if(ok == 1)
        {
            break;
        }
    }

    close(sock);
    return 0;
}