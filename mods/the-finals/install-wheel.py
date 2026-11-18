import struct,sys,hashlib
from pathlib import Path
p=Path(sys.argv[1]);backup=p.with_name('GE32.before-wheel.exe');original=backup.read_bytes() if backup.exists() else p.read_bytes();b=bytearray(original)
if hashlib.sha256(original).hexdigest() != "295e4c85fb82c1969ad2cfdaf1953bf41e291fbf81c770a946ba2f69e315f9ab":
 raise SystemExit("Run patch-character-back.py first on supported GE32 build 2026.11.07 e8859fe9.")
u16=lambda o:struct.unpack_from('<H',b,o)[0]
u32=lambda o:struct.unpack_from('<I',b,o)[0]
pe=u32(60);opt=pe+24;n=u16(pe+6);table=opt+u16(pe+20);sa=u32(opt+32);fa=u32(opt+36)
align=lambda x,a:(x+a-1)//a*a
sections=[struct.unpack_from('<IIII',b,table+i*40+8) for i in range(n)]
def raw(rva):
 for vs,rv,rs,rp in sections:
  if rv<=rva<rv+max(vs,rs):return rp+rva-rv
 raise ValueError(hex(rva))
# The practice roster includes the human player. Permit a roster of one,
# then check the roster size immediately after initializing the human slot.
# The original loop went straight to slot one, skipping that termination check.
minimum = raw(0xf5231)
after_human = raw(0xf549f)
count_check = raw(0xf54d1)
if (b[minimum:minimum + 5] != bytes.fromhex('b8 02 00 00 00') or
    b[after_human:after_human + 2] != bytes.fromhex('eb df') or
    b[count_check:count_check + 5] != bytes.fromhex('49 39 df 75 aa')):
 raise SystemExit("Unsupported GE32 practice roster loop.")
b[minimum + 1] = 1
b[after_human + 1] = count_check - (after_human + 2)

imp=raw(u32(opt+112+8));loadiat=None
while u32(imp+12):
 lookup=u32(imp);iat=u32(imp+16);j=0
 while lookup and struct.unpack_from('<Q',b,raw(lookup)+j*8)[0]:
  val=struct.unpack_from('<Q',b,raw(lookup)+j*8)[0]
  if val<2**63:
   nm=raw(val)+2;end=b.index(0,nm)
   if b[nm:end]==b'LoadLibraryW':loadiat=iat+j*8
  j+=1
 imp+=20
assert loadiat is not None
assert table+(n+1)*40<=u32(opt+60)
vs,rv,rs,rp=sections[-1];rva=align(rv+max(vs,rs),sa);rp=align(len(b),fa)
entry=0x227050;off=raw(entry);saved=bytes(b[off:off+10]);assert saved==bytes.fromhex('5756534881ec90000000')
code=bytearray.fromhex('5152415041514883ec28488d0d00000000ff15000000004883c428415941585a59')
code.extend(saved);jump=len(code);code.extend(b'\xe9'+struct.pack('<i',entry+10-(rva+jump+5)))
namepos=len(code);code.extend('weapon-wheel.dll\0'.encode('utf-16le'))
struct.pack_into('<i',code,13,rva+namepos-(rva+17))
struct.pack_into('<i',code,19,loadiat-(rva+23))
header=bytearray(40);header[:8]=b'.wheel\0\0';struct.pack_into('<IIII',header,8,len(code),rva,align(len(code),fa),rp);struct.pack_into('<I',header,36,0x60000020)
b[table+n*40:table+(n+1)*40]=header;struct.pack_into('<H',b,pe+6,n+1);struct.pack_into('<I',b,opt+56,align(rva+len(code),sa));struct.pack_into('<I',b,opt+64,0)
b.extend(b'\0'*(rp-len(b)));b.extend(code);b.extend(b'\0'*(align(len(code),fa)-len(code)))
b[off:off+10]=b'\xe9'+struct.pack('<i',rva-entry-5)+b'\x90'*5
if not backup.exists():backup.write_bytes(original)
p.write_bytes(b);print('Loader installed. Entry',hex(entry),'loader',hex(rva),'LoadLibrary IAT',hex(loadiat))
