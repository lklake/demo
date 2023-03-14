//
// Created by happy on 3/10/23.
//

#ifndef DEMO_DEMO_ERROR_H
#define DEMO_DEMO_ERROR_H

#include <sys/time.h>

#define LOG_DEBUG(fmt,...) do{struct timeval tm;gettimeofday(&tm,NULL);printf("%lf:"fmt"\n",tm.tv_sec+tm.tv_usec/1000000.0,##__VA_ARGS__);}while(0)
#define LOG_MSG(fmt,...) do{struct timeval tm;gettimeofday(&tm,NULL);printf("%lf:"fmt"\n",tm.tv_sec+tm.tv_usec/1000000.0,##__VA_ARGS__);}while(0)
#define LOG_ERR(fmt,...) do{struct timeval tm;gettimeofday(&tm,NULL);printf("%lf:"fmt"\n",tm.tv_sec+tm.tv_usec/1000000.0,##__VA_ARGS__);}while(0)
#define LOG_WARN(fmt,...) do{struct timeval tm;gettimeofday(&tm,NULL);printf("%lf:"fmt"\n",tm.tv_sec+tm.tv_usec/1000000.0,##__VA_ARGS__);}while(0)

//#define LOG_DEBUG do{;}while(0);
//#define LOG_MSG do{;}while(0);
//#define LOG_ERR do{;}while(0);
//#define LOG_WARN do{;}while(0);

#endif //DEMO_DEMO_ERROR_H
