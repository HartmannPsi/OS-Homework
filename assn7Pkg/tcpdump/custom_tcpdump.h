#ifndef CUSTOM_TCPDUMP_H
#define CUSTOM_TCPDUMP_H
#include <stddef.h>

/**
 * @brief 使用自定义过滤规则对网络数据进行抓包
 * @param iface 需要进行抓包的网络接口名称
 * @param custom_filter 用户自定义的过滤表达式
 * @param buffer 存储抓取到的数据缓冲区
 * @param buffer_size 存储抓包数据的缓冲区大小
 * @return 成功时返回0，失败返回非0错误码
 */
int custom_tcpdump_capture(const char* iface, const char* custom_filter,
                           void* buffer, size_t buffer_size);

#endif