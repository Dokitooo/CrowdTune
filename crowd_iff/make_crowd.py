"""crowd.iff: energetic applause entries (high claps, hands up, sit->stand while clapping) only when the
crowd is excited. Clears the 'always allowed' flag (record word +0x14 bit 28) of the men's applause
transitions in crowdren1; the game then plays them only when its excitement level is above 0.6
(CrowdTune raises that to ExcitedApplauseFrom, default 0.80). Everything else is byte-identical."""
import sys,struct,os
import iff14   # IFF reader/writer for NBA 2K14 files (not included); put iff14.py next to these scripts
from graph import parse
SRC=os.path.join(sys.argv[1], '') if len(sys.argv)>1 else ''   # folder with crowd.iff and crowd (modded).iff
def gated(rec):
    # men's clip names: st_ = seated, sd_ = standing (stst sit->sit, stsd sit->stand, sdst stand->sit).
    # Gate (big moments only): standing up while clapping (stsd_*) and seated high claps / hands up.
    dest=lambda n:n.split('_')[-1]
    return [r for r in rec if r['grp']==2 and (r['name'].startswith('stsd_') or
            (r['name'].startswith('stst_') and ('hiclp' in dest(r['name']) or 'hup' in dest(r['name']))))]
def patch_body(b,ids):
    rec,nodes=parse(b,ids); g=gated(rec); b=bytearray(b)
    for r in g:
        y=struct.unpack_from('<I',b,r['o']+20)[0]; assert y&0x10000000
        struct.pack_into('<I',b,r['o']+20,y&~0x10000000)
    return bytes(b),len(g)
I=iff14.read(SRC+'crowd.iff'); ids={f['id']:f['meta']['name'] for f in I['files']}
f=[f for f in I['files'] if f['meta']['name']=='crowdren1'][0]; o=f['offs'][0]; n=f['meta']['size']
S=I['secs'][0]['data']; nb,k=patch_body(bytes(S[o:o+n]),ids); S[o:o+n]=nb
os.makedirs('out',exist_ok=True); iff14.write(I,'out/crowd.iff'); print('default: gated',k)
# the modded file is stored uncompressed: patch the same bytes in place
d=bytearray(open(SRC+'crowd (modded).iff','rb').read())
so=0x14+struct.unpack_from('<I',d,0x14)[0]-1; s0=struct.unpack_from('<8I',d,so)
fo=s0[5]+o; assert d[fo:fo+n]==bytes(open(SRC+'crowd.iff','rb').read() and iff14.read(SRC+'crowd.iff')['secs'][0]['data'][o:o+n])
d[fo:fo+n]=nb; open('out/crowd (modded).iff','wb').write(d); print('modded: patched in place at',hex(fo))
