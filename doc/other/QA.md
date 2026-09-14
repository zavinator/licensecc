# Q&A

## Development
Development related questions.

### Why do i have to build from source? Can you release a pre-compiled version of the library? 
No we can't release pre-compiled binaries of the library.
In the early phases of the build the library is customized: among other things a private and public key pair is 
generated and the public key is included into the compiled code, in a way that every project gets its own signature keys. 
See [Development and Usage](../analysis/Development-And-Usage-Workflow)

### Help! it doesn't compile!
Every `licensecc` release is tested in the following environments:

* Ubuntu 22.04-26.04 (x86_64 and ARM, GCC and Clang, boost 1.74~1.90, openssl 3.x)
* Windows server 2022-2025 (MSVC, boost 1.71+)
* Windows 11 ARM (MSVC)

Tests are carried out automatically every commit. You can see the current build status in the
[GitHub Actions](https://github.com/open-license-manager/licensecc/actions) tab of the repository.

If you can't get it compiling: 

* be sure to follow the compilation instructions for [Windows](../development/Build-the-library-windows) or [Linux](../development/Build-the-library).
* post a question on the [user forum](https://github.com/open-license-manager/licensecc/discussions)

### Help! it used to compile but now it gives me error.
Updating `licensecc` may pose some issue, since it's still in alpha stage and we may take non-backward compatible changes.
We suggest after every update, if you're building from `develop` to clean the `build` folder and run the cmake configuration from beginning.
If this is not enough, move/backup the contents of your `projects` folder. Let `licensecc` to generate it new. Later
you need to merge the newly generated folder with the one you saved.


## Project status

For the current maintenance status, what to expect, and how you can help, see the
[Maintenance Status](maintenance) page.

### Is it production ready?
This project started as a week-end project in 2014, donated and almost forgotten, then kept alive
and refreshed thanks to its users. It is still experimental in spirit: it's usable with some effort,
and some companies are actively using it in production to issue demo licenses and a few production
licenses. If you plan to use it in a scenario where you need to issue hundreds of licenses plan some
maintaining effort, or better use a more mature/commercial licensing software.

### Licensecc and "the community"
`Licensecc` has a controversal relationship with "the community", especially StackExchange/StackOverflow. 
I've got technical questions canceled a couple of times, some other got hasty comments, some other were hijacked like: "you can't do that!". I've found that out there, there are a lot of people that think that all the software should be licensed as [AGPL](https://en.wikipedia.org/wiki/GNU_Affero_General_Public_License). 
StackOverflow and its fellow sites turned out to be not such a collaborative place to talk about software protection. If you post there remember to choose your words carefully. :speak_no_evil: