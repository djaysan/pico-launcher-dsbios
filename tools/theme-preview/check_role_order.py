#!/usr/bin/env python3
"""The colour-override role order is written out in three places; they must agree.

A mismatch does not fail to build and does not crash - it silently assigns each
colour to the WRONG role, which is the kind of bug that only shows up as "the
theme looks wrong on the card". So it gets a check.

  python3 tools/theme-preview/check_role_order.py
"""
import os, re, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'arm9', 'source', 'themes')


def read(p):
    return open(os.path.join(ROOT, p)).read()


# 1. the enum in ThemeColorOverrides.h
enum_body = re.search(r'enum Role\s*\{(.*?)RoleCount', read('ThemeColorOverrides.h'), re.S).group(1)
enum = [t.strip() for t in enum_body.replace('\n', ' ').split(',') if t.strip()]

# 2. the JSON key table in ThemeInfoFactory.thumb.cpp
names_body = re.search(r'sColorRoleNames\[ThemeColorOverrides::RoleCount\]\s*=\s*\{(.*?)\};',
                       read('ThemeInfoFactory.thumb.cpp'), re.S).group(1)
names = re.findall(r'"([^"]+)"', names_body)

# 3. the struct-field table in Theme.cpp
fields_body = re.search(r'fields\[O::RoleCount\]\s*=\s*\{(.*?)\};', read('Theme.cpp'), re.S).group(1)
fields = re.findall(r'_materialColorScheme\.(\w+)', fields_body)

lower = lambda s: s[0].lower() + s[1:]
ok = True
if not (len(enum) == len(names) == len(fields)):
    print(f'FAIL lengths differ: enum {len(enum)}, names {len(names)}, fields {len(fields)}')
    ok = False
else:
    for i, (e, n, f) in enumerate(zip(enum, names, fields)):
        if not (lower(e) == n == f):
            print(f'FAIL slot {i}: enum {e!r} -> json key {n!r} -> field {f!r}')
            ok = False

print(f'{"ok" if ok else "FAILED"}: {len(enum)} roles line up across the enum, '
      f'the theme.json key table and the scheme fields')
sys.exit(0 if ok else 1)
