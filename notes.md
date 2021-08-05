

# Make clang-format ignore bitunions
grep -r -l BitUnion src | xargs sed -i -e '/^ *BitUnion.*(.*)/i \
// clang-format off' -e '/^ *EndBitUnion(.*)/a; \
// clang-format on'

find src -name "*.cc" -or -name "*.hh" -exec /home/jlp/.vscode/extensions/ms-vscode.cpptools-1.5.1/bin/../LLVM/bin/clang-format -style=file -i {} \;

# Check to see how I've done
find src -name "*.cc" -or -name "*.hh" -exec util/style.py -f -c ControlSpace -c SortedIncludes -c StructureBraces -c Whitespace {} \;

# Remove clang-format off in BitUnion files
grep -r -l BitUnion src | xargs sed -i -e '/^\/\/ clang-format.*$/d' -e '/^;$/d'

## To do


# fix src/arch/x86/regs/int.hh manually
# bitunion.hh is a mess!
