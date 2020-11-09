* Created on: Oct 28, 2020 *
* Author: mala *


<h1>***PCOTest***</h1>

<h2> **Ubuntu installation:**</h2>

If the computer does not have linux installed, please first install linux.
Ubuntu 16.04 is the recommended distribution. We also have tested with 18.04. 


<h2> **Boost instalation:**</h2>

The current test system works with boost program options library to read the configuration file. 

<h3> *get boost:*</h3>

https://www.boost.org/users/history/version_1_73_0.html
tar --bzip2 -xf /path/to/boost _tar_file

<h3> *build program_options library* </h3>

$ cd path/to/boost_1_73_0
$ ./bootstrap.sh  --with-libraries= program_options

you can change the path if you wish with
$ ./bootstrap.sh --prefix=path/to/installation/prefix

<h3> *build the library* </h3>
./b2 install --with-program_options

it is by default built in /usr/local/lib/, unless you change the path.
In case you change the path, please update the cmakeLists file and add the path there.

<h2>**PCO and SISO libraries Installation:**</h2>

please follow the guidelines in readme for this step.

<h2>**Flash the framegrabber:**</h2>

Use the miroDiagnistics tool to flash the framegrabber with the applet. The instructions are in the readMe file.

Build the code and run:

Use the readMe file.

<h2>**Troubleshooting:**</h2>

When the program runs always stop with q or ctrl+c. Otherwise the frame grabber gets locked and does not run again.
If that happens power cycle the system, and wait for 1 minute to start the systm again.

If the OS updates in the meantime, the siso driver should be built and loaded again. This is mentioned in the driver installation guidelines.


<h2>**Configuration options:**</h2>

right now, the program reads the path to configuration file from command line.
The cnfiguration file is a simple .ini text file with <keyword>=<value> structure. 
you can find the keywords by running ./pcoTest --help.

To change the configuration options use the Configuration.cpp definition. 


</h2>**Centroiding filters:**</h2>

There are several values to set for centroiding thresholds. There is a description in Description Algorithm and API document.


***This document is never finalized until we launch!***






