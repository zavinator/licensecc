#############################
Glossary
#############################

Some `common` words that recur in the documentation have a specific meaning. Below an
explanation of the terms and concepts used.

.. glossary::

    Project
        A project in ``licensecc`` terminology usually corresponds to one of your applications
        where the licensing system needs to be integrated. For instance if you want to release
        the executable ``Foo.exe`` you may want to create a project ``Foo`` in licensecc.

        A ``licensecc-project`` is the set of customizations needed to compile the licensecc
        library for your own application:

        (1) a private key used to issue licenses,
        (2) a public key (included at compile time in the licensecc library) used to verify licenses,
        (3) a set of build parameters and customizations specific to your application.

        One source code branch of licensecc may "serve" multiple projects: every time you build
        you specify which project to compile and the build system selects the right include
        files (e.g. :file:`public_key.h`).

    Features
        Features let the licensed application selectively enable or disable functions in the
        application.

    Hardware identifier
        A short string generated from the machine where the licensed software runs. It is
        communicated back to the software publisher to issue a license bound to that specific
        machine. See the :ref:`hardware identifiers <usage/Hardware-identifiers:Hardware Identifiers>`
        user guide.

    lccgen
        The license generator executable, also used as a project configuration tool. It signs
        license files with a project's private key.

    PC signature
        Synonym of *hardware identifier*.

    Locator strategy
        A class implementing :cpp:class:`license::locate::LocatorStrategy` that tells
        ``licensecc`` where to look for a license file. See
        :ref:`find the license file <usage/find-the-license:Find the license file>`.
