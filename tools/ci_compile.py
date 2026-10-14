#!/usr/bin/env python3
"""Compile-only sweep: every .c in ancestor/Keil_5/ble_app_peripheral.uvprojx
built with arm-none-eabi-gcc -fsyntax-only. No link (linker port is next).

Usage: ci_compile.py --sdk <SDKROOT/DA145xx_SDK/6.0.22.1401 or its parent>
                     --ancestor <HMCLOCK-EN dir> [--cc arm-none-eabi-gcc]
Exit 0 iff every file compiles. Prints a per-file summary (first error only).
"""
import argparse, os, subprocess, sys, xml.etree.ElementTree as ET

def conv(p, anc, keil, sdk):
    p = p.strip().replace('\\', '/')
    if p.startswith('./../../../../../'): return sdk + '/' + p[len('./../../../../../'):]
    if p.startswith('../../../../../'): return sdk + '/' + p[len('../../../../../'):]
    if p.startswith('../'): return anc + '/' + p[len('../'):]
    if p.startswith('./'): return keil + '/' + p[len('./'):]
    return keil + '/' + p

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sdk', required=True)
    ap.add_argument('--ancestor', required=True)
    ap.add_argument('--cc', default='arm-none-eabi-gcc')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--extra-include', action='append', default=[])
    a = ap.parse_args()
    sdk = a.sdk
    if os.path.basename(sdk.rstrip('/')) != '6.0.22.1401':
        cand = os.path.join(sdk, 'DA145xx_SDK', '6.0.22.1401')
        sdk = cand if os.path.isdir(cand) else sdk
    anc, keil = a.ancestor, os.path.join(a.ancestor, 'Keil_5')
    t = ET.parse(os.path.join(keil, 'ble_app_peripheral.uvprojx'))
    incs = []
    for e in t.getroot().iter('IncludePath'):
        if e.text:
            for p in e.text.split(';'):
                q = conv(p, anc, keil, sdk)
                if q not in incs: incs.append(q)
    for p in a.extra_include:
        if p not in incs: incs.append(p)
    files = []
    for f in t.getroot().iter('File'):
        n, p = f.findtext('FileName'), f.findtext('FilePath')
        iib = f.findtext('FileOption/CommonProperty/IncludeInBuild')
        if n and n.endswith('.c') and p and iib != '0':
            files.append((n, conv(p, anc, keil, sdk)))
    print(f"sources={len(files)} includes={len(incs)} cc={a.cc}", flush=True)
    missing = [(n, q) for n, q in files if not os.path.isfile(q)]
    if missing:
        print(f"MISSING SOURCES: {len(missing)}")
        for n, q in missing[:10]: print(f"  {n} -> {q}")
        return 2
    # Keil MiscControls: force-include the config chain into every TU.
    force = ['-include', 'da14585_config_basic.h',
             '-include', 'da14585_config_advanced.h',
             '-include', 'user_config.h']
    I = [x for p in incs for x in ('-I', p)]
    if a.dry_run:
        print("CMD:", a.cc, '-mcpu=cortex-m0 -mthumb -std=gnu11 -fsyntax-only',
              *force, f"[{len(I)//2} -I dirs]", "<file.c>")
        return 0
    ok, fails = 0, []
    for n, q in files:
        r = subprocess.run([a.cc, '-mcpu=cortex-m0', '-mthumb', '-std=gnu11',
                            '-fsyntax-only', '-w'] + force + I + [q],
                           capture_output=True, text=True, timeout=180)
        if r.returncode == 0:
            ok += 1
        else:
            lines = [l for l in r.stderr.split('\n') if 'error' in l.lower()]
            fails.append((n, (lines[0] if lines else r.stderr.split(chr(10))[0])[:200]))
    print(f"COMPILE-ONLY: ok={ok} fail={len(fails)} / {len(files)}")
    for n, e in fails[:25]:
        print(f"FAIL {n}: {e}")
    return 0 if not fails else 1

if __name__ == '__main__':
    sys.exit(main())
