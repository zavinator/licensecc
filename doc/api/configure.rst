#######################################
Configuration
#######################################

Customization points that need no new C++ code: you only change values. If what you miss is a behavior
the library doesn't provide, see the :doc:`extension points <extend>` you have to implement instead.

***************************************************************
API parameters, constants, defaults: licensecc_properties.h
***************************************************************

``licensecc_properties.h`` in your project directory is the first customization point. It is generated
per project when the project is first built, next to your public and private key, and it is never
overwritten afterwards. It holds the api buffer sizes, the environment variable names, the hardware
identification strategies and the behavior of the pre-defined verifiers: every setting is commented
there, so the file is also the list of what can be changed. Delete it and re-run the configuration step
to regenerate it from ``src/templates/licensecc_properties.h.in`` (ie. to go back to the library
defaults - the release notes tell you when an upgrade requires it).

The settings that don't require editing the file
=================================================

The macros below are the only ones defined inside an ``#ifndef`` in the generated file, so a definition
that reaches the preprocessor before it wins over the default, and they can be set at build time as
described in :ref:`Overriding a setting from the build line
<api/configure:Overriding a setting from the build line>`.

=============================== ============================== ==============================================================
Macro                           Default                        Effect
=============================== ============================== ==============================================================
``FIND_LICENSE_NEAR_MODULE``    ``true``                       Look for a license named after the executable in its folder.
``LCC_ADD_DEFAULT_LIMITS``      ``1``                          Register the date, pc signature, virtualization and license
                                                               signature verifiers. With ``0`` only the verifiers you register
                                                               run, see :ref:`adding new limits <api/extend:Adding new limits:>`.
``LCC_NTP_SERVER_NAME``         ``"pool.ntp.org"``             Host queried to check the system clock of a date limited license.
``LCC_NTP_CHECK``               ``LCC_NTP_CHECK_OPTIONAL``     How much the dates are verified against that host, see
                                                               :ref:`Tweak the date verification
                                                               <api/configure:Tweak the date verification>`.
``LCC_MAX_ALLOWED_OFFSET_SEC``  ``3600``                       Max tolerated difference, in seconds, between the system clock
                                                               and the server time.
=============================== ============================== ==============================================================

Overriding a setting from the build line
==========================================

The alternative to committing project-specific values in the generated header - to ship the same sources
with a different NTP server, or to build for a machine without network access.

The definition must reach the *compiler*: a ``cmake -DLCC_NTP_CHECK=...`` on the configure line only
creates a CMake variable, the header is not rewritten with it and the library default stays in place.

.. code-block:: console

   cmake .. -DCMAKE_CXX_FLAGS='-DLCC_NTP_CHECK=LCC_NTP_CHECK_NO -DLCC_MAX_ALLOWED_OFFSET_SEC=900'
   cmake .. -DCMAKE_CXX_FLAGS='-DLCC_NTP_SERVER_NAME=\"time.windows.com\"' # quotes must reach the preprocessor

.. code-block:: cmake

   add_definitions(-DLCC_NTP_SERVER_NAME="time.windows.com") # before add_subdirectory: its targets inherit it
   add_subdirectory(submodules/licensecc)

Those macros are read by the sources of the library, so they must be defined when **the library itself**
is compiled - defining them only in the application that links it (a ``find_package`` integration) does
nothing.

The remaining settings are plain ``#define``\ s: passed on the build line the value in the file wins
anyway, with a redefinition warning. Edit the file to change them, and keep the ``LCC_API_*`` sizes the
same in the library and in every application using it, since they are part of the api structures.

The values that are *substituted* when the file is generated - the project name and version, and
``LCC_VERIFY_MAGIC`` from ``LCC_PROJECT_MAGIC_NUM`` - are ordinary CMake variables instead
(``cmake .. -DLCC_PROJECT_NAME=MyApp -DLCC_PROJECT_MAGIC_NUM=42``), baked in at generation time: delete
the file and configure again to change one.

Tweak default hardware signature generator
=============================================

The strategy lists used when ``STRATEGY_DEFAULT`` is selected (``LCC_BARE_TO_METAL_STRATEGIES``,
``LCC_VM_STRATEGIES``, ``LCC_LXC_STRATEGIES``, ``LCC_DOCKER_STRATEGIES``, ``LCC_CLOUD_STRATEGIES``) are
changed in ``licensecc_properties.h``: their meaning and their default values are documented in the
:doc:`hardware identifiers reference <hardware_identifiers>`.

Tweak the date verification
=============================================

``LCC_NTP_CHECK`` is ``LCC_NTP_CHECK_NO`` (verify the dates against the system clock only, the behavior
of the library prior to this feature), ``LCC_NTP_CHECK_OPTIONAL`` (default, if the server can't be
reached fall back to the system clock) or ``LCC_NTP_CHECK_REQUIRED`` (reject the license if the server
can't be reached). An out of sync clock is reported with the event ``TIME_OUT_OF_SYNC``, and a license
without date limits never accesses the network. The full description of the limit is in the
:ref:`Execution limits <analysis/features:Date>` section.
