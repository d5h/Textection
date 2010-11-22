Without taking the time to do a full write up, I'd like to explain
what this is briefly.  It uses features bases on "objects" in the
image to do
[OCR](http://en.wikipedia.org/wiki/Optical_character_recognition).  An
object is defined as a group of contiguous pixels of the same color,
basically like doing Paintshop's flood fill for each pixel and taking
the unique set of results.  That algorithm is implemented in
`objfind.cc`.  It could be O(N) where N is the size of the image, but
the code here is a little lazy and has worst-case O(N^2) runtime.
Typically, however, finding the objects runs on the order of a
hundredth of a second on my rather slow laptop.

The features it looks at are defined in `features.h` and
`posfeatures.cc`.  It uses aspect ratio, and a score based on the top
and bottom position of the object relative to other objects in the
image of similar size (because text is usually horizontally aligned).
I had planned features based on the number of holes in an object, and
various measurements of objects at certain points to help distinguish
characters.  Right now it thinks almost everything is a "c", "I", or
"4".  I got busy with more pressing work.

There are four executables.  `train` displays the image and highlights
each object.  The user types the corresponding character or `space` to
skip, `tab` to move on to the next image, or `esc` to exit.  The
training data is saved in an SQLite3 database.  `features` reads each
image in the database and records the value of the above-mentioned
features for each character in the image.  The output is an
[ARFF](http://www.cs.waikato.ac.nz/~ml/weka/arff.html) file.  The ARFF
data is fed to [`multiboost`](http://www.multiboost.org/) (not
included in the code), which runs the multi-class version of AdaBoost
to determine coefficients for the linear combination of features.
`generalize.py` is a Python script which reads the XML output from
multiboost and generates a C++ classifier.  This process is automated
in the Shell script, `build_classifier`.  `detect` links in this file
and uses it to classify objects in the image, discard those that score
lowly, and output the closest character match for the others.

Image preprocessing is done in `image.cc`, which includes pixel and
color downsampling.  Common code dealing with the command line and
SQLite3 is provided in `args.cc` and `sql.*` respectively.

It's hard to say how well this could perform once the correct features
have been found and the training data is sufficient.  I haven't had
time to work on it in a while.
