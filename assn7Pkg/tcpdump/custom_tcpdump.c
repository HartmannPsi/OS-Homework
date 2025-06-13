#include "custom_tcpdump.h"

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t* handle = NULL;
  struct bpf_program fp;
  const u_char* packet;
  struct pcap_pkthdr header;
  size_t offset = 0;

  handle = pcap_open_live(iface, BUFSIZ, 1, 1000, errbuf);
  if (!handle) {
    fprintf(stderr, "pcap_open_live failed: %s\n", errbuf);
    return -1;
  }

  if (pcap_compile(handle, &fp, custom_filter, 0, PCAP_NETMASK_UNKNOWN) < 0) {
    fprintf(stderr, "pcap_compile failed: %s\n", pcap_geterr(handle));
    pcap_close(handle);
    return -2;
  }

  if (pcap_setfilter(handle, &fp) < 0) {
    fprintf(stderr, "pcap_setfilter failed: %s\n", pcap_geterr(handle));
    pcap_freecode(&fp);
    pcap_close(handle);
    return -3;
  }

  pcap_freecode(&fp);

  while ((packet = pcap_next(handle, &header)) != NULL) {
    if (offset + header.len > buffer_size) break;

    memcpy((u_char*)buffer + offset, packet, header.len);
    offset += header.len;
  }

  pcap_close(handle);
  return 0;
}
