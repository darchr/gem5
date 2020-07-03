from m5.util import convert

import unittest

class FrequencyTester(unittest.TestCase):

    def test_GHz(self):
        self.assertAlmostEqual(convert.toFrequency('1GHz'), 1e9)

class LatencyTester(unittest.TestCase):

    def test_ns(self):
        self.assertAlmostEqual(convert.toLatency('1ns'), 1e-9)

    def test_s(self):
        self.assertAlmostEqual(convert.toLatency('15s'), 15.0)

if __name__=="__m5_main__":
    unittest.main(module=None)
