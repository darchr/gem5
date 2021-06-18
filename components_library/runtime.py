

from m5.defines import buildEnv

from .isas import ISA
from .coherence_protocol import CoherenceProtocol

def get_runtime_isa() -> ISA:
    """Gets the target ISA.
    This can be inferred at runtime.

    :returns: The target ISA.
    """
    isa_map = {
        "sparc": ISA.SPARC,
        "mips": ISA.MIPS,
        "null": ISA.NULL,
        "arm": ISA.ARM,
        "x86": ISA.X86,
        "power": ISA.POWER,
        "riscv": ISA.RISCV,
    }

    isa_str = str(buildEnv["TARGET_ISA"]).lower()
    if isa_str not in isa_map.keys():
        raise NotImplementedError(
            "ISA '" + buildEnv["TARGET_ISA"] + "' not recognized."
        )

    return isa_map[isa_str]

def get_runtime_coherence_protocol() -> CoherenceProtocol:
    """Gets the cache coherence protocol.
    This can be inferred at runtime.

    :returns: The cache coherence protocol.
    """
    protocol_map = {
        "mi_example": CoherenceProtocol.MI_EXAMPLE,
        "moesi_hammer": CoherenceProtocol.ARM_MOESI_HAMMER,
        "garnet_standalone": CoherenceProtocol.GARNET_STANDALONE,
        "moesi_cmp_token": CoherenceProtocol.MOESI_CMP_TOKEN,
        "mesi_two_level": CoherenceProtocol.MESI_TWO_LEVEL,
        "moesi_amd_base": CoherenceProtocol.MOESI_AMD_BASE,
        "mesi_three_level_htm": CoherenceProtocol.MESI_THREE_LEVEL_HTM,
        "mesi_three_level": CoherenceProtocol.MESI_THREE_LEVEL,
        "gpu_viper": CoherenceProtocol.GPU_VIPER,
        "chi": CoherenceProtocol.CHI,
    }

    protocol_str = str(buildEnv["PROTOCOL"]).lower()
    if protocol_str not in protocol_map.keys():
        raise NotImplementedError(
            "Protocol '" + buildEnv["PROTOCOL"] + "' not recognized."
        )

    return protocol_map[protocol_str]