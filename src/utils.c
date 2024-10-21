#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Função para obter a hora atual como string
void get_current_time(char* buffer, size_t size) {
    time_t raw_time;
    struct tm* time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(buffer, size, "[%H:%M:%S]", time_info);  // Formato: [HH:MM:SS]
}

void log_event(const char* ficheiroLog, const char* evento) {
    FILE* log_fp = fopen(ficheiroLog, "a");  // Open in append mode
    if (log_fp == NULL) {
        perror("Erro ao abrir o ficheiro de log");
        return;
    }
    char time_buffer[20];
    get_current_time(time_buffer, sizeof(time_buffer));
    fprintf(log_fp, "%s %s\n", time_buffer, evento);  // Write time + event
    fclose(log_fp);
}