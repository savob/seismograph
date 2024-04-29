#ifndef LOGGING_H
#define LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

int setupLogging();
int startLogFile();
int closeLogFile();
int appendLogFile(float reads[]);

#ifdef __cplusplus
}
#endif

#endif