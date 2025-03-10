#include <sqlite3.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <iostream>
sqlite3 *db_parc, *db_users;
#define PORT 2750
int splitter(char *str, char *delim, char *part1, char *part2)
{
    if (str == NULL || part1 == NULL || part2 == NULL)
    {
        return -1; 
    }

    
    char *delimiter_pos = strchr(str, *delim);
    if (!delimiter_pos)
    {
        return -1;
    }

    
    int position = delimiter_pos - str;

    
    strncpy(part1, str, position);
    part1[position] = '\0'; 

    strcpy(part2, delimiter_pos + 1); 

    return position; }
void init_parc()
{
    char *err = NULL;
    int rc;

    rc = sqlite3_open("parcare.db", &db_parc);
    if (rc)
    {
        fprintf(stderr, "Nu s-a putut deschide baza de date: %s\n", sqlite3_errmsg(db_parc));
        return;
    }
    else
    {
        fprintf(stdout, "Baza de date deschisa cu succes.\n");
    }

    char *sql = "CREATE TABLE IF NOT EXISTS parcare ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "status TEXT NOT NULL, "
                "user TEXT);";

    rc = sqlite3_exec(db_parc, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la crearea tabelului: %s\n", err);
        sqlite3_free(err);
    }
    else
    {
        fprintf(stdout, "Tabelul creat cu succes.\n");
    }

    const char *verif = "SELECT COUNT(*) FROM parcare;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_parc, verif, -1, &stmt, NULL);
    sqlite3_step(stmt);
    int num = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (num == 0)
    {
        const char *insert = "INSERT INTO parcare (status) VALUES "
                             "('liber'), ('liber'), ('liber'), "
                             "('liber'), ('liber'), ('liber'), ('liber'), ('liber'), ('liber'), "
                             "('liber'), ('liber'), ('liber'), ('liber'), ('liber'), ('liber'), "
                             "('liber'), ('liber'), ('liber'), ('liber'), ('liber'), ('liber');";
        rc = sqlite3_exec(db_parc, insert, NULL, NULL, &err);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Eroare la turnare: %s\n", err);
            sqlite3_free(err);
        }
    }
    int rc1 = sqlite3_open("users.db", &db_users);
    if (rc1)
    {
        fprintf(stderr, "Nu s-a putut deschide baza 2 de date: %s\n", sqlite3_errmsg(db_users));
        return;
    }
    else
    {
        fprintf(stdout, "Baza de date 2 deschisa cu succes.\n");
    }

    char *sql1 = "CREATE TABLE IF NOT EXISTS users ("
                 "user TEXT PRIMARY KEY, "
                 "pass TEXT NOT NULL, "
                 "loged BOOL NOT NULL);";

    rc1 = sqlite3_exec(db_users, sql1, NULL, NULL, &err);
    if (rc1 != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la crearea tabelului: %s\n", err);
        sqlite3_free(err);
    }
    else
    {
        fprintf(stdout, "Tabelul creat cu succes.\n");
    }
}

void Parcare(char *rasp)
{
    const char *select = "SELECT id, status FROM parcare;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_parc, select, -1, &stmt, NULL) != SQLITE_OK)
    {
        sprintf(rasp, "Eroare la citirea datelor din baza de date.\n");
        return;
    }

    strcpy(rasp, "");
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        const char *status = (const char *)sqlite3_column_text(stmt, 1);
        char linie[50];
        sprintf(linie, "Loc %d: %s\n", id, status);
        strcat(rasp, linie);
    }
    sqlite3_finalize(stmt);
}

void Parcheaza(int id, char *user, char *rasp)
{
    
    const char *check_query = "SELECT id FROM parcare WHERE user = ?;";
    sqlite3_stmt *stmt_check;
    sqlite3_prepare_v2(db_parc, check_query, -1, &stmt_check, NULL);
    sqlite3_bind_text(stmt_check, 1, user, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt_check) == SQLITE_ROW)
    {
        int current_spot = sqlite3_column_int(stmt_check, 0);
        sprintf(rasp, "Masina este deja parcata pe locul %d.\n", current_spot);
        sqlite3_finalize(stmt_check);
        return;
    }
    sqlite3_finalize(stmt_check);

   
    const char *schimba = "UPDATE parcare SET status = 'ocupat', user = ? WHERE id = ? AND status = 'liber';";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_parc, schimba, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(db_parc) > 0)
        {
            sprintf(rasp, "Ai parcat pe locul %d.\n", id);
        }
        else
        {
            sprintf(rasp, "Locul %d este deja ocupat.\n", id);
        }
    }
    else
    {
        sprintf(rasp, "Eroare la parcare pe locul %d.\n", id);
    }
    sqlite3_finalize(stmt);
}

void Pleaca(int idloc, char *user, char *rasp)
{
    
    const char *check_query = "SELECT id FROM parcare WHERE user = ?;";
    sqlite3_stmt *stmt_check;
    sqlite3_prepare_v2(db_parc, check_query, -1, &stmt_check, NULL);
    sqlite3_bind_text(stmt_check, 1, user, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt_check) != SQLITE_ROW)
    {
        sprintf(rasp, "Masina utilizatorului %s nu este parcata.\n", user);
        sqlite3_finalize(stmt_check);
        return;
    }
    sqlite3_finalize(stmt_check);
    const char *schimba = "UPDATE parcare SET status = 'liber', user = NULL WHERE id = ? AND status = 'ocupat';";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_parc, schimba, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, idloc);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(db_parc) > 0)
        {
            sprintf(rasp, "Ai eliberat locul %d.\n", idloc);
        }
        else
        {
            sprintf(rasp, "Locul %d este deja liber.\n", idloc);
        }
    }
    else
    {
        sprintf(rasp, "Eroare la eliberarea locului %d.\n", idloc);
    }
    sqlite3_finalize(stmt);
}
bool verif_user(char *user, char *pass)
{
    sqlite3_stmt *stmt;
    bool result = 0;

    const char *verif = "SELECT 1 FROM users WHERE user = ? AND pass = ?;";

    if (sqlite3_prepare_v2(db_users, verif, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db_users));
        return 0;
    }

    
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);

    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = 1; 
    }

    
    sqlite3_finalize(stmt);

    return result;
}
// Înregistrează utilizatorul
int inregistrare(char *user, char *pass)
{
    // Verifică dacă utilizatorul există deja
    const char *check_query = "SELECT 1 FROM users WHERE user = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_users, check_query, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);

    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    if (exists)
    {
        return 0; // Utilizatorul există deja
    }

    // Adaugă utilizatorul nou
    const char *insert_query = "INSERT INTO users (user, pass,loged) VALUES (?, ?,false);";
    sqlite3_prepare_v2(db_users, insert_query, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);

    int result = (sqlite3_step(stmt) == SQLITE_DONE) ? 1 : 0;
    sqlite3_finalize(stmt);
    return result;
}

void Parcagiu(int fc)
{
    char buffer[1024] = {0};
    char response[2048] = {0};
    char user[100];
    bool log = 0;
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int readBytes = read(fc, buffer, sizeof(buffer));
        if (readBytes <= 0)
        {
            break;
        }

        if (strncmp(buffer, "Despre", 6) == 0 && log == 1)
        {
            Parcare(response);
        }
        else if (strncmp(buffer, "Parchez", 7) == 0 && log == 1)
        {

            int spotId = atoi(buffer + 8);
            Parcheaza(spotId, user, response);
        }
        else if (strncmp(buffer, "plec", 4) == 0 && log == 1)
        {

            int spotId = atoi(buffer + 5);
            Pleaca(spotId, user, response);
        }
        else if (strncmp(buffer, "EXIT", 4) == 0 && log == 1)
        {
            char *schimbare = "UPDATE users SET loged = false WHERE user = ?;";
            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(db_users, schimbare, -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                log = 0;
                memset(user, 0, sizeof(user));
                strcpy(response, "Deconectare reusita.\n");
            }
            else
            {
                strcpy(response, "Eroare la deconectare.\n");
            }
            log = 0;
            break;
        }
        else if (strncmp(buffer, "log", 3) == 0)
        {
            memset(user, 0, sizeof(user)); 
            char pass[100] = {0};
            splitter(buffer + 4, " ", user, pass);

            if (verif_user(user, pass))
            {
                const char *check_query = "SELECT * FROM users WHERE user = ? AND loged = true;";
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db_users, check_query, -1, &stmt, NULL) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
                    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
                    sqlite3_finalize(stmt);

                    if (exists)
                    {
                        sprintf(response, "Utilizatorul este deja logat.\n");
                    }
                    else
                    {
                        const char *update_query = "UPDATE users SET loged = true WHERE user = ?;";
                        if (sqlite3_prepare_v2(db_users, update_query, -1, &stmt, NULL) == SQLITE_OK)
                        {
                            sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
                            if (sqlite3_step(stmt) == SQLITE_DONE)
                            {
                                sprintf(response, "Autentificare reusita\n");
                                log = 1;
                            }
                            else
                            {
                                fprintf(stderr, "Eroare la logare: %s\n", sqlite3_errmsg(db_users));
                                strcpy(response, "Eroare la logare.\n");
                            }
                            sqlite3_finalize(stmt);
                        }
                    }
                }
            }
            else
            {
                sprintf(response, "Autentificare esuata, incercati sa va inregistrati.\n");
            }
        }
        else if (strncmp(buffer, "reg", 3) == 0)
        {
            memset(user,0,sizeof(user));
            char pass[100] = {0};
            splitter(buffer + 4, " ", user, pass);
            int ok = inregistrare(user, pass);
            if (ok == 1)
            {
                char *update = "UPDATE users SET loged = true WHERE user = ?;";
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db_users, update, -1, &stmt, NULL) != SQLITE_OK)
                {
                    fprintf(stderr, "Eroare la inregistrare: %s\n", sqlite3_errmsg(db_users));
                    strcpy(response, "Eroare la inregistrare.\n");
                }
                else
                {
                    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) == SQLITE_DONE)
                    {
                        sprintf(response, "Inregistrare reusita\n");
                        log = 1;
                    }
                    else
                    {
                        fprintf(stderr, "Eroare la actualizarea statusului de logare: %s\n", sqlite3_errmsg(db_users));
                        strcpy(response, "Eroare la actualizarea statusului de logare.\n");
                    }
                    sqlite3_finalize(stmt);
                }
            }
            else 
            {
                strcpy(response, "Inregistrare esuata. Utilizatorul exista deja sau a aparut o eroare.\n");
            }
        }
        else if (strncmp(buffer, "disc", 4) == 0)
            {
                const char *check_query = "SELECT * FROM users WHERE user = ? AND loged = true;";
                sqlite3_stmt *stmt;

                
                if (sqlite3_prepare_v2(db_users, check_query, -1, &stmt, NULL) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);

                    
                    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
                    sqlite3_finalize(stmt);

                    if (exists)
                    {
                        const char *update_query = "UPDATE users SET loged = false WHERE user = ?;";
                        if (sqlite3_prepare_v2(db_users, update_query, -1, &stmt, NULL) == SQLITE_OK)
                        {
                            sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);

                            
                            if (sqlite3_step(stmt) == SQLITE_DONE)
                            {
                                log = 0;
                                memset(user, 0, sizeof(user)); 
                                sprintf(response, "Deconectare reusita.\n");
                            }
                            else
                            {
                                fprintf(stderr, "Eroare la deconectare: %s\n", sqlite3_errmsg(db_users));
                                strcpy(response, "Eroare la deconectare.\n");
                            }
                            sqlite3_finalize(stmt);
                        }
                        else
                        {
                            fprintf(stderr, "Eroare la pregătirea cererii de deconectare: %s\n", sqlite3_errmsg(db_users));
                            strcpy(response, "Eroare la pregătirea cererii de deconectare.\n");
                        }
                    }
                    else
                    {
                        strcpy(response, "Acest utilizator nu este logat.\n");
                    }
                }
                else
                {
                    fprintf(stderr, "Eroare la verificarea utilizatorului: %s\n", sqlite3_errmsg(db_users));
                    strcpy(response, "Eroare la verificarea utilizatorului.\n");
                }
            }

        else if (log == 1)
        {
            strcpy(response, "Comanda necunoscuta.\n");
        }
        else
        {
            strcpy(response, "Trebuie sa va logati.\n");
        }

        send(fc, response, strlen(response), 0);
    }

    close(fc);
    exit(0);
}
bool isStatusFullyLiber(sqlite3 *db)
{
    const char *query = "SELECT COUNT(*) FROM parcare WHERE status != 'liber';";
    sqlite3_stmt *stmt;
    int result = 0;

    
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = sqlite3_column_int(stmt, 0); 
    }

   
    sqlite3_finalize(stmt);

    
    return result == 0;
}
void rand_parc(int loc)
{
    const char *schimba = "UPDATE parcare SET status = 'ocupat' WHERE id = ? AND status = 'liber';";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_parc, schimba, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, loc);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        if (sqlite3_changes(db_parc) > 0)
        {
            printf("Ai ocupat locul %d.\n", loc);
        }
        else
        {
            printf("Locul %d este deja ocupat.\n", loc);
        }
    }
    else
    {
        printf("Eroare la ocuparea locului %d.\n", loc);
    }
    sqlite3_finalize(stmt);
}

int main()
{
    int rc = sqlite3_open("parcare.db", &db_parc);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Nu s-a deschis baza de date: %s\n", sqlite3_errmsg(db_parc));
        return -1;
    }
    int rc2 = sqlite3_open("users.db", &db_users);
    if (rc2 != SQLITE_OK)
    {
        fprintf(stderr, "Nu s-a deschis baza de date: %s\n", sqlite3_errmsg(db_users));
        return -1;
    }
    init_parc();
    if (isStatusFullyLiber(db_parc))
    {
        for (int i = 0; i < 10; i++)
        {
            rand_parc(1 + rand() % 21);
        }
    }
    else
    {
        printf("Nu toate locurile sunt libere.\n");
    }
    int serverFd, clientSocket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 15) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server la port %d...\n", PORT);

    while (1)
    {
        clientSocket = accept(serverFd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (clientSocket < 0)
        {
            perror("Accept failed");
            continue;
        }
        else
        {
            printf("Client conectat cu descriptorul %d\n", clientSocket);
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            close(clientSocket);
            continue;
        }

        if (pid == 0)
        { // Proces copil
            close(serverFd);
            Parcagiu(clientSocket);
            exit(0);
        }
        else
        { // Proces părinte
            close(clientSocket);
        }
    }

    sqlite3_close(db_parc);
    sqlite3_close(db_users);
    return 0;
}