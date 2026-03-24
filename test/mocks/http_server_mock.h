#ifndef HTTP_SERVER_MOCK_H
#define HTTP_SERVER_MOCK_H

#include "../../src/wifi/http_server.h"

void wifiHttpServerMockReset();
void wifiHttpServerMockSetBeginResult(bool result);
int wifiHttpServerMockGetBeginCallCount();
int wifiHttpServerMockGetEndCallCount();
int wifiHttpServerMockGetPollCallCount();
bool wifiHttpServerMockDispatchRequest(const char *method,
                                       const char *path,
                                       const char *authHeader,
                                       const char *body,
                                       int *status,
                                       const char **contentType,
                                       char *responseBuf,
                                       size_t responseBufSize);

#endif  // HTTP_SERVER_MOCK_H
