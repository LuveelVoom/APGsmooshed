
Quick start
-----------

Make sure your machine has the correct system requirements before commencing.
This essentially boils down to you having an **x86-64 processor** (likely to
be the case unless you're using a smartphone, tablet, or Raspberry Pi). Then
getting started with `lifelib` is straightforward.

 - Install Python with the `numpy` and `jupyter` packages and the 'pip'
   package manager. The [Anaconda](https://www.anaconda.com/download)
   distribution contains those packages and many more, and is available
   on Windows, Mac OS X, and Linux. You can use either Python 2 or 3.

 - If on Mac OS X, make sure you have the **command-line developer tools**
   installed. You can test this by opening a terminal and running the
   command `g++ --version`.

 - Download and run the [installation script][1] to install or upgrade
   `lifelib`. Internally, this uses the pip package manager and installs
   `lifelib` into a user-local directory.

 - Open a Jupyter notebook (e.g. by opening Anaconda Prompt and typing
   `jupyter notebook`) and execute the following commands. They are explained
   in the 'example usage' section later in this document:

        import lifelib
        sess = lifelib.load_rules("b3s23")
        lt = sess.lifetree()
        lidka = lt.pattern("bo7b$obo6b$bo7b8$8bo$6bobo$5b2obo2$4b3o!")
        print("Initial population: %d" % lidka.population)
        lidka_30k = lidka[30000]
        print("Final population: %d" % lidka_30k.population)
        lidka.viewer()

 - Try some of the [example notebooks][2] to familiarise yourself with
   `lifelib` usage. Other features are documented in this README file.

[1]: https://gitlab.com/apgoucher/python-lifelib/blob/master/install/install.py
[2]: https://gitlab.com/apgoucher/python-lifelib/tree/master/notebooks

![](images/screenshot.png)

