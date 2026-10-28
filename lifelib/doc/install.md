
Installation notes
------------------

If you are including `lifelib` as part of a project, it does not necessarily
need 'installing' per se. Instead, you can clone `lifelib` into your
project's directory:

    cd path/containing/your/project
    git clone https://gitlab.com/apgoucher/lifelib.git

It can be accessed from with a Python script using:

    import lifelib

or from within a C++11 program using (for example):

    #include "lifelib/pattern2.h"

If you want to install `lifelib` so that it's available on your system for
you to access anywhere (such as from a Python script, module, or even a
Jupyter notebook), then run:

    pip install --user --upgrade python-lifelib

to download the latest source distribution from PyPI. The --user argument is
to ensure that it is installed under your home directory, rather than system
wide, because `lifelib` relies on the ability to create and compile its own
source code.

Using lifelib in Windows Python
-------------------------------

If you install `lifelib` into a native Windows Python distribution, such as
Anaconda, then you need to run the following line of code. (If you used the
installation script in the 'quick start' section, then this will have already
been done for you.)

    import lifelib
    lifelib.install_cygwin()

This ensures that Cygwin and all required packages are installed into a
subtree of the `lifelib` package directory (within your user site-packages
directory). Approximately one gigabyte of disk space will be consumed when
you run this for the first time.

Apart from requiring this one-time command, there is no difference between
running `lifelib` in Windows or POSIX.
