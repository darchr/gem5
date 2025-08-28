from m5.params import *
from m5.objects.ClockedObject import ClockedObject


class EncoderDecoder(ClockedObject):
    type = "EncoderDecoder"
    cxx_header = "accl/graph/sega/encoder_decoder.hh"
    cxx_class = "gem5::EncoderDecoder"

    # Parameters
    bit_width = Param.Unsigned(3, "Bit width (3 or 6).")
    is_encoder = Param.Bool(True, "True = encoder, False = decoder.")
    dual_output = Param.Bool(
        True, "For 6-bit: two temporal outputs in parallel."
    )
    cycles_per_op = Param.Unsigned(
        0,
        "If nonzero, overrides default cycles.",
    )
