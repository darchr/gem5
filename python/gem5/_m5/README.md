# Notes

To generate the .pyi files, you can use stubgen from mypy.

Due to a [bug](https://github.com/python/mypy/issues/7692) in mypy, you may
need to install the development version from github. This was
[merged recently](https://github.com/python/mypy/pull/8888).

Once mypy is installed, you can run gem5 with a script to manually run stubgen to generate the files.

```python
# This file is adapted from stubgen

import sys
from mypy.stubgen import main

if __name__=="__m5_main__":
    sys.exit(main())
```

```sh
gem5.opt gem5_stubgen.py -p _m5
```
