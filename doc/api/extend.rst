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

The first one needs no code: it is a matter of changing values, and it is documented in
:doc:`configure`. The rest of this page is about the second one, the interfaces you implement (and
compile) to add a behavior that the library doesn't provide.

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

The limits shipped with the library are registered unless the project sets ``LCC_ADD_DEFAULT_LIMITS``
to ``0``: see :ref:`the settings of licensecc_properties.h
<api/configure:The settings that don't require editing the file>`.

.. TODO::

   this section need to be completed
