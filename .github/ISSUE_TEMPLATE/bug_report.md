---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

## Summary
**<Give a summary of the bug you're reporting>**

## Checklist before posting
Note: fill in [x] below after doing each of the following.

- [ ] I have searched previous questions in this forum to see if the bug is already reported
- [ ] I have searched the mailing list for an answer to my question
- [ ] I have checked [Gerrit](https://gem5-review.googlesource.com/) to check if someone has already fixed this bug
- [ ] I have completely filled out this form
- [ ] I have included the exact command line and all runscripts required to reproduce the bug
- [ ] I have included any extra inputs necessary (e.g., links to disk images, binaries, etc.)

## Details of your set up
Fill in the answers below.
- My current gem5 commit is: **<Insert commit hash here, ensure it is *from the master branch on googlesource* (a link is even better)>**
- I have modifications on top of the gem5 changeset linked above: **<yes/no>**
- The question/problem occurs on the mainline master branch of gem5: **<yes/no>**
- I am using **\<full system or syscall emulation>**
- I am using **\<timing mode or atomic mode>**
- I am using **\<x86 or ARM or RISC-V or other>**

## To reproduce
In order for us to fix this bug or to help you fix the bug, we must be able to reproduce the bug.
Please provide us will all of the required information to reproduce the bug.
This includes runscripts/config scripts, exact command lines, and any other inputs including binaries, disk images, and kernels when using FS mode.

### Steps to reproduce

- Command: **\<this is the *exact* command used>**
- Runscript: **\<this could be se.py, fs.py, or a custom script. If it is a custom script, upload the script or provide a link>**
- Other inputs:
  - Binary: **\<a link to the binary, or upload>**
  - Disk image: **\<a link to the disk image *do not upload with this report!*>**
  - Kernel binary: **\<a link to the kernel binary *do not upload with this report!*>**

### Expected behavior
**\<This is a description of what you expected to happen>**

### Observed behavior
**\<This is the incorrect behavior you observed>**

## Additional information
**\<Here, you can put any additional information>**

Do *not* post screenshots of terminals!
Copy-paste the *relevant details* if you need.
Alternatively, you can upload the stdout/stderr and attach it to this report.

