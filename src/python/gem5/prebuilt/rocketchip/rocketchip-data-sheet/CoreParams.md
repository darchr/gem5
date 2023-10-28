# RocketChip Core

## Integer Instructions

Usually, integer instructions should take one cycle. I have not seen information about the latency of integer instructions in the RocketChip documentation, but let's assume that it is one cycle, and fine-tune it later.

``` python
class RocketIntAluFU(MinorDefaultIntAluFU):
    opLat = 1
```

## Multiplication and Division Instructions

It looks like this is the code for Multiplication and Division latencies:

``` scala
case class MulDivParams(
  mulUnroll: Int = 1,
  divUnroll: Int = 1,
  mulEarlyOut: Boolean = false,
  divEarlyOut: Boolean = false,
  divEarlyOutGranularity: Int = 1
)

class MulDiv(cfg: MulDivParams, width: Int, nXpr: Int = 32, aluFn: ALUFN = new ALUFN) extends Module {
  private def minDivLatency = (cfg.divUnroll > 0).option(if (cfg.divEarlyOut) 3 else 1 + w/cfg.divUnroll)
  private def minMulLatency = (cfg.mulUnroll > 0).option(if (cfg.mulEarlyOut) 2 else w/cfg.mulUnroll)
```

The value of `w` needs some calculation which seems a bit convoluted for me to figure out the exact value.

I would recommend setting `mulEarlyOut` and `divEarlyOut` both to `true`.

This would make the Core parameters like this in gem5:

``` python
class RocketIntMulFU(MinorDefaultIntMulFU):
    opLat = 2


class RocketIntDivFU(MinorDefaultIntDivFU):
    opLat = 3
```

## Memory Instructions

RocketChip does not give a latency for memory instructions.

However, it does give information about miss penalty. Source: [CS152 Laboratory Exercise 2](https://inst.eecs.berkeley.edu/~cs152/sp18/handouts/lab2-0.2.pdf)

| Microarchitectural Event  | Miss Penalty |
| ---------- | ------------ |
| L1 Cache miss | 23 cycles (L2 cache access latency) |
| L2 Cache miss | 100 cycles (DRAM access latency) |
| L1 TLB miss | 2 cycles |
| L2 TLB miss | 3×(2/3×1+1/3×123) cycles |
| Branch misprediction | 3 cycles |
| Target address misprediction | 3 cycles |

So, through the above miss penalty information, we can calculate the latency of memory instructions.

``` python
class RocketLoadStoreFU(MinorDefaultLoadStoreFU):
    opLat = 42
```

This value feels a bit high, but I am not sure how to calculate it. This definitely needs more investigation.

## Other FU Pool Parameters

These should be the other parameters of the FU Pool:

``` python
class RocketFloatSimdFU(MinorDefaultFloatSimdFU):
    pass

class RocketPredFU(MinorDefaultPredFU):
    pass

class RocketMiscFU(MinorDefaultMiscFU):
    pass

class RocketVecFU(MinorDefaultVecFU):
    pass
```

## Rocket Branch Predictor

Here are the details for the [branch predictor](https://github.com/chipsalliance/rocket-chip/blob/master/src/main/scala/rocket/BTB.scala):

``` scala
case class BHTParams(
  nEntries: Int = 512,
  counterLength: Int = 1,
  historyLength: Int = 8,
  historyBits: Int = 3)

case class BTBParams(
  nEntries: Int = 28,
  nMatchBits: Int = 14,
  nPages: Int = 6,
  nRAS: Int = 6,
  bhtParams: Option[BHTParams] = Some(BHTParams()),
  updatesOutOfOrder: Boolean = false)
```

A gem5 equivalent would be:

``` python
class RocketBP(TournamentBP):
    BTBEntries = 28
    RASSize = 6
    localHistoryTableSize = 256
    localPredictorSize = 1024
    globalPredictorSize = 1024
    choicePredictorSize = 1024
    localCtrBits = 1
    globalCtrBits = 1
    choiceCtrBits = 2
    indirectBranchPred = SimpleIndirectPredictor()
    indirectBranchPred.indirectSets = 6
```

I picked the tournament BP because of the existence of the history table.
