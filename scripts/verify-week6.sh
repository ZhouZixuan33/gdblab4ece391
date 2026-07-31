#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
labs="
week6-thread/lab16-context-switch
week6-thread/lab17-scheduler-lifecycle
week6-thread/lab18-race-lock-irq
"

for lab in $labs; do
    echo "== ${lab}"
    make -C "${ROOT}/${lab}" clean
    make -C "${ROOT}/${lab}" MODE=solution check
    make -C "${ROOT}/${lab}" MODE=exercise check
    make -C "${ROOT}/${lab}" MODE=solution run-check
done

echo "Week 6 verification passed."
