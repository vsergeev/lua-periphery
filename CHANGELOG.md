* v2.4.1 - 04/21/2023
    * Bump c-periphery dependency version to v2.4.1 for fix of realtime
      timestamp reporting for line events in gpio-cdev v2 implementation.

* v2.4.0 - 03/31/2023
    * Bump c-periphery dependency version to v2.4.0 for gpio-cdev v2 ABI
      support in character device GPIOs.

* v2.3.1 - 01/05/2021
    * Bump c-periphery dependency version to v2.3.1 for build fix of SPI under
      Linux kernel headers without 32-bit SPI mode flags support.

* v2.3.0 - 12/16/2020
    * Bump c-periphery dependency version to v2.3.0 for new advanced open
      function in MMIO module, for 32-bit extra flags support in SPI module,
      and minor build improvements.
    * MMIO
        * Add `path` property to table constructor for use with alternate
          memory character devices (e.g. `/dev/gpiomem`).
    * SPI
        * Add support for 32-bit flags to `extra_flags` property and table
          constructor.
    * Update missing error codes in documentation for I2C, MMIO, Serial, and
      SPI modules.

* v2.2.5 - 11/19/2020
    * Bump c-periphery dependency version to v2.2.5 for additional direction
      checks in GPIO module, improved string handling in LED and GPIO modules,
      and default optimization added to CFLAGS in Makefile.

* v2.2.4 - 09/11/2020
    * Bump c-periphery dependency version to v2.2.3 for build fix of GPIO under
      older Linux kernel headers missing line event support in the gpio-cdev
      ABI, and to fix bits per word truncation in SPI handle string
      representation.
    * Bump c-periphery dependency version to v2.2.4 for fix of future spurious
      closes after an error during open in MMIO, Serial, SPI, I2C, and GPIO
      modules.
    * Enable build with Lua 5.4.
    * Contributors
        * Frédéric Vergez, @ikarius - ae2b61c

* v2.2.2 - 07/28/2020
    * Bump c-periphery dependency version to v2.2.2 for conditional compilation
      of character device GPIO support to allow build under older Linux kernel
      headers, and to fix missing definition warnings.

* v2.2.1 - 05/31/2020
    * Bump c-periphery dependency version to v2.2.1 for build fix with uClibc
      and argument name fix in GPIO module.

* v2.2.0 - 05/29/2020
    * Bump c-periphery dependency version to v2.2.0 for new GPIO and Serial
      module APIs, fixes, and improvements.
    * GPIO
        * Make `timeout_ms` argument optional for `poll()` and default to
          blocking poll.
        * Add `poll_multiple()` static method.
        * Add line consumer `label` property.
        * Add line `bias`, line `drive`, and `inverted` properties.
        * Add additional properties to table constructor for character device
          GPIOs.
        * Improve wording in documentation.
    * Serial
        * Make `timeout_ms` argument optional for `poll()` and default to
          blocking poll.
        * Add `vmin` and `vtime` properties for the corresponding termios
          settings.
        * Add description of termios timeout behavior with `read()` to
          documentation.
        * Improve wording and fix typos in documentation.

* v2.1.0 - 01/07/2020
    * Bump c-periphery dependency version to v2.1.0 for new LED and PWM
      modules.
    * Add LED module.
    * Add PWM module.
    * Fix synopsis, examples, and read_event() description in GPIO module
      documentation.

* v2.0.1 - 10/08/2019
    * Bump c-periphery dependency version to v2.0.1 for character device GPIO
      support, to fix blocking read performance in Serial, and for error
      detection of unexpected empty read in Serial module.
    * Add support for character device GPIOs to the GPIO module.
        * Remove support for preserve direction from GPIO constructor.
        * Update error codes.
    * Simplify error codes for MMIO, I2C, and Serial modules.
    * Remove duplicated package version from all modules.
    * Update tests with running hints for Raspberry Pi 3.
    * Improve cross-compilation support in Makefile.

* v1.1.3 - 04/28/2018
    * Bump c-periphery dependency version to v1.1.3 to fix data's most
      significant bit getting stripped when opening a serial port with parity
      enabled.

* v1.1.2 - 04/01/2018
    * Bump c-periphery dependency version to v1.1.2 to add handling for delayed
      pin directory export on some platforms when opening a GPIO.

* v1.1.1 - 04/25/2017
    * Fix multiple reads in I2C transfer.
    * Bump c-periphery dependency version to v1.1.1 to fix blocking GPIO poll
      for some platforms.
    * Contributors
        * Wojciech Nizinski, @niziak - 2d39685

* v1.1.0 - 09/29/2016
    * Add support for preserving pin direction to GPIO open.
    * Fix build issues caused by make flags propagating to c-periphery.
    * Bump c-periphery dependency version for serial-related and build fixes.
    * Contributors
        * gw0 - cc17642

* v1.0.6 - 07/30/2015
    * Fix `struct timespec`/`nanosleep()` related compilation error on older
      versions of gcc/glibc.

* v1.0.5 - 04/08/2015
    * Bump `c-periphery` dependency version for serial-related portability
      fixes.
    * Add support for Lua 5.3.

* v1.0.4 - 01/29/2015
    * Fix `I2C_M_STOP` undeclared compilation error on kernel versions older
      than 3.6.

* v1.0.3 - 12/26/2014
    * Improve LuaRocks build process.
    * Make unit tests compatible with Lua 5.1
    * Minor documentation fixes.

* v1.0.2 - 05/30/2014
    * Add support for Lua 5.1.

* v1.0.1 - 05/16/2014
    * Change GPIO poll return type from integer to boolean.
    * Clean up docs.

* v1.0.0 - 05/15/2014
    * Initial release.
