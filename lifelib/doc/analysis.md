
Analysing patterns
------------------

As well as editing functionality, patterns in `lifelib` have various
properties that give basic information:

    lidka.population # returns population count
    lidka.bounding_box # returns bounding box

If a pattern is periodic (an oscillator or spaceship), you can additionally
use the following further properties:

    pattern.period
    pattern.displacement
    pattern.apgcode

Provided you have an Internet connection, you can download sample soups
that generate the pattern:

    samples = pattern.download_samples()

This queries [Catagolue](https://catagolue.appspot.com/home) for the seeds
which have been reported by [apgsearch](https://gitlab.com/apgoucher/apgmera)
to generate the pattern in question.

Loading/saving files
--------------------

Both the macrocell and RLE file formats from Golly are supported by `lifelib`
for both reading and writing files; moreover, they have been generalised to
support up to $`2^{64}`$ states. The Python version of `lifelib` also allows
the reading and writing of compressed files (.rle.gz and .mc.gz). The easiest
way to use this is:

    lidka.save('lidka.rle')
    lidka_reloaded = lt.load('lidka.rle')

Unless otherwise specified, file format is inferred from the extension.

Jupyter notebook support and LifeViewer integration
---------------------------------------------------

If you are running `lifelib` in Python from a Jupyter notebook, you can view
a pattern in Chris Rowett's LifeViewer by calling the pattern's `.viewer()`
method:

    lidka.viewer()

Note that, for this to work, you need to be viewing the notebook from a
browser with Internet access; the LifeViewer JavaScript plugin is sourced
from `conwaylife.com`.

In Internet Explorer, the lack of support for data URIs means that you need
to use the antiquated option:

    lidka.viewer(base64=False)

which is worse because 'downloading' the notebook as HTML does not preserve
the embedded LifeViewers in the latter case.
