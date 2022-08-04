
#include "uart_module_event.h"

static void profile_uart_module_event(struct log_event_buf *buf,
                                      const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(uart_module_event,
                      ENCODE(),
                      ENCODE(),
                      profile_uart_module_event);

static char *type_to_str(enum uart_module_event_type type)
{
    switch (type)
    {
    case UART_EVT_READY:
        return "UART_EVT_READY";
    case UART_EVT_RX:
        return "UART_EVT_RX";
    case UART_EVT_ERROR:
        return "UART_EVT_ERROR";
    default:
        return "UNKNOWN";
    }
}

static void log_uart_module_evt(const struct app_event_header *evt)
{

    struct uart_module_event *uart_module_evt = cast_uart_module_event(evt);

    APP_EVENT_MANAGER_LOG(evt, "Type: %s", type_to_str(uart_module_evt->type));
}

APP_EVENT_TYPE_DEFINE(uart_module_event,
                      log_uart_module_evt,
                      &uart_module_event_info,
                      APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
