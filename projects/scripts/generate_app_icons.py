#!/usr/bin/env python3
"""Regenerate the application icons in bin/assets/icon from their SVG sources.

For every application this derives the 256x256 .png runtime window icon and,
where platform packaging needs them, the Windows .ico, the MinGW .res/.x64.res
COFF resource objects, and the macOS .icns container.

The binary layouts intentionally mirror the historical artifacts:

  .ico     32bpp BMP entries (bottom-up BGRA XOR bitmap + AND mask with the bit
           set where alpha == 0) for sizes <= 128, a PNG entry for 256, with
           each application's historical entry sizes and order preserved.
  .icns    ic04/ic05 as 'ARGB' magic + per-plane RLE (literal byte n < 0x80
           introduces n+1 raw bytes, repeat byte n >= 0x80 introduces
           (n & 0x7f) + 3 copies of the next byte), PNG payloads for the
           larger types, and the 'info' metadata chunk carried over from the
           original Apple iconutil-generated files.
  .res     compiled from <app>.rc + <app>.ico with mingw windres, for both the
           pe-i386 and pe-x86-64 targets.

Requires: python3 (stdlib only), inkscape, and a mingw-w64 windres (found as
$WINDRES, x86_64-w64-mingw32-windres, or windres).
"""

import argparse
import base64
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

PNG_SIZE = 256

# Sizes requested from the .icns entry table below (plus the ICO sizes and
# PNG_SIZE), rendered natively from the SVG instead of resampled.
ICNS_ENTRIES = [
    ( 'ic12', 64, 'png' ),
    ( 'ic07', 128, 'png' ),
    ( 'ic13', 256, 'png' ),
    ( 'ic08', 256, 'png' ),
    ( 'ic04', 16, 'argb' ),
    ( 'ic14', 512, 'png' ),
    ( 'ic09', 512, 'png' ),
    ( 'ic05', 32, 'argb' ),
    ( 'ic10', 1024, 'png' ),
    ( 'ic11', 32, 'png' ),
    ( 'info', 0, 'info' ),
]

# The 'info' element is an NSKeyedArchiver property list written by Apple's
# iconutil. It carries no artwork, so the original bytes are kept verbatim to
# stay byte-compatible with iconutil output.
INFO_CHUNK = base64.b64decode(
    "YnBsaXN0MDDUAQIDBAUGBwpYJHZlcnNpb25ZJGFyY2hpdmVyVCR0b3BYJG9iamVjdHMSAAGGoF8Q"
    "D05TS2V5ZWRBcmNoaXZlctEICVRyb290gAGnCwwXGBkaHlUkbnVsbNMNDg8QExZXTlMua2V5c1pO"
    "Uy5vYmplY3RzViRjbGFzc6IREoACgAOiFBWABIAFgAZUbmFtZV8QFmFzc2V0Y2F0YWxvZy1yZWZl"
    "cmVuY2VUaWNvbtMNDg8bHBagoIAG0h8gISJaJGNsYXNzbmFtZVgkY2xhc3Nlc1xOU0RpY3Rpb25h"
    "cnmiISNYTlNPYmplY3QIERokKTI3SUxRU1thaHB7goWHiYyOkJKXsLW8vb7AxdDZ5ukAAAAAAAAB"
    "AQAAAAAAAAAkAAAAAAAAAAAAAAAAAAAA8g=="
)

# Per-application inputs and outputs. The ICO size lists keep the historical
# entry sets and order of each file; Windows selects the best size match
# regardless of order, so this is preserved only to keep artifact diffs minimal.
APPS = {
    'ee': { 'svg': 'ee-icon.svg', 'ico': [16, 24, 32, 48, 64, 128, 256], 'icns': True, 'res': True },
    'ecode': { 'svg': 'ecode-icon.svg', 'ico': [256, 128, 64, 48, 32, 16], 'icns': True, 'res': True },
    'eterm': { 'svg': 'eterm-icon.svg', 'ico': [16, 24, 32, 48, 64, 72, 96, 128, 256], 'icns': True, 'res': True },
    'eeiv': { 'svg': 'eeiv-icon.svg', 'ico': [16, 24, 32, 48, 64, 72, 96, 128, 256], 'icns': True, 'res': True },
    'eproc': { 'svg': 'eproc-icon.svg', 'ico': [16, 24, 32, 48, 64, 72, 96, 128, 256], 'icns': True, 'res': True },
}


def decode_png( blob ):
    """Decode an 8-bit RGBA non-interlaced PNG into row-major RGBA bytes."""

    if blob[:8] != b'\x89PNG\r\n\x1a\n':
        raise SystemExit( 'not a PNG blob' )
    width = height = 0
    idat = []
    pos = 8
    while pos + 8 <= len( blob ):
        length, ctype = struct.unpack( '>I4s', blob[pos:pos + 8] )
        body = blob[pos + 8:pos + 8 + length]
        if ctype == b'IHDR':
            width, height, depth, color, _, _, interlace = struct.unpack( '>IIBBBBB', body )
            if ( depth, color, interlace ) != ( 8, 6, 0 ):
                raise SystemExit(
                    f'unsupported PNG: depth={depth} color={color} interlace={interlace}' )
        elif ctype == b'IDAT':
            idat.append( body )
        pos += 12 + length
    raw = zlib.decompress( b''.join( idat ) )
    stride = width * 4
    out = bytearray( height * stride )
    previous = bytearray( stride )
    source = 0
    for y in range( height ):
        filter_type = raw[source]
        source += 1
        line = bytearray( raw[source:source + stride] )
        source += stride
        for x in range( stride ):
            left = line[x - 4] if x >= 4 else 0
            above = previous[x]
            corner = previous[x - 4] if x >= 4 else 0
            if filter_type == 1:
                line[x] = ( line[x] + left ) & 0xFF
            elif filter_type == 2:
                line[x] = ( line[x] + above ) & 0xFF
            elif filter_type == 3:
                line[x] = ( line[x] + ( left + above ) // 2 ) & 0xFF
            elif filter_type == 4:
                estimate = left + above - corner
                distances = ( abs( estimate - left ), abs( estimate - above ), abs( estimate - corner ) )
                predictor = ( left, above, corner )[distances.index( min( distances ) )]
                line[x] = ( line[x] + predictor ) & 0xFF
        out[y * stride:( y + 1 ) * stride] = line
        previous = line
    return width, height, bytes( out )


def bmp_entry( png ):
    """32bpp BGRA bottom-up XOR bitmap plus AND mask, as in the historical .ico files."""

    width, height, pixels = decode_png( png )
    xor = bytearray()
    mask_rows = ( ( width + 31 ) // 32 ) * 4
    mask = bytearray()
    for y in range( height - 1, -1, -1 ):
        for x in range( width ):
            offset = ( y * width + x ) * 4
            r, g, b, a = pixels[offset:offset + 4]
            xor += bytes( ( b, g, r, a ) )
        row = bytearray( mask_rows )
        for x in range( width ):
            if pixels[( y * width + x ) * 4 + 3] == 0:
                row[x // 8] |= 0x80 >> ( x % 8 )
        mask += row
    header = struct.pack( '<IiiHHIIiiII', 40, width, 2 * height, 1, 32, 0, width * height * 4,
                          3779, 3779, 0, 0 )
    return header + bytes( xor ) + bytes( mask )


def build_ico( pngs, sizes ):
    entries = []
    for size in sizes:
        if size == PNG_SIZE:
            entries.append( ( size, pngs[size] ) )
        else:
            entries.append( ( size, bmp_entry( pngs[size] ) ) )
    out = struct.pack( '<HHH', 0, 1, len( entries ) )
    offset = 6 + 16 * len( entries )
    for size, blob in entries:
        out += struct.pack( '<BBBBHHII', size % 256, size % 256, 0, 0, 1, 32, len( blob ), offset )
        offset += len( blob )
    for _, blob in entries:
        out += blob
    return out


def rle_encode( plane ):
    """icns ARGB RLE: n < 0x80 introduces n+1 raw bytes, n >= 0x80 introduces ( n & 0x7f ) + 3 copies."""

    out = bytearray()
    i = 0
    size = len( plane )
    while i < size:
        run = 1
        while run < 130 and i + run < size and plane[i + run] == plane[i]:
            run += 1
        if run >= 3:
            out += bytes( ( 0x80 | ( run - 3 ), plane[i] ) )
            i += run
            continue
        start = i
        i += run
        while i - start < 128:
            if i >= size:
                break
            run = 1
            while run < 3 and i + run < size and plane[i + run] == plane[i]:
                run += 1
            if run >= 3:
                break
            i += run
        out += bytes( ( i - start - 1, ) ) + bytes( plane[start:i] )
    return bytes( out )


def rle_decode( stream ):
    out = bytearray()
    i = 0
    while i < len( stream ):
        token = stream[i]
        i += 1
        if token & 0x80:
            out += bytes( [stream[i]] ) * ( ( token & 0x7f ) + 3 )
            i += 1
        else:
            count = token + 1
            out += stream[i:i + count]
            i += count
    return bytes( out )


def argb_entry( png ):
    """ic04/ic05 payload: 'ARGB' magic plus RLE-compressed A, R, G, B planes."""

    width, height, pixels = decode_png( png )
    planes = [bytearray( width * height ) for _ in range( 4 )]
    for p in range( width * height ):
        r, g, b, a = pixels[p * 4:p * 4 + 4]
        planes[0][p] = a
        planes[1][p] = r
        planes[2][p] = g
        planes[3][p] = b
    encoded = [rle_encode( plane ) for plane in planes]
    blob = b'ARGB' + b''.join( encoded )
    if rle_decode( blob[4:] ) != b''.join( planes ):
        raise SystemExit( 'ARGB RLE round-trip failed' )
    return blob


def build_icns( pngs ):
    chunks = []
    for entry_type, size, encoding in ICNS_ENTRIES:
        if encoding == 'png':
            payload = pngs[size]
        elif encoding == 'argb':
            payload = argb_entry( pngs[size] )
        else:
            payload = INFO_CHUNK
        chunks.append( entry_type.encode() + struct.pack( '>I', 8 + len( payload ) ) + payload )
    body = b''.join( chunks )
    return b'icns' + struct.pack( '>I', 8 + len( body ) ) + body


def render_png( inkscape, svg, size, out_path ):
    subprocess.run(
        [inkscape, '--export-type=png', '--export-area-page',
         f'--export-width={size}', f'--export-height={size}',
         '--export-background-opacity=0', f'--export-filename={out_path}', svg],
        check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL )
    with open( out_path, 'rb' ) as handle:
        return handle.read()


def find_windres():
    candidates = [os.environ.get( 'WINDRES' ), 'x86_64-w64-mingw32-windres', 'windres']
    for candidate in candidates:
        if candidate and shutil.which( candidate ):
            return candidate
    raise SystemExit( 'windres not found (install mingw-w64 binutils or set $WINDRES)' )


def generate( app, icon_dir, tmp_dir, inkscape, windres ):
    spec = APPS[app]
    icns_sizes = {size for _, size, _ in ICNS_ENTRIES}
    needed = set( spec['ico'] ) | {PNG_SIZE}
    if spec['icns']:
        needed |= icns_sizes
    pngs = {}
    for size in sorted( needed ):
        pngs[size] = render_png( inkscape, os.path.join( icon_dir, spec['svg'] ), size,
                                 os.path.join( tmp_dir, f'{app}-{size}.png' ) )

    written = [f'{app}.png']
    with open( os.path.join( icon_dir, f'{app}.png' ), 'wb' ) as handle:
        handle.write( pngs[PNG_SIZE] )
    if spec['ico']:
        with open( os.path.join( icon_dir, f'{app}.ico' ), 'wb' ) as handle:
            handle.write( build_ico( pngs, spec['ico'] ) )
        written.append( f'{app}.ico' )
    if spec['icns']:
        with open( os.path.join( icon_dir, f'{app}.icns' ), 'wb' ) as handle:
            handle.write( build_icns( pngs ) )
        written.append( f'{app}.icns' )
    if spec['res']:
        for target, output in ( ( 'pe-i386', f'{app}.res' ), ( 'pe-x86-64', f'{app}.x64.res' ) ):
            subprocess.run( [windres, f'--target={target}', '-O', 'coff', f'{app}.rc', output],
                            check=True, cwd=icon_dir )
        written.append( f'{app}.res, {app}.x64.res' )
    print( f'{app}: {", ".join( written )}' )


def main():
    parser = argparse.ArgumentParser( description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter )
    parser.add_argument( '--app', action='append', choices=sorted( APPS ),
                         help='application to regenerate (repeatable; default: all)' )
    args = parser.parse_args()

    script_dir = os.path.dirname( os.path.abspath( __file__ ) )
    icon_dir = os.path.join( os.path.abspath( os.path.join( script_dir, '..', '..' ) ),
                             'bin', 'assets', 'icon' )
    inkscape = shutil.which( 'inkscape' )
    if not inkscape:
        raise SystemExit( 'inkscape not found' )
    apps = args.app or list( APPS )
    windres = find_windres() if any( APPS[app]['res'] for app in apps ) else None

    with tempfile.TemporaryDirectory() as tmp_dir:
        for app in apps:
            generate( app, icon_dir, tmp_dir, inkscape, windres )
    return 0


if __name__ == '__main__':
    sys.exit( main() )
