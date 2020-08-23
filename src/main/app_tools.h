#ifndef APP_TOOLS_H
#define APP_TOOLS_H

///////////////////////////////////////////////////////////////////////////////

#define LOGMT(tag, message)     ESP_LOGD(tag, "%.03f KB %s", (float)(esp_get_free_heap_size()) / 1024, message);
#define LOGM(message)           LOGMT(TAG, message);

#define LOGM_FUNC_IN()          ESP_LOGD(TAG, "=> %s %.03f KB", __func__, (float)(esp_get_free_heap_size()) / 1024);
#define LOGM_FUNC_OUT()         ESP_LOGD(TAG, "<= %s %.03f KB", __func__, (float)(esp_get_free_heap_size()) / 1024);

#define LOG_FUNC_IN()           ESP_LOGD(TAG, "=> %s", __func__);
#define LOG_FUNC_OUT()          ESP_LOGD(TAG, "<= %s", __func__);

///////////////////////////////////////////////////////////////////////////////

#endif // APP_TOOLS_H