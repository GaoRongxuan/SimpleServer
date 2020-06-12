#ifndef __LOG_H__
#define __LOG_H__

#define USE_DEBUG

#ifdef USE_DEBUG
#define LOG_INFO(format, args...) { \
    fprintf(stdout, "[%s][%d]"format, __FUNCTION__, __LINE__, ##args); \
}
#else
#define LOG_INFO(format, args...)
#endif

#define LOG_ERR(format, args...) { \
    fprintf(stderr, "[%s][%d]"format, __FUNCTION__, __LINE__, ##args); \
}

#endif
