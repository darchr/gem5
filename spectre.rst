:Author: Jason Lowe-Power


Showing spectre on gem5
-----------------------

Build spectre.c

.. code-block:: sh

    gcc spectre.c -o spectre


Note: This seems to work best with gcc4.8.

.. code-block:: sh

    dockerrun powerjg/gem5-build-gcc48 gcc spectre.c -o spectre -static -std=c99


I've included an output from the O3 pipeline viewer in ``O3-pipeview-example.txt``.
The object code that generated this trace can be found in spectre.s.

Finally, to generate the pipeline view, use something like the following.

.. code-block:: sh

    build/X86/gem5.opt --debug-flags=O3PipeView --debug-file=pipeout --debug-start=924810000 configs/learning_gem5/part1/two_level.py ../../meltdown/spectre

    ./util/o3-pipeview.py -w 175 --color m5out/pipeout -o tage-pipeview.out


Note: The branch predictor you use is quite important.
Also, it seems like small changes to the object file make significant differences in the performance of the attack.
