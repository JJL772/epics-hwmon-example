
/** This make device support structs easier to work with,
  * but it's only available in base R7.0.3+ */
#define USE_TYPED_DSET

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <longinRecord.h>
#include <aiRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <recGbl.h>
#include <alarm.h>
#include <iocsh.h>

/******************************************************************************
 * Hardware Interface Decls
 *****************************************************************************/

const char* hwmon_type = "none";

/** hwmon state, may be shared between records */
struct temp_state {
  int fd;
  uint32_t cpu_max;
  uint32_t cpu_crit;
  epicsMutexId mutex;   /** Multiple records may be reading from the same FD */
};

struct temp_ops {
  const char* type;
  int (*init)(int /*hwmon_inst*/, struct temp_state*);
  uint32_t (*read)(struct temp_state*);
  struct temp_state state;
};

static int ops_init(struct temp_ops** ops);

/** Read temp via 'coretemp' hwmon driver */
static int coretemp_init(int, struct temp_state*);
/** Reead temp via 'k10temp' hwmon driver */
static int k10temp_init(int, struct temp_state*);
/** Read temp via the raspberrypi hwmon driver */
static int cpu_thermal_init(int, struct temp_state*);

/** Common implementation, since most of these drivers behave the same */
static uint32_t generic_temp_read(struct temp_state*);

struct temp_ops temp_ops[] = {
  {"coretemp", coretemp_init, generic_temp_read},
  {"k10temp", k10temp_init, generic_temp_read},
  {"cpu_thermal", cpu_thermal_init, generic_temp_read},
};


/******************************************************************************
 * Device Support Decls
 *****************************************************************************/

/** Device support routines */
static long temp_report(int level);
static long temp_init(int after);
static long temp_init_record(dbCommon* prec);
static long temp_read_ai(aiRecord* prec);

enum { CPU_TEMP, CPU_MAX_TEMP, CPU_CRIT_TEMP };

/** Device private structure for temp_mon. This is initialized and stored in
  * the DPVT field in the record, allowing the device support code to retain
  * some of its own state */
struct temp_mon_dpvt {
  int type;
  struct temp_ops* ops;
};

/** Define the device support functions */
struct aidset devTempMon = {
  .common = {
    .number = 6,
    .report = temp_report,
    .init = temp_init,
    .init_record = temp_init_record,
    .get_ioint_info = NULL,
  },
  .read_ai = temp_read_ai,
  .special_linconv = NULL,
};

epicsExportAddress(dset, devTempMon);

/******************************************************************************
 * EPICS Device Support
 *****************************************************************************/

/**
 * Generate some output when dumping EPICS device support info.
 * This isn't really needed for this device support module. You can set this to
 * NULL in the dset struct too.
 */
static long
temp_report(int level)
{
  return 0;
}

/**
 * Device support type initialization. This is called at IOC init time. 
 * after indicates if this was called before or after init_record. This is only
 * called once per device support type, with after=0 and after=1.
 */
static long
temp_init(int after)
{
  return 0;
}

static long
temp_init_record(dbCommon* prec)
{
  aiRecord* pai = (aiRecord*)prec;
  
  /** Parse the INST_IO input link specified by the database */
  const char* pinp = pai->inp.value.instio.string;

  int ntype = 0;

  /** Parse type. For now we only support CPU_TEMP */
  if (!strcmp(pinp, "CPU_TEMP"))
    ntype = CPU_TEMP;
  else if (!strcmp(pinp, "CPU_MAX_TEMP"))
    ntype = CPU_MAX_TEMP;
  else if (!strcmp(pinp, "CPU_CRIT_TEMP"))
    ntype = CPU_CRIT_TEMP;
  else {
    printf("Invalid type '%s', must be CPU_TEMP, CPU_MAX_TEMP or CPU_CRIT_TEMP\n", pinp);
    return S_dev_badInpType;
  }

  /** Try to init hwmon ops now */
  struct temp_ops* ops;
  if (ops_init(&ops) < 0) {
    /** Even if we can't find an appropriate hwmon interface, return success.
      * we'll just check for valid dpvt later. If we return error here, the
      * entire DB will not load */
    printf("Could not find appropriate hwmon interface\n");
    return 0;
  }
  
  /** Allocate and fill out the device private structure */
  struct temp_mon_dpvt* dpvt = calloc(1,sizeof(struct temp_mon_dpvt));
  dpvt->type = ntype;
  dpvt->ops = ops;
  
  /** Store our dpvt into the record. This will allow us to share state with 
    * the other per-record device support routines (i.e. temp_read_ai) */
  prec->dpvt = dpvt;

  return 0;
}

static long
temp_read_ai(aiRecord* prec)
{
  struct temp_mon_dpvt* dpvt = prec->dpvt;
  if (!dpvt) {
    /** If dpvt is NULL, the required hwmon interface was not found. Indicate that
      * the value is undefined */
    prec->udf = TRUE;
    /** Also set an INVALID_ALARM, indicating that there was an issue */
    recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
    return 0;
  }

  if (dpvt->type == CPU_TEMP) {
    /** Read the new value from sysfs. This is a direct read, so we'll be
      * getting a value in mC. i.e. 43375 for 43.375 C. The record support
      * will handle converting this value. Please refer to the CoreTemp.db
      * for slope and offset settings. */
    prec->rval = dpvt->ops->read(&dpvt->ops->state);
  }
  if (dpvt->type == CPU_CRIT_TEMP) {
    prec->rval = dpvt->ops->state.cpu_crit;
  }
  else if (dpvt->type == CPU_MAX_TEMP) {
    prec->rval = dpvt->ops->state.cpu_max;
  }
  
  /** Returning 0 allows record support to perform unit conversions on RVAL based on
    * the conversion parameters. In CoreTemp.db, we set these parameters (ESLO, EOFF)
    * to convert from mC -> C or F */
  return 0;
}

/******************************************************************************
 * Hardware interface
 *****************************************************************************/

static uint32_t
sysfs_read_uint(const char* file)
{
  int fd = open(file, O_RDONLY);
  if (fd < 0)
    return 0;
  
  char buf[128];
  ssize_t nr = 0;
  if ((nr = read(fd, buf, sizeof(buf))) <= 0)
    return 0;
  buf[nr-1] = 0;
  close(fd);
  return strtoul(buf, NULL, 10);
}

static int
ops_init(struct temp_ops** out_ops)
{
  int i;
  struct temp_ops* ops = NULL;

  /** Find appropriate hwmon interface */
  for (i = 0; i < 99; ++i) {
    char file[128];
    snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/name", i);

    int fd = open(file, O_RDONLY);
    if (fd < 0)
      continue;

    char name[256];
    ssize_t nr = 0;
    if ((nr = read(fd, name, sizeof(name))) <= 0) {
      close(fd);
      continue;
    }
    /** Strip trailing newline */
    name[nr-1] = 0;
    
    /** Find appropriate interface */
    for (int n = 0; n < sizeof(temp_ops)/sizeof(temp_ops[0]); ++n) {
      if (!strcmp(temp_ops[n].type, name)) {
        ops = &temp_ops[n];
        break;
      }
    }
    
    close(fd);
  
    if (ops)
      break;
  }
  
  /** No supported hwmon types... */
  if (!ops)
    return -1;

  *out_ops = ops;
  
  /** Don't re-init the state */
  if (ops->state.fd != 0)
    return 0;

  ops->state.mutex = epicsMutexMustCreate();

  hwmon_type = ops->type;

  /** Run the hwmon-specific init code for the hwmon instance we found */
  return ops->init(i, &ops->state);
}

/**
 * Read k10temp off of sysfs from an already open FD
 */
static uint32_t
generic_temp_read(struct temp_state* st)
{
  assert(st->fd >= 0);

  epicsMutexMustLock(st->mutex);
  
  lseek(st->fd, 0, SEEK_SET);
  
  char buf[32];
  ssize_t nr = 0;
  if ((nr = read(st->fd, buf, sizeof(buf))) <= 0)
    return 0;
  buf[nr-1] = 0;

  epicsMutexUnlock(st->mutex);

  return strtoul(buf, NULL, 10);
}

/**
 * Init k10temp
 */
static int
k10temp_init(int hwmon_inst, struct temp_state* state)
{
  /** k10temp doesn't support max_temp and crit_temp, just set to sane defaults */
  state->cpu_crit = 110000;
  state->cpu_max = 100000;
  
  /** The field we want to monitor is 'Tctl'. This isn't actually
    * 'real' CPU temperature, but rather a value that is read to control
    * cooling. Usually it'll be a bit higher than the actual temperature.
    * Ref: https://docs.kernel.org/hwmon/k10temp.html */
  for (int i = 1; i < 99; ++i) {
    char file[128];
    snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_label",
      hwmon_inst, i);
    
    int fd = open(file, O_RDONLY);
    if (fd < 0)
      break;
    
    char label[256];
    size_t nr = 0;
    if ((nr = read(fd, label, sizeof(label))) <= 0) {
      close(fd);
      continue;
    }
    /** Strip trailing newline too */
    label[nr-1] = 0;

    close(fd);

    /** Found the field we want, now open tempN_input */
    if (!strcmp(label, "Tctl")) {
      snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_input",
        hwmon_inst, i);
      state->fd = open(file, O_RDONLY);
      return 0;
    }
  }
  return -1;
}

static int
coretemp_init(int hwmon_inst, struct temp_state* state)
{
  /** Set sane defaults for cpu_crit/max */
  state->cpu_crit = 110000;
  state->cpu_max = 100000;


  /** For coretemp, we want to monitor the package temperature. This driver
    * actually exposes all core temperatures, unlike k10temp and cpu_thermal,
    * but we can't really monitor all of these. We could return maybe these using
    * an aai record in the future. Or we could average them.
    * I think that monitoring the package temperature is good enough though.
    * Ref: https://docs.kernel.org/hwmon/coretemp.html */
  for (int i = 1; i < 99; ++i) {
    char file[128];
    snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_label",
      hwmon_inst, i);

    int fd = open(file, O_RDONLY);
    if (fd < 0)
      break;
    
    char label[256];
    size_t nr = 0;
    if ((nr = read(fd, label, sizeof(label))) <= 0) {
      close(fd);
      continue;
    }
    /** Strip trailing newline too */
    label[nr-1] = 0;

    close(fd);

    /** Found the field we want, now open tempN_input */
    if (!strncasecmp(label, "Package id", sizeof("Package id")-1)) {
      snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_input",
        hwmon_inst, i);
      state->fd = open(file, O_RDONLY);
      
      /** 'coretemp' also supports tempX_crit and tempX_max, 
       * so let's set those */
      snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_max",
        hwmon_inst, i);
      state->cpu_max = sysfs_read_uint(file);
      snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp%d_crit",
        hwmon_inst, i);
      state->cpu_crit = sysfs_read_uint(file);

      return 0;
    }
  }
  return -1;
}

static int
cpu_thermal_init(int hwmon_inst, struct temp_state* state)
{
  /** 'cpu_thermal' doesn't support max_temp and crit_temp, 
    * just set to sane defaults */
  state->cpu_crit = 110000;
  state->cpu_max = 100000;
  
  /** 'cpu_thermal' only exposes a single value */
  char file[128];
  snprintf(file, sizeof(file), "/sys/class/hwmon/hwmon%d/temp1_input",  hwmon_inst);
  state->fd = open(file, O_RDONLY);
  return state->fd < 0 ? -1 : 0;
}

/******************************************************************************
 * IOC shell function registration
 *****************************************************************************/

static void
hwmonStatusCallFunc(const iocshArgBuf* args)
{
  /** Caller wants us to re-index all hwmon types */
  if (args[0].ival != 0) {
    printf("Available hwmon sensors:\n");

    /** Loop through all hwmon types and read their type */
    for (int i = 0; i < 99; ++i) {
      char buf[128];
      snprintf(buf, sizeof(buf), "/sys/class/hwmon/hwmon%d/name", i);
      int fd = open(buf, O_RDONLY);
      if (fd < 0)
        break;

      char name[128];
      ssize_t l = 0;
      if ((l = read(fd, name, sizeof(name)-1)) <= 0) {
        close(fd);
        continue;
      }
      name[l] = 0;
      close(fd);

      printf(" %s", name);
    }
    return;
  }

  /** Otherwise just report on what's being used */
  printf("hwmon type: %s\n", hwmon_type);
}

/** This is defined as a "registrar" in the dbd file. It will be called when
  * registerRecordDeviceDriver is, before iocBoot.
  */ 
void
tempMonRegister()
{
  /* hwmonStatus */
  {
    static const iocshArg arg0 = {"bReIndex", iocshArgInt};
    static const iocshArg* args[] = {&arg0};
    static const iocshFuncDef func = {"hwmonStatus", 1, args};
    iocshRegister(&func, hwmonStatusCallFunc);
  }
}

epicsExportRegistrar(tempMonRegister);