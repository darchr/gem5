# build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cptTest/npb/bt4     --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/checkpoint_both.py  bt C  RambusTagProbOpt &
# build/X86_MESI_Two_Level/gem5.opt --outdir=cptTest/gapbs/bfs13  configs-npb-gapbs-archive/gapbs_checkpoint.py bfs 22 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=cptTest/gapbs/bfs15-single --debug-flags=ChkptRstrTest configs-npb-gapbs-chkpt-restore/checkpoint_both.py bfs 22
# build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cptTest/gapbs/bfs8  --debug-flags=PolicyManager  configs-npb-gapbs-archive/gapbs_checkpoint.py bfs 22  RambusTagProbOpt 0 0 &

# build/X86_MESI_Two_Level/gem5.debug -re  --outdir=rstrTest/npb/bt3     --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/restore_both.py  bt C  RambusTagProbOpt 1 0 0 0 &
# build/X86_MESI_Two_Level/gem5.debug -re  --outdir=rstrTest/gapbs/bfs3  --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/restore_both.py bfs 22  RambusTagProbOpt 1 0 0 0 &