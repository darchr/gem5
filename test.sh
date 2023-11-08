build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/bc    configs-npb-gapbs-chkpt-restore/checkpoint_both.py bc    22 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/bfs   configs-npb-gapbs-chkpt-restore/checkpoint_both.py bfs   22 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/cc    configs-npb-gapbs-chkpt-restore/checkpoint_both.py cc    22 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/pr    configs-npb-gapbs-chkpt-restore/checkpoint_both.py pr    22 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/sssp  configs-npb-gapbs-chkpt-restore/checkpoint_both.py sssp  22 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/gapbs/22/tc    configs-npb-gapbs-chkpt-restore/checkpoint_both.py tc    22 &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/bt       configs-npb-gapbs-chkpt-restore/checkpoint_both.py bt    C  &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/cg       configs-npb-gapbs-chkpt-restore/checkpoint_both.py cg    C  &
# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/ft       configs-npb-gapbs-chkpt-restore/checkpoint_both.py ft    C  &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/is       configs-npb-gapbs-chkpt-restore/checkpoint_both.py is    C  &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/lu       configs-npb-gapbs-chkpt-restore/checkpoint_both.py lu    C  &
# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/mg       configs-npb-gapbs-chkpt-restore/checkpoint_both.py mg    C  &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/sp       configs-npb-gapbs-chkpt-restore/checkpoint_both.py sp    C  &
## build/X86_MESI_Two_Level/gem5.opt -re --outdir=cpt-resub/8ch/chkpt-atomic/npb/C/ua       configs-npb-gapbs-chkpt-restore/checkpoint_both.py ua    C  &

# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cptTest/chkpt-timing-gap configs-npb-gapbs-chkpt-restore/checkpoint_both.py bfs 22 &
# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cptTest/chkpt-timing-npb configs-npb-gapbs-chkpt-restore/checkpoint_both.py bt  C &

# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cptTest/chkpt-atomic-gap configs-npb-gapbs-chkpt-restore/checkpoint_both.py bfs 22 &
# build/X86_MESI_Two_Level/gem5.opt -re --outdir=cptTest/chkpt-atomic-npb configs-npb-gapbs-chkpt-restore/checkpoint_both.py bt  C &

# build/X86_MESI_Two_Level/gem5.opt -re --debug-flags=ChkptRstrTest --outdir=rstrTest/gap/bfs-22-dirmap configs-npb-gapbs-chkpt-restore/restore_both.py bfs 22 RambusTagProbOpt 1 0 0 0 &
# build/X86_MESI_Two_Level/gem5.opt -re --debug-flags=ChkptRstrTest --outdir=rstrTest/npb/bt-C-dirmap   configs-npb-gapbs-chkpt-restore/restore_both.py bt  C  RambusTagProbOpt 1 0 0 0 &
