#######################################
Extension points
#######################################

Version 2.5 of the library comes with predefined customization points.

 * For "no code customization"   : change default behaviors, enable/disable features customize pre-defined behaviors 
 * Implement new features        : if you need a feature that is not supported licensecc provides 4 extension points.
   - locators  : find and read a license file 
   - parser    : modify the license format 
   - verifier  : change how the license is validated (signature/limits/hardware identifiers)
   - merger    : if multiple licenses or errors are found change the strategy the error is reported.

***************************************************************
API parameters, constants, defaults: licensecc_properties.h
***************************************************************

The easiest configuration point is the file `licensecc_properties.h` in your project directory. 
This file is generated per project, when the project is first built near your public and private key. 
If you placed your projects outside `licensecc` source tree it may be committed in your private git.

The file is never overwritten once it's generated. 

It contains the default buffer sizes used in the API, environment variable names, default 
strategies behavior. If there is some arbitrary name in the library, you may find it here.

.. code-block:: c

   //names of the environment variables to find 
   #define LCC_LICENSE_LOCATION_ENV_VAR "LICENSE_LOCATION"
   #define LCC_LICENSE_DATA_ENV_VAR "LICENSE_DATA"

   //Maximum size of a license file or base64 data (we don't want somebody to crash our software)
   #define LCC_API_MAX_LICENSE_DATA_LENGTH 1024 * 8

   // Miscellaneous api data structure sizes
   #define LCC_API_PC_IDENTIFIER_SIZE 15
   #define LCC_API_PROPRIETARY_DATA_SIZE 64
   #define LCC_API_AUDIT_EVENT_NUM 5
   #define LCC_API_AUDIT_EVENT_PARAM2 255
   #define LCC_API_VERSION_LENGTH 15
   #define LCC_API_FEATURE_NAME_SIZE 15
   #define LCC_API_EXPIRY_DATE_SIZE 10
   #define LCC_API_ERROR_BUFFER_SIZE 256

Tweak default hardware signature generator
=============================================

If the provided hardware signatures don't behave well for your customers, or you want to change the default
way the library generates the pc identifier you can have a look at the following section.

First of all be sure to read about the standard behavior of :c:func:`identify_pc` here:

.. toctree::

   hardware_identifiers

Then you can change the way the default strategy works in `licensecc` by changing 
the underlying strategies in this section of `licensecc_properties.h`, as documented
in the :doc:`hardware identifiers reference <hardware_identifiers>` (the strategy lists and their
default values live there, keeping a single source of truth).

Tweak the date verification
=============================================

Licenses with an expiry date or a start date can be verified against the time reported by an NTP
server, to detect a system clock that has been set in the past to keep using an expired license:

.. code-block:: c

   #define NTP_SERVER_NAME "pool.ntp.org"
   #define NTP_CHECK NTP_CHECK_OPTIONAL
   #define MAX_ALLOWED_OFFSET_SEC 3600

``NTP_CHECK`` is ``NTP_CHECK_NO`` (verify the dates against the system clock only, the behavior of
the library prior to this feature), ``NTP_CHECK_OPTIONAL`` (default, if the server can't be reached
fall back to the system clock) or ``NTP_CHECK_REQUIRED`` (reject the license if the server can't be
reached). An out of sync clock is reported with the event ``TIME_OUT_OF_SYNC``.

Each setting is commented in ``licensecc_properties.h``. A full description of this limit, and of
how the dates are verified, is in the :ref:`Execution limits <analysis/features:Date>` section.

**************************************************************
Finding the licenses in new places: custom license locators 
**************************************************************
Your software struggle to find the license file? you can implement your own way to find it. For instance if you want 
to download the license from a remote server, you could implement the logic here. 
 
The extension points are declared in the public C++ header
``include/licensecc/LocatorStrategy.hpp`` (``license::locate::LocatorStrategy``).


.. TODO::
   
   this section need to be completed

**************************************************************
Adding new limits: 
**************************************************************

Verifiers use ``include/licensecc/datatypes_cpp.hpp``
(``license::LimitVerifierFn``).

.. TODO::
   
   this section need to be completed