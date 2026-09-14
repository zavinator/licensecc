# Issue licenses

The easiest way to issue licenses is to point `lccgen` at a project folder. A project (a key pair
plus project customizations — see [Projects, features and versions](concepts.rst) for the full
definition) is created for you when you configure the library: a default project named `DEFAULT`
lives in `licensecc/projects`, and you can create more with the `LCC_PROJECT_NAME` CMake variable.

Place the `lccgen` executable in your path (this is the executable needed to issue licenses). It is
compiled together with the library, so you should find it somewhere in your build tree or where you
installed the library.

The lines below will create a perpetual, unlimited license for your software:

```console
cd projects/DEFAULT #(or whatever your project name is)
lccgen license issue -o licenses/{license-file-name}.lic
```

## Licensing software with hardware identifier

To issue a license linked to a specific machine you first need to retrieve a hardware identifier for
it. This is done by running an executable on the destination machine (usually your own software,
which calls the `licensecc` API and prints out the required identifier).

If you are just experimenting with the library you can compile and use the
[examples project](https://github.com/open-license-manager/examples) to print out such a hardware
signature, or you can run `lccinspector` on the destination machine.

Once you have the hardware identifier you can issue the command:

```console
cd projects/DEFAULT #(or whatever your project is)
lccgen license issue --client-signature XXXX-XXXX-XXXX -o licenses/{license-file-name}.lic
```

to create the license file (usually this command is issued on the host machine where you compiled
`licensecc`).

## Full set of options
A good way to start exploring available options is the command: `lccgen license issue --help`

| Parameter          | Description                                                                                  |
|--------------------|----------------------------------------------------------------------------------------------|
|base64,b            | the license is encoded for inclusion in environment variables                                |
|valid-from          | Specify the start of the validity for this license. Format YYYY-MM-DD. If not specified defaults to today. |
|valid-to,e          | The expire date for this license. Format YYYY-MM-DD. If not specified the license won't expire |
|client-signature,s  | The signature of the hardware where the licensed software will run. It should be in the format XXXX-XXXX-XXXX. If not specified the license won't be linked to a specific pc. |
|virtualization-type,t | Limit the execution to a specific virtualization environment. Values: `NONE` (bare metal), `CONTAINER`, `VM`. If not specified the license will run on any environment. |
|output-file-name,o  | License output file path.                                                                    |
|extra-data,x        | Application specific data (max 64 characters by default). They'll be returned when calling the `acquire_license` method |
|feature-names,f     | Comma separated list of features to license. See the [program_features example](https://github.com/open-license-manager/examples/blob/develop/basic_features/program_features.cpp). |
|start-version       | The first version of the software this license applies to.                                   |
|end-version         | The last version of the software this license applies to.                                    |
|project-folder,p    | Path to where project configurations and licenses are stored. Defaults to the current folder. |
|primary-key         | Primary key location, in case it is not in the default folder.                               |
|custom-value        | Custom `key=value` pair to be added to the license. Can be repeated.                         |
|verbose,v           | Turn on verbose output (global option).                                                      |

> **Note:** the CLI help reports dates as `YYYYMMDD`, but the generator's date normalization also
> accepts the friendlier `YYYY-MM-DD` form used in the examples below.

## Examples

All the commands below are issued from the project folder (`cd projects/DEFAULT`).

Perpetual license, no limits:

```console
lccgen license issue -o licenses/perpetual.lic
```

Time-limited license (trial, valid from today until a given date):

```console
lccgen license issue --valid-from 2026-01-01 --valid-to 2026-12-31 -o licenses/trial.lic
```

License linked to a specific pc (hardware identifier retrieved with `lccinspector`
or your own software, see above):

```console
lccgen license issue --client-signature XXXX-XXXX-XXXX -o licenses/licensed_pc.lic
```

License restricted to a virtualization environment (eg. only on bare metal, or only
in a container/VM):

```console
lccgen license issue --virtualization-type NONE -o licenses/bare_metal_only.lic
lccgen license issue --virtualization-type CONTAINER -o licenses/container_only.lic
lccgen license issue --virtualization-type VM -o licenses/vm_only.lic
```

Combining limits (eg. trial linked to a specific pc):

```console
lccgen license issue --valid-to 2026-12-31 --client-signature XXXX-XXXX-XXXX -o licenses/trial_licensed_pc.lic
```

Multi-feature license:

```console
lccgen license issue -f DEFAULT,MY_AWESOME_FEATURE -o licenses/multi_feature.lic
```
