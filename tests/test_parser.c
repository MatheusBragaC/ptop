#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/core/parser.h"
#include "../src/common/cfg.h"

void test_parse_sysfs_int()
{
    printf("Running test_parse_sysfs_int...\n");
    assert(parse_sysfs_int("12345") == 12345);
    assert(parse_sysfs_int("  6789\n") == 6789);
    assert(parse_sysfs_int("0") == 0);
    assert(parse_sysfs_int("invalid") == 0);
    printf("Passed.\n");
}

void test_parse_proc_stat()
{
    printf("Running test_parse_proc_stat...\n");

    // Mock /proc/stat content for 4 cores (simplified)
    const char *mock_data = 
        "cpu  100 0 200 1000 0 0 0 0 0 0\n"
        "cpu0 10 0 20 100 0 0 0 0 0 0\n"
        "cpu1 10 0 20 100 0 0 0 0 0 0\n"
        "cpu2 10 0 20 100 0 0 0 0 0 0\n"
        "cpu3 10 0 20 100 0 0 0 0 0 0\n"
        "intr 123 0\n";

    CpuModel model = {0};
    uint64_t prev_total[CORES_N] = {0};
    uint64_t prev_idle[CORES_N] = {0};

    // First pass (initialization)
    parse_proc_stat(mock_data, &model, prev_total, prev_idle);

    // Verify parser updated state but usage should be 0 or irrelevant on first tick relative to 0
    // Check if prev_idle updated: cpu0 idle=100
    assert(prev_idle[0] == 100);

    // Second pass with increased values
    // cpu0: active increased by 10 (10->20, 20->30), idle by 10 (100->110)
    // total diff: 20 active + 10 idle = 30
    // usage: 20/30 = 66%
    const char *mock_data_2 = 
        "cpu  200 0 400 2000 0 0 0 0 0 0\n"
        "cpu0 20 0 30 110 0 0 0 0 0 0\n"
        "cpu1 20 0 30 110 0 0 0 0 0 0\n"
        "cpu2 20 0 30 110 0 0 0 0 0 0\n"
        "cpu3 20 0 30 110 0 0 0 0 0 0\n"
        "intr 125 0\n";
    
    parse_proc_stat(mock_data_2, &model, prev_total, prev_idle);

    printf("CPU0 Usage: %d%%\n", model.usage[0]);
    
    // allow some integer rounding variance, but strictly logic says:
    // prev: active=30, idle=100, total=130
    // curr: active=50, idle=110, total=160
    // diff_total=30, diff_idle=10
    // usage = (30-10)*100 / 30 = 2000/30 = 66.6% -> 66
    
    // Wait, my manual calculation of mock data:
    // Pass 1: user=10, sys=20 => active=30. idle=100.
    // Pass 2: user=20, sys=30 => active=50. idle=110.
    // Diff Active=20. Diff Idle=10. Diff Total=30.
    // Usage = (Total - Idle) / Total = (30 - 10) / 30 = 20 / 30 = 66%
    
    assert(model.usage[0] == 66);
    printf("Passed.\n");
}

int main()
{
    test_parse_sysfs_int();
    test_parse_proc_stat();
    return 0;
}
