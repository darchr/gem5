
from .state import *
from .test import *
from .suite import *
from .runner import *
from .loader import *
from .fixture import *
from .config import *
from main import main

#TODO Remove this awkward bootstrap
#FIXME
from gem5 import *

#TODO Remove this as an export, users should getcwd from os
from os import getcwd