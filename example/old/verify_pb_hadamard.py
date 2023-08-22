all_orig_lines = []
all_wiki_lines = []
with open("H_16 copy.txt", "r") as orig, open("H_16_Wikipedia.txt") as wiki:

    for line in orig:
        line = line.split(sep=",")
        line[-1] = line[-1].replace("\n", "")
        all_orig_lines.append(line)
        fold_line = line
        for i, num in enumerate(fold_line):
            if num == "-1":
                fold_line[i] = "1"
            if num == "1":
                fold_line[i] = "-1"
        all_orig_lines.append(fold_line)
    for line in wiki:
        # temp = line[-1].replace("\n", "")
        line = line.split(sep=",")
        line[-1] = line[-1].replace("\n", "")
        all_wiki_lines.append(line)
        fold_line = line
        for i, num in enumerate(fold_line):
            if num == "-1":
                fold_line[i] = "1"
            if num == "1":
                fold_line[i] = "-1"
        all_wiki_lines.append(fold_line)


all_orig_lines.sort()
all_wiki_lines.sort()
# print(all_orig_lines)

print(all_wiki_lines)

# diff = []
# for orig_line, wiki_line in zip(all_orig_lines, all_wiki_lines):
#     if orig_line != wiki_line:
#         diff.append(orig_line)
# print(diff)
