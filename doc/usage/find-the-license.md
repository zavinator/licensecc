# Find the license file

## How `licensecc` finds the license file

licensecc, when integrated into a software, can automatically find its license file (or multiple
license files). It builds a list of *locator strategies* and tries them in this order:

1. **Next to the executable** — placing the license in the same folder as the licensed executable
   makes the software find its own license. The filename must match the executable name, with the
   `.lic` extension: if you're licensing `my_awesome_software.exe`, the license file must be named
   `my_awesome_software.lic` in the same folder. Controlled by `FIND_LICENSE_NEAR_MODULE`
   (enabled by default).

2. **Environment variables** (only when `FIND_LICENSE_WITH_ENV_VAR` is enabled, which is *not* the
   default):
   - Set `LICENSE_LOCATION` to the full path of the license file. Multiple files can be separated by
     `;`.
   - Set `LICENSE_DATA` to the full license content (base64 or raw) to load it directly.

3. **Caller-provided location** — the application passes a `LicenseLocation` structure to
   `acquire_license`, containing either a file path or the complete license content.

4. **Custom locator** — by implementing and registering the `license::locate::LocatorStrategy`
   interface (declared in `include/licensecc/LocatorStrategy.hpp`) software authors can define their
   own strategy, for example to download the license from a server. See the
   [extension points](../api/extend) reference.

## Multiple license files

licensecc can handle multiple license files at the same time. When several valid licenses are found,
they are merged and the best one is selected (see the
[license acquisition flow](../analysis/license_acquisition_flow) for how this works). This is
what lets a multi-feature license cover independent features of the same licensed software.

## Where the discovery mechanisms are configured

The environment variable names and the near-module lookup flag are defined per-project in
`licensecc_properties.h`:

- `FIND_LICENSE_NEAR_MODULE` (look next to the executable — default `true`)
- `FIND_LICENSE_WITH_ENV_VAR` (enable the environment-variable lookups — default `false`)
- `LCC_LICENSE_LOCATION_ENV_VAR` (`LICENSE_LOCATION`)
- `LCC_LICENSE_DATA_ENV_VAR` (`LICENSE_DATA`)
- `LCC_LICENSE_FILE_EXTENSION` (`.lic`)

See the [extension points](../api/extend) reference for details on editing that generated file.
