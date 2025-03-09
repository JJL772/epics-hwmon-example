# EPICS hwmon example

Sample IOC that monitors CPU usage through sysfs.

Supported platforms:
- Intel x86 processors via the 'coretemp' hwmon driver
- AMD x86 processors (k10 and later) via the 'k10temp' hwmon driver
- Raspberry Pi via the 'cpu\_thermal' hwmon driver

Demonstrates the following EPICS concepts:
- Custom device support for analog input record
- Analog input linear conversion using ESLO & EOFF
- Alarm parameters (HIHI, HIGH, HSV, HHSV)
- Alarm management in device support code
- Simple IOC application that uses the custom device support

## Creating a new app

```
# Create temperature monitor device support
makeBaseApp.pl -t support tempMonApp

# Create IOC application
makeBaseApp.pl -t ioc tempMonIoc

# Create IOC boot directories
makeBaseApp.pl -i temp-mon-test
```
