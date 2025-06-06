git clone https://github.com/darchr/gem5.git

cd gem5

git checkout mb/saga-cache-v0

scons build/ARM/gem5.opt -j50 --without-tcmalloc  --linker=gold

# build/ARM/gem5.opt -re --debug-flags=ProtocolTrace,MBRUBY --debug-start=10400102795250 --outdir=m5out-restore128core/NPB-C/bt saga-chi-npb-gapbs/configs/npb-restore-tree.py --benchmark=bt --size=C
# build/ARM/gem5.opt -re --debug-flags=ProtocolTrace,MBRUBY --debug-start=10229968769000 --outdir=m5out-restore128core/NPB-C/ep saga-chi-npb-gapbs/configs/npb-restore-tree.py --benchmark=ep --size=C
# build/ARM/gem5.opt -re --debug-flags=ProtocolTrace,MBRUBY --debug-start=27101563610000 --outdir=m5out-restore128core/NPB-C/ft saga-chi-npb-gapbs/configs/npb-restore-tree.py --benchmark=ft --size=C
build/ARM/gem5.opt -re --debug-flags=ProtocolTrace,MBRUBY --debug-start=10475578004250 --outdir=m5out-restore128core/NPB-C/sp saga-chi-npb-gapbs/configs/npb-restore-tree.py --benchmark=sp --size=C