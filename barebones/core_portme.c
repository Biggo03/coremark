/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/
#include "coremark.h"
#include "core_portme.h"
#include <stdarg.h>

#define CSR_MCYCLE       0xB00u
#define CSR_MDBG         0x7A0u
#define CSR_MTEST_STATUS 0x7C0u

#define TEST_PASS 0xABCD1234u
#define TEST_FAIL 0xDEADBEEFu

#define COREMARK_REPORT_MAGIC 0x434D524Bu

static ee_u32
read_csr(ee_u32 csr_addr)
{
    ee_u32 value;
    asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    return value;
}

static void
write_csr(ee_u32 csr_addr, ee_u32 value)
{
    asm volatile("csrw %0, %1" : : "i"(csr_addr), "r"(value));
}

#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

volatile coremark_run_report coremark_report = { 0 };

/* Porting : Timing functions
        How to capture time and convert to seconds must be ported to whatever is
   supported by the platform. e.g. Read value from on board RTC, read value from
   cpu clock cycles performance counter etc. Sample implementation for standard
   time.h and windows.h definitions included.
*/
CORETIMETYPE
barebones_clock()
{
    return read_csr(CSR_MCYCLE);
}
/* Define : TIMER_RES_DIVIDER
        Divider to trade off timer resolution and total time that can be
   measured.

        Use lower values to increase resolution, but make sure that overflow
   does not occur. If there are issues with the return value overflowing,
   increase this value.
        */
#define GETMYTIME(_t)              (*_t = barebones_clock())
#define MYTIMEDIFF(fin, ini)       ((fin) - (ini))
#define TIMER_RES_DIVIDER          1
#define SAMPLE_TIME_IMPLEMENTATION 1
#define EE_TICKS_PER_SEC           (RV32_COREMARK_CPU_FREQ_HZ / TIMER_RES_DIVIDER)

/** Define Host specific (POSIX), or target specific global time variables. */
static CORETIMETYPE start_time_val, stop_time_val;

/* Function : start_time
        This function will be called right before starting the timed portion of
   the benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or zeroing some system parameters - e.g. setting the cpu clocks
   cycles to 0.
*/
void
start_time(void)
{
    GETMYTIME(&start_time_val);
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the
   benchmark.

        Implementation may be capturing a system timer (as implemented in the
   example code) or other system parameters - e.g. reading the current value of
   cpu cycles counter.
*/
void
stop_time(void)
{
    GETMYTIME(&stop_time_val);
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.

        Actual value returned may be cpu cycles, milliseconds or any other
   value, as long as it can be converted to seconds by <time_in_secs>. This
   methodology is taken to accommodate any hardware or simulated platform. The
   sample implementation returns millisecs by default, and the resolution is
   controlled by <TIMER_RES_DIVIDER>
*/
CORE_TICKS
get_time(void)
{
    CORE_TICKS elapsed
        = (CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
    return elapsed;
}
/* Function : time_in_secs
        Convert the value returned by get_time to seconds.

        The <secs_ret> type is used to accommodate systems with no support for
   floating point. Default implementation implemented by the EE_TICKS_PER_SEC
   macro above.
*/
secs_ret
time_in_secs(CORE_TICKS ticks)
{
    secs_ret retval = ((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
    return retval;
}

ee_u32 default_num_contexts = 1;

int
ee_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

/* Function : portable_init
        Target specific initialization code
        Test for some common mistakes.
*/
void
portable_init(core_portable *p, int *argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (sizeof(ee_ptr_int) != sizeof(ee_u8 *))
    {
        ee_printf(
            "ERROR! Please define ee_ptr_int to a type that holds a "
            "pointer!\n");
    }
    if (sizeof(ee_u32) != 4)
    {
        ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
    }
    coremark_report.magic        = 0;
    coremark_report.total_ticks  = 0;
    coremark_report.iterations   = 0;
    coremark_report.size         = 0;
    coremark_report.total_errors = 0;
    coremark_report.seedcrc      = 0;
    coremark_report.crclist      = 0;
    coremark_report.crcmatrix    = 0;
    coremark_report.crcstate     = 0;
    coremark_report.crcfinal     = 0;
    p->portable_id = 1;
}
/* Function : portable_fini
        Target specific final code
*/
void
portable_fini(core_portable *p)
{
    p->portable_id = 0;
}

void
portable_shutdown(struct RESULTS_S *results,
                  ee_s16            total_errors,
                  ee_u16            seedcrc,
                  CORE_TICKS        total_time)
{
    coremark_report.magic        = COREMARK_REPORT_MAGIC;
    coremark_report.total_ticks  = total_time;
    coremark_report.iterations   = results->iterations;
    coremark_report.size         = results->size;
    coremark_report.total_errors = (ee_u32)(ee_s32)total_errors;
    coremark_report.seedcrc      = seedcrc;
    coremark_report.crclist      = results->crclist;
    coremark_report.crcmatrix    = results->crcmatrix;
    coremark_report.crcstate     = results->crcstate;
    coremark_report.crcfinal     = results->crc;

    write_csr(CSR_MDBG, ((ee_u32)seedcrc << 16) | results->crc);
    write_csr(CSR_MTEST_STATUS, (total_errors == 0) ? TEST_PASS : TEST_FAIL);
}
