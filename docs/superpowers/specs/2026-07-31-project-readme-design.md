# ECE391 Project README Design

## Purpose

Redesign the repository root `README.md` for students encountering ECE391 for
the first time. The README must present the repository as preparation for
systems programming in ECE391 rather than as a collection of GDB exercises.
GDB and QEMU remain important, but they are framed as tools for observing and
debugging low-level behavior.

## Audience

The primary reader is a student who has not started ECE391 and may not yet
understand how debugging, assembly, interrupts, threads, virtual memory,
processes, and system calls fit together.

The README should therefore:

- define the project's purpose before introducing commands;
- provide a clear prerequisite and setup path;
- explain why the weeks appear in their chosen order;
- distinguish completed material from work in progress;
- give the reader one unambiguous place to start.

## Content Strategy

Use a curriculum-first structure:

1. Project title and concise value proposition
2. Scope and learning outcomes
3. Curriculum roadmap
4. Prerequisites
5. Environment setup
6. Quick start
7. Recommended lab workflow
8. Verification commands
9. Repository resources and course-material attribution

The opening must explicitly state that the repository prepares students for
ECE391 systems programming. It must also explain that GDB and QEMU are
diagnostic tools, not the main subject of the project.

## Curriculum Roadmap

Present all eight weeks in learning order:

1. GDB and debugging fundamentals
2. Memory, stack, pointer, and core-dump analysis
3. Makefiles, ELF symbols, RISC-V calling conventions, and assembly
4. Bare-metal execution with QEMU and remote GDB
5. Privilege levels, CSRs, traps, and interrupts
6. Kernel threads, scheduling, synchronization, and interrupt-safe critical
   sections
7. Virtual memory
8. Processes and system calls

Weeks 7 and 8 must remain visible in the roadmap and be clearly marked
`In Progress`. Their current lecture material may be mentioned without
implying that runnable labs or verification scripts already exist.

Each completed week should link to its directory-level README. Weeks without a
directory-level README should link only when there is a useful existing entry
point, and their status must remain explicit.

## Setup and Usage

Use Ubuntu as the supported environment and group packages by purpose:

- native C build and debugging tools;
- RISC-V cross-compilation tools;
- QEMU and multi-architecture GDB.

Keep setup concise and link to the detailed files under `setup/`. The quick
start should direct a new student to Week 1, Lab 1. Verification commands
should include only scripts that currently exist: Weeks 1, 2, 3, 4, and 6.
The missing Week 5 verification script must not be implied.

Explain a repeatable learning loop:

1. read the goal and failure scenario;
2. predict the relevant machine state;
3. build and reproduce the behavior;
4. inspect it with the appropriate debugging tools;
5. explain the root cause in systems terms;
6. complete review or challenge questions;
7. run the available check.

## Tone and Presentation

The README remains entirely in English. Use approachable, technically precise
language and avoid assuming prior operating-systems knowledge. Prefer short
sections, tables, and direct links over long prose.

Avoid:

- presenting command memorization as the learning objective;
- describing unfinished weeks as complete;
- claiming official UIUC or ECE391 affiliation;
- duplicating detailed lab instructions already maintained in weekly or lab
  READMEs;
- changing files outside the root README during implementation.

## Acceptance Criteria

The redesigned root README is complete when:

- its title and introduction identify ECE391 preparation as the project goal;
- interrupts, virtual memory, threads, processes, and system calls appear in
  the project scope;
- GDB and QEMU are described as debugging and observation tools;
- a first-time student can identify prerequisites, setup steps, the first lab,
  and the recommended learning order;
- Weeks 7 and 8 are visible and marked `In Progress`;
- all repository-relative links and listed verification scripts exist;
- all text is English;
- the README does not overstate completion or official course affiliation.
