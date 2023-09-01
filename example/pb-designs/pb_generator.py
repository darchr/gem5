pb_16_generator = [
    "+",
    "+",
    "+",
    "+",
    "-",
    "+",
    "-",
    "+",
    "+",
    "-",
    "-",
    "+",
    "-",
    "-",
    "-",
]

pb_design_transpose = []
matrix_dim = 15

curr_col = pb_16_generator
for j in range(0, matrix_dim):  # access column
    curr_col = pb_16_generator[matrix_dim - j :]
    # print(curr_col)
    curr_col = curr_col + pb_16_generator[: matrix_dim - j]
    # print(curr_col)
    pb_design_transpose.append(curr_col)

# print(pb_design_transpose)
pb_design = []
for i in range(0, matrix_dim):
    row = []
    for j in range(0, matrix_dim):
        row.append(pb_design_transpose[j][i])
    pb_design.append(row)
last_row = []
for j in range(0, matrix_dim):
    last_row.append("-")
pb_design.append(last_row)

# print(pb_design)

with open("PB_16.txt", "w") as text:
    for line in pb_design:
        write_row = ""
        for char in line:
            if char == "+":
                write_row += "1,"
            elif char == "-":
                write_row += "-1,"
        write_row = write_row[0 : len(write_row) - 1]
        text.write(write_row)
        text.write("\n")
