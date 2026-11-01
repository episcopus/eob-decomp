# Review: <unit id>

Reviewer: <your agent name>
Commit reviewed: <git rev-parse HEAD on main when you started>

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix <file:line>: <what is wrong> (<evidence: an address, a caller, a data file>)
- [ ] should <file:line>: <what would be clearer>
- [ ] note <anything the next reader should know>

## Summary

<one paragraph: is the unit understandable from its source alone; what names are still placeholders
and why (no evidence yet is a fine reason)>
