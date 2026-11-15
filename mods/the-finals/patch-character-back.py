import struct,hashlib,sys
from pathlib import Path
p=Path(sys.argv[1]); original=p.read_bytes(); backup=p.with_name('GE32.original.exe')
if backup.exists(): original=backup.read_bytes()
if hashlib.sha256(original).hexdigest() != 'b466f7d5c6bf8168fa908a6294a381f08d6ba6f9f565ccd0a3469b5c60f5d2d6':
 raise SystemExit('Unsupported GE32 binary: expected Windows build 2026.11.07 e8859fe9.')
b=bytearray(original); pe=struct.unpack_from('<I',b,60)[0]; n=struct.unpack_from('<H',b,pe+6)[0]; opt=pe+24; os=struct.unpack_from('<H',b,pe+20)[0]; table=opt+os
base=struct.unpack_from('<Q',b,opt+24)[0]; sa,fa=struct.unpack_from('<II',b,opt+32)
align=lambda x,a:(x+a-1)//a*a
last=table+(n-1)*40; vs,rv,rs,rp=struct.unpack_from('<IIII',b,last+8)
rva=align(rv+max(vs,rs),sa); raw=align(len(b),fa)
assert table+(n+1)*40<=struct.unpack_from('<I',b,opt+60)[0]
entry=0x5b5b0; entryraw=1024+370096
assert b[entryraw:entryraw+7]==bytes.fromhex('55415741564155')
code=bytearray(); labels={}; fix=[]
def emit(h): code.extend(bytes.fromhex(h))
def branch(op,target):
 emit(op); pos=len(code);code.extend(b'\0'*4);fix.append((pos,target))
def label(s): labels[s]=rva+len(code)
emit('4883ec28') # Windows x64 call alignment and shadow space
branch('e8',0x121ef0) # online character-select state
emit('85c0');branch('0f85','resume') # leave online implementation alone
emit('31c9ba00400000');branch('e8',0x19f790) # slot 0, native B
emit('6685c0');branch('0f84','resume')
emit('b90e00000031d2');branch('e8',0x63e40) # MENU_MP_OPTIONS
emit('4883c428c3')
label('resume');emit('4883c42855415741564155');branch('e9',entry+7)
for pos,target in fix:
 if isinstance(target,str):target=labels[target]
 struct.pack_into('<i',code,pos,target-(rva+pos+4))
# section is executable/readable code, all references relative for ASLR
header=bytearray(40);header[:8]=b'.bback\0\0';struct.pack_into('<IIII',header,8,len(code),rva,align(len(code),fa),raw);struct.pack_into('<I',header,36,0x60000020)
b[table+n*40:table+(n+1)*40]=header;struct.pack_into('<H',b,pe+6,n+1);struct.pack_into('<I',b,opt+56,align(rva+len(code),sa));struct.pack_into('<I',b,opt+64,0)
b.extend(b'\0'*(raw-len(b)));b.extend(code);b.extend(b'\0'*(align(len(code),fa)-len(code)))
b[entryraw:entryraw+7]=b'\xe9'+struct.pack('<i',rva-entry-5)+b'\x90\x90'
if not backup.exists():backup.write_bytes(original)
p.write_bytes(b)
print('Patched character-screen B -> multiplayer options; online path unchanged. Original backup:',backup)
print('Hook RVA',hex(rva),'bytes',len(code),'SHA256',hashlib.sha256(b).hexdigest())
