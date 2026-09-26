#     __                           __    __
#    / /   ____ ___  ______  _____/ /_  / /
#   / /   / __ `/ / / / __ \/ ___/ __ \/ / 
#  / /___/ /_/ / /_/ / / / /__/ / / /_/  
# /_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
# Launch! for DOS ---------------------
# MAINTAINER NOTES - Launch! 3.73
# File: MAKEGLYP.PY
# Role: Maintainer glyph-data generation helper
# Build/ownership: Host-side utility, not part of DOS runtime.
# Maintainer contract: Generated glyph output must remain compatible with GLYPHDAT.H consumers.
# Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
# DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
# Regenerate GLYPHDAT.H from the canonical Launch! glyph libraries.
from pathlib import Path
root=Path(__file__).resolve().parent
sources=[('launch_glyph16','res/LAUNCHUI.FNT',16),('launch_glyph14','res/LAUNCHUI.F14',14)]
count=103
with (root/'GLYPHDAT.H').open('w') as o:
    o.write(f'/* Generated from res/LAUNCHUI.FNT / res/LAUNCHUI.F14. Do not hand edit. */\n#define LAUNCH_GLYPH_COUNT {count}\n#ifdef LAUNCH_GLYPH_FAR\n#define GLYPH_STORE far\n#else\n#define GLYPH_STORE\n#endif\n')
    for symbol,filename,height in sources:
        data=(root/filename).read_bytes()
        if len(data)!=256*height: raise SystemExit(f'{filename}: wrong size')
        o.write(f'static const unsigned char GLYPH_STORE {symbol}[LAUNCH_GLYPH_COUNT][32]={{\n')
        for logical_id in range(1,count+1):
            glyph=list(data[logical_id*height:(logical_id+1)*height])+[0]*(32-height)
            comma=',' if logical_id<count else ''
            o.write('  {'+','.join(f'0x{x:02X}' for x in glyph)+'}'+comma+'\n')
        o.write('};\n')
