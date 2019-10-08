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
