/*******************************************************************************
  needfullthings.h
*******************************************************************************/

#ifndef _NEEDFULL_THINGS_H_
#define _NEEDFULL_THINGS_H_

/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE
#endif


/******************************************************************************/

static inline int my_min_int(int a, int b)
{
  if (a<b) return a;
  else return b;
}


/******************************************************************************/

#define UINT32_INC_CEIL(val)  \
{\
  uint32 temp;\
  temp = (val)+1;\
  if (temp) (val)=temp;\
}


/******************************************************************************/

#define UINT32_INC_CEIL_CNT_OVL(val,counter)  \
{\
  uint32 temp;\
  temp = (val)+1;\
  if (temp) \
  {\
    (val)=temp;\
  }\
  else\
  {\
    temp=(counter)+1;\
    if (temp)\
    {\
      (counter)=temp;\
    }\
  }\
}

/******************************************************************************/

#define UINT32_INC_X_CEIL(val,inc)  \
{\
  uint32 temp;\
  temp = (uint32)(val)+(uint32)(inc);\
  if (temp >= (uint32)(val)) \
  {\
    (val)=temp;\
  }\
  else\
  {\
    (val)=0xffffffffU;\
  }\
}

/******************************************************************************/

#define UINT32_INC_X_CEIL_CNT_OVL(val,inc,counter)  \
{\
  uint32 temp;\
  temp = (uint32)(val)+(uint32)(inc);\
  if (temp >= (uint32)(val)) \
  {\
    (val)=temp;\
  }\
  else\
  {\
    (val)=0xffffffffU;\
    temp=(counter)+1;\
    if (temp)\
    {\
      (counter)=temp;\
    }\
  }\
}

/******************************************************************************/

#define UINT32_LOW_HIGH_INC_X(low,high,inc)  \
{\
  uint32 temp;\
  temp = (uint32)(low)+(uint32)(inc);\
  if (temp >= (uint32)(low)) \
  {\
    (low)=temp;\
  }\
  else\
  {\
    (low)=temp;\
    (high)=(high)+1;\
  }\
}

/******************************************************************************
  plattform specific
******************************************************************************/

#ifdef ARCH_X86


#ifdef FILLER_RTAI

#define FEBRUARY                2
#define STARTOFTIME             1970
#define SECDAY                  86400L
#define SECYR                   (SECDAY * 365)
#define leapyear(y)             ((!(y % 4) && (y % 100)) || !(y % 400))
#define days_in_year(a)         (leapyear(a) ? 366 : 365)
#define days_in_month(a)        (month_days[(a) - 1])

struct rtc_time {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
        int tm_wday;
        int tm_yday;
        int tm_isdst;
};

static int month_days[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static void to_tm(int tim, struct rtc_time * tm)
{
        register int i;
        register long hms, day, gday;

        gday = day = tim / SECDAY;
        hms = tim % SECDAY;

        /* Hours, minutes, seconds are easy */
        tm->tm_hour = hms / 3600;
        tm->tm_min = (hms % 3600) / 60;
        tm->tm_sec = (hms % 3600) % 60;

        /* Number of years in days */
        for (i = STARTOFTIME; day >= days_in_year(i); i++)
                day -= days_in_year(i);
        tm->tm_year = i;

        /* Number of months in days left */
        if (leapyear(tm->tm_year))
                days_in_month(FEBRUARY) = 29;
        for (i = 1; day >= days_in_month(i); i++)
                day -= days_in_month(i);
        days_in_month(FEBRUARY) = 28;
        tm->tm_mon = i;

        /* Days are what is left over (+1) from all that. */
        tm->tm_mday = day + 1;

        /*
         * Determine the day of week. Jan. 1, 1970 was a Thursday.
         */
        tm->tm_wday = (gday + 4) % 7;
}


#define tic_read(var) {var=rt_get_cpu_time_ns();}

#elif defined FILLER_USER



#define tic_read(var) { struct timeval tv; gettimeofday(&tv,0);  var=(tv.tv_sec*1e6+tv.tv_usec); }

#define rt_task_wait_period() usleep(100)

#define do_gettimeofday(A) gettimeofday(A,0)

#define rt_printk printf

/* #define EBUSY 1 */

#endif

static __inline__ void set_msr (unsigned long msr)
{
}

/******************************************************************************/

static __inline__ void set_hid0 (unsigned long msr)
{
}

/******************************************************************************/

static __inline__ unsigned long disable_interrupts (void)
{
  return 0;
}

/******************************************************************************/


#else

// PowerPC

#define tic_read(var) __asm volatile("sync" "\n\t" "mftb %0" "\n\t" "sync" : "=r" (var) )

/******************************************************************************/

#define hid0_read(var) __asm volatile("sync" "\n\t" "mfspr %0,0x3f0" "\n\t" "sync" : "=r" (var) )

/******************************************************************************/

#define SYNC  __asm volatile("sync")

/******************************************************************************/

static __inline__ unsigned long get_msr (void)
{
  unsigned long msr;

  asm volatile ("sync");
  asm volatile ("mfmsr %0":"=r" (msr):);
  asm volatile ("sync");

  return msr;
}

/******************************************************************************/

static __inline__ void set_msr (unsigned long msr)
{
  asm volatile ("sync");
  asm volatile ("mtmsr %0"::"r" (msr));
  asm volatile ("sync");
}

/******************************************************************************/

static __inline__ void set_hid0 (unsigned long msr)
{
  asm volatile ("sync");
  asm volatile ("mtspr 0x3f0,%0"::"r" (msr));
  asm volatile ("sync");
}

/******************************************************************************/

static __inline__ unsigned long disable_interrupts (void)
{
  unsigned long msr;
  msr = get_msr();
  set_msr(msr & ~0x8000); /* disable interrupts */
  return msr;
}

/******************************************************************************/

static __inline__ unsigned long enable_interrupts (void)
{
  unsigned long msr;
  msr = get_msr();
  set_msr(msr | 0x8000);  /* enable interrupts */
  return msr;
}

/******************************************************************************/

static __inline__ unsigned long enable_data_cache (void)
{
      unsigned long hid0;
      hid0_read(hid0);
      set_hid0(hid0 | 0x00004000);
}

/******************************************************************************/

static __inline__ unsigned long disable_data_cache (void)
{
      unsigned long hid0;
      hid0_read(hid0);
      set_hid0( hid0 & (~0x00004000));
}

/******************************************************************************/

static __inline__ unsigned long enable_instruction_cache (void)
{
      unsigned long hid0;
      hid0_read(hid0);
      set_hid0(hid0 | 0x00008000);
}

/******************************************************************************/

static __inline__ unsigned long disable_instruction_cache (void)
{
      unsigned long hid0;
      hid0_read(hid0);
      set_hid0( hid0 & (~0x00008000));
}

/******************************************************************************/

#endif  // PowerPC


/******************************************************************************/

#endif /* _NEEDFULL_THINGS_H_ */
