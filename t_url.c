#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "llurl.h"

// 辅助函数：获取字段字符串
const char *http_parser_url_field(const char *url, struct http_parser_url *u,
                                  enum http_parser_url_fields field, char *buf, size_t buflen) {
  if (!(u->field_set & (1 << field))) {
    return NULL;
  }

  uint16_t off = u->field_data[field].off;
  uint16_t len = u->field_data[field].len;

  if (len >= buflen) {
    return NULL;
  }

  memcpy(buf, url + off, len);
  buf[len] = '\0';

  return buf;
}

void print_url_fields(const char *url, struct http_parser_url *u) {
  char buf[256];

  printf("URL: %s\n", url);
  printf("Field set: 0x%04x\n", u->field_set);

  if (u->field_set & (1 << UF_SCHEMA)) {
    http_parser_url_field(url, u, UF_SCHEMA, buf, sizeof(buf));
    printf("Schema: %s\n", buf);
  }

  if (u->field_set & (1 << UF_HOST)) {
    http_parser_url_field(url, u, UF_HOST, buf, sizeof(buf));
    printf("Host: %s\n", buf);
  }

  if (u->field_set & (1 << UF_PORT)) {
    printf("Port: %d\n", u->port);
  }

  if (u->field_set & (1 << UF_PATH)) {
    http_parser_url_field(url, u, UF_PATH, buf, sizeof(buf));
    printf("Path: %s\n", buf);
  }

  if (u->field_set & (1 << UF_QUERY)) {
    http_parser_url_field(url, u, UF_QUERY, buf, sizeof(buf));
    printf("Query: %s\n", buf);
  }

  if (u->field_set & (1 << UF_FRAGMENT)) {
    http_parser_url_field(url, u, UF_FRAGMENT, buf, sizeof(buf));
    printf("Fragment: %s\n", buf);
  }

  if (u->field_set & (1 << UF_USERINFO)) {
    http_parser_url_field(url, u, UF_USERINFO, buf, sizeof(buf));
    printf("UserInfo: %s\n", buf);
  }

  printf("\n");
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>

// 性能测试结构
typedef struct {
    uint64_t total_iterations;    // 总迭代次数
    uint64_t successful_parses;   // 成功解析次数
    uint64_t failed_parses;       // 失败解析次数
    double total_time_seconds;    // 总耗时（秒）
    double parse_time_ns;         // 单次解析时间（纳秒）
    double throughput_per_second; // 每秒解析次数
} benchmark_result_t;

// 获取当前时间（纳秒）- 高精度版本
static inline uint64_t get_current_time_ns() {
#ifdef CLOCK_MONOTONIC_RAW
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000 + (uint64_t)tv.tv_usec * 1000;
#endif
}

// 预热CPU缓存（运行少量迭代）
static void warm_up_parser(const char* url, size_t url_len, int is_connect,
                           struct http_parser_url* parser_result, int warmup_iterations) {
    for (int i = 0; i < warmup_iterations; i++) {
        http_parser_parse_url(url, url_len, is_connect, parser_result);
    }
}

// 单个URL的性能测试
benchmark_result_t benchmark_single_url(const char* url, int is_connect,
                                        uint64_t iterations, int warmup_iterations) {
    benchmark_result_t result = {0};
    size_t url_len = strlen(url);
    struct http_parser_url parser_result;

    // 预热
    warm_up_parser(url, url_len, is_connect, &parser_result, warmup_iterations);

    // 开始性能测试
    uint64_t start_time_ns = get_current_time_ns();

    for (uint64_t i = 0; i < iterations; i++) {
        int parse_result = http_parser_parse_url(url, url_len, is_connect, &parser_result);
        if (parse_result == 0) {
            result.successful_parses++;
        } else {
            result.failed_parses++;
        }
    }

    uint64_t end_time_ns = get_current_time_ns();

    // 计算结果
    result.total_iterations = iterations;
    result.total_time_seconds = (double)(end_time_ns - start_time_ns) / 1e9;

    if (result.total_time_seconds > 0) {
        result.throughput_per_second = (double)iterations / result.total_time_seconds;
        result.parse_time_ns = (double)(end_time_ns - start_time_ns) / iterations;
    }

    return result;
}

// 批量URL性能测试（测试多个不同URL）
benchmark_result_t benchmark_url_set(const char** urls, int* is_connect_flags,
                                     int url_count, uint64_t iterations_per_url,
                                     int warmup_iterations) {
    benchmark_result_t total_result = {0};
    benchmark_result_t* individual_results = malloc(url_count * sizeof(benchmark_result_t));

    if (!individual_results) {
        fprintf(stderr, "Memory allocation failed\n");
        return total_result;
    }

    // 测试每个URL
    for (int i = 0; i < url_count; i++) {
        individual_results[i] = benchmark_single_url(urls[i], is_connect_flags[i],
                                                     iterations_per_url, warmup_iterations);

        // 累加总结果
        total_result.total_iterations += individual_results[i].total_iterations;
        total_result.successful_parses += individual_results[i].successful_parses;
        total_result.failed_parses += individual_results[i].failed_parses;
        total_result.total_time_seconds += individual_results[i].total_time_seconds;
    }

    // 计算总体性能指标
    if (total_result.total_time_seconds > 0) {
        total_result.throughput_per_second = (double)total_result.total_iterations /
                                             total_result.total_time_seconds;
        total_result.parse_time_ns = total_result.total_time_seconds * 1e9 /
                                     total_result.total_iterations;
    }

    free(individual_results);
    return total_result;
}

// 打印性能测试结果
void print_benchmark_result(const benchmark_result_t* result, const char* test_name) {
    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf("性能测试: %s\n", test_name);
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("总迭代次数:      %llu\n", result->total_iterations);
    printf("成功解析次数:    %llu\n", result->successful_parses);
    printf("失败解析次数:    %llu\n", result->failed_parses);
    printf("总耗时:         %.6f 秒\n", result->total_time_seconds);
    printf("单次解析时间:   %.2f 纳秒\n", result->parse_time_ns);
    printf("吞吐量:         %.2f 次/秒\n", result->throughput_per_second);
    printf("───────────────────────────────────────────────────────────────────\n");

    // 添加性能评级
    if (result->parse_time_ns < 10) {
        printf("性能评级:        ★★★★★ (极快)\n");
    } else if (result->parse_time_ns < 50) {
        printf("性能评级:        ★★★★☆ (快速)\n");
    } else if (result->parse_time_ns < 200) {
        printf("性能评级:        ★★★☆☆ (良好)\n");
    } else if (result->parse_time_ns < 1000) {
        printf("性能评级:        ★★☆☆☆ (一般)\n");
    } else {
        printf("性能评级:        ★☆☆☆☆ (较慢)\n");
    }
    printf("═══════════════════════════════════════════════════════════════════\n\n");
}

// 性能对比测试（多个不同实现）
void benchmark_comparison(const char* url, uint64_t iterations) {
    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf("URL 解析性能对比测试\n");
    printf("测试URL: %s\n", url);
    printf("═══════════════════════════════════════════════════════════════════\n");

    // 创建测试数据
    struct http_parser_url parser_result;
    size_t url_len = strlen(url);

    // 方法1: 直接调用（基准）
    uint64_t start = get_current_time_ns();
    for (uint64_t i = 0; i < iterations; i++) {
        http_parser_parse_url(url, url_len, 0, &parser_result);
    }
    uint64_t end = get_current_time_ns();
    double time_ns = (double)(end - start) / iterations;
    double throughput = 1e9 / time_ns;

    printf("实现方法:           本实现 (基于状态机)\n");
    printf("单次解析时间:       %.2f 纳秒\n", time_ns);
    printf("吞吐量:             %'.2f 次/秒\n", throughput);
    printf("───────────────────────────────────────────────────────────────────\n");

    // 这里可以添加其他实现方法的对比
    printf("对比说明:           (可在此处添加其他实现的测试代码)\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
}

// 内存使用分析
void analyze_memory_usage() {
    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf("内存使用分析\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("结构体大小:\n");
    printf("  http_parser_url:  %zu 字节\n", sizeof(struct http_parser_url));
    printf("  其中: field_set:  %zu 字节\n", sizeof(uint16_t));
    printf("        port:       %zu 字节\n", sizeof(uint16_t));
    printf("        field_data: %zu 字节 × %d 字段 = %zu 字节\n",
           sizeof(struct { uint16_t off; uint16_t len; }),
           UF_MAX, sizeof(struct { uint16_t off; uint16_t len; }) * UF_MAX);

    // 估计栈使用量
    size_t estimated_stack_usage =
        sizeof(struct http_parser_url) +    // 结果结构体
        sizeof(const uint8_t*) * 3 +        // 指针变量
        sizeof(uint16_t) +                  // port_val
        sizeof(int) * 2 +                   // 标志变量
        sizeof( int );               // 状态枚举

    printf("估计栈使用量:      ~%zu 字节\n", estimated_stack_usage);
    printf("内存特点:          无动态内存分配，零碎片\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
}

// 完整的性能测试套件
void run_comprehensive_benchmark_suite() {
    // 测试URL集合
    const char* test_urls[] = {
        "https://www.example.com:8080/path/to/file?query=1#fragment",
        "http://user:pass@example.com/path",
        "ftp://example.com",
        "/relative/path?query",
        "//example.com/path",
        "mailto:user@example.com",
        "http://[2001:db8::1]:8080/path",
        "example.com:8080",
        "https://api.example.com/v1/users/12345/posts?limit=10&offset=0",
        "ws://chat.example.com:9000/ws"
    };

    int is_connect_flags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int url_count = sizeof(test_urls) / sizeof(test_urls[0]);

    printf("===============================================================================\n");
    printf("                URL 解析器性能测试套件\n");
    printf("===============================================================================\n");

    // 测试参数
    uint64_t iterations_per_url = 1000000;  // 每个URL测试100万次
    int warmup_iterations = 1000;           // 预热1000次
    uint64_t total_iterations = iterations_per_url * url_count;

    printf("测试配置:\n");
    printf("  URL 数量:       %d\n", url_count);
    printf("  每个URL迭代次数: %llu\n", iterations_per_url);
    printf("  总迭代次数:      %llu\n", total_iterations);
    printf("  预热迭代次数:    %d\n", warmup_iterations);
    printf("───────────────────────────────────────────────────────────────────\n");

    // 运行批量测试
    printf("正在运行性能测试...\n");
    benchmark_result_t result = benchmark_url_set(test_urls, is_connect_flags,
                                                  url_count, iterations_per_url,
                                                  warmup_iterations);

    // 打印结果
    print_benchmark_result(&result, "批量URL测试");

    // 单个代表性URL的详细测试
    printf("正在进行单个代表性URL详细测试...\n");
    const char* representative_url = "https://www.example.com:8080/path/to/file?query=1#fragment";
    benchmark_result_t single_result = benchmark_single_url(representative_url, 0,
                                                            iterations_per_url * 2,
                                                            warmup_iterations);
    print_benchmark_result(&single_result, "代表性URL测试");

    // 性能对比
    benchmark_comparison(representative_url, 1000000);

    // 内存使用分析
    analyze_memory_usage();

    // 性能优化建议
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("性能优化建议:\n");
    printf("───────────────────────────────────────────────────────────────────\n");
    printf("1. 基于状态机的单次遍历设计 - ✓ 已实现\n");
    printf("2. 无动态内存分配 - ✓ 已实现\n");
    printf("3. 内联字符分类函数 - ✓ 已实现\n");
    printf("4. 使用查找表加速字符分类 - 可考虑实现\n");
    printf("5. 分支预测优化 - 状态机已优化\n");
    printf("6. 内存对齐优化 - 可进一步优化\n");
    printf("═══════════════════════════════════════════════════════════════════\n");

    // 预期性能
    printf("\n预期性能 (在 3.0 GHz CPU 上):\n");
    printf("  吞吐量:          10-50 百万次/秒\n");
    printf("  单次解析时间:    20-100 纳秒\n");
    printf("  内存带宽:        ~100 MB/秒 (假设平均URL长度50字节)\n");
    printf("===============================================================================\n");
}

// 简单的性能测试函数（返回每秒解析次数）
double benchmark_throughput(const char* url, uint64_t iterations) {
    size_t url_len = strlen(url);
    struct http_parser_url parser_result;

    // 预热
    for (int i = 0; i < 1000; i++) {
        http_parser_parse_url(url, url_len, 0, &parser_result);
    }

    // 开始测试
    uint64_t start_time_ns = get_current_time_ns();

    for (uint64_t i = 0; i < iterations; i++) {
        http_parser_parse_url(url, url_len, 0, &parser_result);
    }

    uint64_t end_time_ns = get_current_time_ns();

    // 计算吞吐量
    double total_time_seconds = (double)(end_time_ns - start_time_ns) / 1e9;
    double throughput = (double)iterations / total_time_seconds;

    return throughput;
}

// 快速性能测试函数
void quick_performance_test() {
    const char* test_url = "https://www.example.com:8080/path/to/file?query=1#fragment";
    uint64_t iterations = 5000000;  // 500万次

    printf("\n快速性能测试:\n");
    printf("URL: %s\n", test_url);
    printf("迭代次数: %llu\n", iterations);

    double throughput = benchmark_throughput(test_url, iterations);

    printf("───────────────────────────────────────────────────────────────────\n");
    printf("结果: %.2f 次/秒\n", throughput);

    if (throughput > 10000000) {
        printf("评价: 性能极佳 (> 10M ops/sec)\n");
    } else if (throughput > 5000000) {
        printf("评价: 性能优秀 (> 5M ops/sec)\n");
    } else if (throughput > 1000000) {
        printf("评价: 性能良好 (> 1M ops/sec)\n");
    } else {
        printf("评价: 性能一般\n");
    }
    printf("───────────────────────────────────────────────────────────────────\n");
}


int main() {
  struct http_parser_url u;
  int result;

  // 测试1: mailto URL
  printf("=== Test 1: mailto URL ===\n");
  const char *mailto_url = "mailto:user@example.com";
  result = http_parser_parse_url(mailto_url, strlen(mailto_url), 0, &u);
  if (result == 0) {
    print_url_fields(mailto_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", mailto_url);
  }

  // 测试2: IPv6链路本地地址
  printf("=== Test 2: IPv6 link-local address ===\n");
  const char *ipv6_url = "http://[fe80::1%25eth0]/path";
  result = http_parser_parse_url(ipv6_url, strlen(ipv6_url), 0, &u);
  if (result == 0) {
    print_url_fields(ipv6_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", ipv6_url);
  }

  // 测试3: 带编码字符的URL
  printf("=== Test 3: URL with encoded characters ===\n");
  const char *encoded_url = "http://example.com/path%20with%20spaces?query%3Dvalue%26key";
  result = http_parser_parse_url(encoded_url, strlen(encoded_url), 0, &u);
  if (result == 0) {
    print_url_fields(encoded_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", encoded_url);
  }

  // 测试4: 路径中带星号的URL
  printf("=== Test 4: URL with asterisk in path ===\n");
  const char *asterisk_url = "http://example.com/path/*/file.txt";
  result = http_parser_parse_url(asterisk_url, strlen(asterisk_url), 0, &u);
  if (result == 0) {
    print_url_fields(asterisk_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", asterisk_url);
  }

  // 测试5: 完整URL
  printf("=== Test 5: Complete URL ===\n");
  const char *full_url = "https://user:pass@www.example.com:8080/path/to/file?query=1&test=2#section";
  result = http_parser_parse_url(full_url, strlen(full_url), 0, &u);
  if (result == 0) {
    print_url_fields(full_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", full_url);
  }

  // 测试6: 相对URL
  printf("=== Test 6: Relative URL ===\n");
  const char *relative_url = "/path/to/file?query=test";
  result = http_parser_parse_url(relative_url, strlen(relative_url), 0, &u);
  if (result == 0) {
    print_url_fields(relative_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", relative_url);
  }

  // 测试7: protocol-relative URL
  printf("=== Test 7: Protocol-relative URL ===\n");
  const char *proto_relative = "//example.com/path";
  result = http_parser_parse_url(proto_relative, strlen(proto_relative), 0, &u);
  if (result == 0) {
    print_url_fields(proto_relative, &u);
  } else {
    printf("Failed to parse: %s\n\n", proto_relative);
  }

  // 测试8: CONNECT方法
  printf("=== Test 8: CONNECT method ===\n");
  const char *connect_url = "example.com:443";
  result = http_parser_parse_url(connect_url, strlen(connect_url), 1, &u);
  if (result == 0) {
    print_url_fields(connect_url, &u);
  } else {
    printf("Failed to parse: %s\n\n", connect_url);
  }

  printf("URL 解析器性能测试\n");
  printf("==================\n\n");

  // 方法1: 快速测试
  quick_performance_test();

  // 方法2: 完整测试套件
  printf("\n运行完整性能测试套件...\n");
  run_comprehensive_benchmark_suite();

  // 方法3: 单个URL详细测试
  printf("\n单个URL详细性能分析...\n");
  const char* complex_url = "https://user:pass@api.example.com:8443/v2/users/12345/posts?limit=50&offset=100&sort=desc#section";
  benchmark_result_t result1 = benchmark_single_url(complex_url, 0, 2000000, 1000);
  print_benchmark_result(&result1, "复杂URL测试");

  return 0;
}
