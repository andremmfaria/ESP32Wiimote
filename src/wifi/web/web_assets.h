#ifndef WIFI_WEB_ASSETS_H
#define WIFI_WEB_ASSETS_H

#include <cstddef>

namespace web_assets {

extern const char kIndexHtml[];
extern const size_t kIndexHtmlLen;
const char *indexHtml();
size_t indexHtmlLen();

extern const char kAppJs[];
extern const size_t kAppJsLen;
const char *appJs();
size_t appJsLen();

extern const char kStylesCss[];
extern const size_t kStylesCssLen;
const char *stylesCss();
size_t stylesCssLen();

}  // namespace web_assets

#endif  // WIFI_WEB_ASSETS_H