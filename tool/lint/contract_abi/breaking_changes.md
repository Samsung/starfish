# Delegate contract breaking changes

tool/lint/check_contract_abi.py fails a PR that breaks the UWE delegate
contract (src/public/contract/*.h and the enums/structs it exposes by
value) unless every item it reports has a matching line in this file, added
in the same PR. Each line doubles as the permanent record of an intentional
contract break, so write the reason and a reference (issue/PR number), not
just the key. Do not add a line merely to make the PR pass; reviewers read
each one as evidence that the break was deliberate.

## How to waive: from checker output to a line here

When the checker fails, it prints the breaking items and the exact keys it
expects to find. For example, swapping two virtual methods in
ResourceErrorDelegate.h produces:

```
The delegate contract ABI changed (BREAKING):
  [breaking] vtable-slot :: ResourceError[2] -- ..GetErrorCode.. -> ..GetDescription..
  [breaking] vtable-slot :: ResourceError[3] -- ..GetDescription.. -> ..GetErrorCode..

2 breaking item(s) are not waived in tool/lint/contract_abi/breaking_changes.md:
  add a line naming: ResourceError[2]
  add a line naming: ResourceError[3]
```

Copy every key from the "add a line naming:" lines into a new entry at the
end of this file -- one entry may cover several keys. Note a swap always
moves TWO slots, so both must appear:

```
- ResourceError[2], ResourceError[3]: swapped GetErrorCode/GetDescription  declaration order -- intentional, contract v2 (#1234)
```

Rules the matcher applies:

- Only lines ADDED relative to the PR's merge-base count. Entries already
  in this file from earlier PRs never waive a new break, even for the same
  key -- history below is a record, not a standing exemption.
- A key must appear verbatim (plain substring match): `ResourceError[2]` is
  satisfied by a longer sentence containing it, but not by `ResourceError`
  alone or by an abbreviation like `ResourceError[2-3]`.
- Because matching is plain substring, never write a key you are NOT
  waiving -- a coincidental match silently waives a real break. This
  includes the guide you are reading: its examples use real keys, so a PR
  that rewords this guide while ALSO breaking those same slots would
  accidentally waive them. Keep guide edits and contract breaks in
  separate PRs.

Key shapes by change type, as the checker prints them:

| Change | Key looks like |
|---|---|
| vtable slot moved/removed/retyped | `WebView[7]` |
| struct member changed | `WebContainerArguments.width` |
| enum value changed | `TTSMode::Forced` |
| extern "C" wrapper removed/renamed | `LWEDelegate_Settings_Create` |

## History
