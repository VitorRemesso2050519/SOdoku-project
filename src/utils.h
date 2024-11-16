#ifndef UTILS_H
#define UTILS_H

void get_current_time(char* buffer, size_t size);
void log_event(const char* ficheiroLog, int user_id, int event_code, const char* description);
void imprimirGrelha(const char* grelha);

#endif // UTILS_H