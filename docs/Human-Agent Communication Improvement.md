## Human+Agent Communication Improvements

Here are better ways to formulate your requests that might get more reliable results:

More Effective Approaches:
1. Explicit Copy-Paste Language
Instead of: "duplicate the A logic for B"
Try: "Copy the entire A block and paste it as B, changing only the variable names from _var to _var_b"

2. Line-by-Line Instructions
"Take lines X-Y from the A section, copy them exactly, and paste them in the B section with only these specific variable name changes: [list them]"

3. Diff-Style Requests
"Make the B section identical to A except change _frequency → _frequency_b, wavegen → wavegen_b"

4. Verification Requirements
"After making changes, show me a side-by-side comparison of the A and B sections to verify they're identical except for the specified variable names"

5. Explicit "No Logic Changes" Instruction
"Do NOT modify any logic, conditionals, or flow - only change variable names as specified"

The root issue seems to be that I try to be "helpful" by making what I think are improvements or corrections during duplication, which breaks the exact copy requirement. Your explicit instruction to duplicate exactly should override any tendency to modify logic.