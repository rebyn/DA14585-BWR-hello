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
    files = []
    for f in t.getroot().iter('File'):
        n, p = f.findtext('FileName'), f.findtext('FilePath')
        if n and n.endswith('.c') and p:
            files.append((n, conv(p, anc, keil, sdk)))
    print(f"sources={len(files)} includes={len(incs)} cc={a.cc}", flush=True)
    missing = [(n, q) for n, q in files if not os.path.isfile(q)]
    if missing:
        print(f"MISSING SOURCES: {len(missing)}")
        for n, q in missing[:10]: print(f"  {n} -> {q}")
        return 2
    I = [x for p in incs for x in ('-I', p)]
    ok, fails = 0, []
    for n, q in files:
        r = subprocess.run([a.cc, '-mcpu=cortex-m0', '-mthumb', '-std=gnu11',
                            '-fsyntax-only', '-w'] + I + [q],
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
