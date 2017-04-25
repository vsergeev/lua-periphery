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
