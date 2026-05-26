#!/bin/bash

# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if (( $# != 1 )); then
    echo "USAGE: $0 proposal" >&2
    exit 1
fi

in="$1"
name=$(grep 'Document number' "$in" | grep -oP 'P\d+R\w+')
out="$name".html

# Generate HTML from Markdown (title from YAML front matter)
sed "s/@DATE@/$(date +%F)/g" "$in" | pandoc - -f markdown -t html5 -s \
  --css=css/proposal-style.css \
  --embed-resources --standalone \
  -o "$out"

# Post-process: Convert spec clauses to definition lists with 3 columns
# Match only whitespace and nbsp between number and clause name
perl -i -C -0pe 's|<p><strong>(\d+)</strong>[\s\x{00A0}]*<em>(Constraints\|Preconditions\|Effects\|Postconditions\|Returns\|Throws\|Error conditions)</em>:\s*(.*?)</p>|<dl class="spec-clause"><dt class="num"><strong>$1</strong></dt><dt class="clause"><em>$2</em>:</dt><dd>$3</dd></dl>|gs' \
  "$out"

# Post-process: Convert error condition items to grid layout
# Match paragraphs starting with (N.M)
perl -i -0pe 's|<p>\((\d+\.\d+)\)\s+—\s+(.*?)</p>|<p class="error-item"><span class="error-num">(\1)</span><span class="error-content">— \2</span></p>|gs' \
  "$out"

echo "$out"
