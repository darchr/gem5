# build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cptTest/npb/bt3     --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/checkpoint_both.py 0  bt C  RambusTagProbOpt &
# build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cptTest/gapbs/bfs3  --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/checkpoint_both.py 1 bfs 22  RambusTagProbOpt &

# build/X86_MESI_Two_Level/gem5.debug -re  --outdir=rstrTest/npb/bt3     --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/restore_both.py 0  bt C  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.debug -re  --outdir=rstrTest/gapbs/bfs3  --debug-flags=ChkptRstrTest  configs-npb-gapbs-chkpt-restore/restore_both.py 1 bfs 22  RambusTagProbOpt 1 0 0 0 &