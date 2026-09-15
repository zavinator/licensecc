########################
License acquisition flow
########################

Short description of the architecture used by ``Licensecc::acquire_license``
after the cursor/parser refactor.

Components
**********

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Component
     - Responsibility
   * - ``Licensecc``
     - Orchestrates the whole flow; owns the ``EventRegistry`` and the
       ``LicenseVerifier``.
   * - ``LocatorFactory::get_active_strategies``
     - Builds the list of active ``LocatorStrategy`` objects (application folder,
       env vars, caller-provided location, extra strategies).
   * - ``FoundLicenseCursor``
     - Iterates over the strategies and their locations, yielding ``RawLicenseData``
       (location id + raw license content). Strategies that return no locations
       are skipped.
   * - ``LicenseParser::parseLicense``
     - Parses one ``RawLicenseData`` (INI) into zero or more ``FullLicenseInfo``.
   * - ``LicenseVerifier::verify_limit``
     - Verifies (signature and limits) one ``FullLicenseInfo``, registers the
       outcome events into the ``EventRegistry`` and fills the ``LicenseInfo``
       output, returning a ``FUNCTION_RETURN``. The default verifiers are
       ``verify_date`` (``limits/date_verifier.cpp``), ``verify_pc_signature``,
       ``verify_virtualization`` and ``verify_signature``.
   * - ``merge_licenses`` (file-static free function in ``Licensecc.cpp``)
     - Picks the best valid license (no-input, or latest expiry) and fills
       ``LicenseInfo``; decides the final ``LCC_EVENT_TYPE``.

.. note::

   ``verify_date`` is the only verifier that can access the network: when the license declares a
   date limit and ``LCC_NTP_CHECK`` is not ``LCC_NTP_CHECK_NO`` it queries an NTP server to check the
   system clock, and may fail with ``TIME_OUT_OF_SYNC`` instead of ``LICENSE_OK`` /
   ``PRODUCT_EXPIRED``. See the settings documented in
   :ref:`Configuration <api/configure:Tweak the date verification>`.

Sequence diagram
****************

.. mermaid::

   sequenceDiagram
      participant C as C API
      participant F as Licensecc
      participant LF as LocatorFactory
      participant P as LicenseParser
      participant V as LicenseVerifier

      C->>+F: acquire_license(callerInfo, <br> licenseLocation, license_out)
      F->>LF: get_active_strategies(strategies, licenseLocation)
      LF-->>F: vector <LocatorStrategy>
 
      loop each strategy and location (FoundLicenseCursor) 
          F->>+P: parseLicense(RawLicenseData)
          P-->>-F: vector <FullLicenseInfo>
          loop each FullLicenseInfo
              F->>+V: verify_limit(fullLicenseInfo, er, LicenseInfo)
              V-->>-F: FUNCTION_RETURN
          end
      end
      
      F->>F: merge_licenses(all_results, er, license_out)
      F-->>-C: result

Notes
*****

- Strategies that return no locations are skipped by the cursor.
- Events (found/not found/malformed) are recorded in the shared
  ``EventRegistry`` and exported to ``LicenseInfo::status`` at the end.

