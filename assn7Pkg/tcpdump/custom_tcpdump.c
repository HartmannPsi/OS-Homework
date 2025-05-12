#include "custom_tcpdump.h"  // 自定义头文件，声明接口函数

#include <pcap.h>    // 提供 pcap_open_live, pcap_compile 等函数
#include <stdio.h>   // 标准输入输出函数
#include <stdlib.h>  // 标准库（malloc, free）
#include <string.h>  // 字符串函数，如 memcpy

/**
 * @brief 使用自定义过滤规则对网络数据进行抓包
 *
 * @param iface 抓包使用的网络接口（如 "eth0", "lo"）
 * @param custom_filter 用户传入的过滤规则（如 "tcp port 80"）
 * @param buffer 用户传入的缓冲区，用于保存抓到的数据
 * @param buffer_size 缓冲区的最大大小（以字节为单位）
 * @return 抓包成功返回 0，失败返回负数（不同负数代表不同错误）
 */
int custom_tcpdump_capture(const char* iface, const char* custom_filter,
                           void* buffer, size_t buffer_size) {
  char errbuf[PCAP_ERRBUF_SIZE];  // 错误信息缓冲区
  pcap_t* handle = NULL;          // pcap session 句柄
  struct bpf_program fp;          // 编译好的 BPF 过滤器
  const u_char* packet;           // 抓取的数据包指针
  struct pcap_pkthdr header;      // 数据包头信息（时间戳、长度）
  size_t offset = 0;              // 当前写入 buffer 的偏移量

  // 打开一个抓包会话（iface 接口，BUFSIZ 缓冲区大小，1 表示混杂模式，1000
  // 表示超时时间）
  handle = pcap_open_live(iface, BUFSIZ, 1, 1000, errbuf);
  if (!handle) {
    fprintf(stderr, "pcap_open_live failed: %s\n", errbuf);
    return -1;
  }

  // 编译过滤规则（将 custom_filter 字符串转换为 BPF bytecode）
  if (pcap_compile(handle, &fp, custom_filter, 0, PCAP_NETMASK_UNKNOWN) < 0) {
    fprintf(stderr, "pcap_compile failed: %s\n", pcap_geterr(handle));
    pcap_close(handle);
    return -2;
  }

  // 设置过滤器（实际启用前面编译的规则）
  if (pcap_setfilter(handle, &fp) < 0) {
    fprintf(stderr, "pcap_setfilter failed: %s\n", pcap_geterr(handle));
    pcap_freecode(&fp);  // 释放编译器生成的过滤器内存
    pcap_close(handle);
    return -3;
  }

  // 编译器生成的过滤器使用完就可以释放
  pcap_freecode(&fp);

  // 开始循环抓包（只抓符合过滤规则的包）
  while ((packet = pcap_next(handle, &header)) != NULL) {
    // 检查是否越界（buffer 剩余空间是否够）
    if (offset + header.len > buffer_size) break;

    // 将抓到的包复制到用户提供的缓冲区中
    memcpy((u_char*)buffer + offset, packet, header.len);
    offset += header.len;
  }

  // 关闭 pcap 抓包句柄
  pcap_close(handle);
  return 0;
}
