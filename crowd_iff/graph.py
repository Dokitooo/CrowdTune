import struct,sys
import iff14   # IFF reader/writer for NBA 2K14 files (not included); put iff14.py next to these scripts
def load(path='crowd.iff'):   # the stock crowd.iff, copied next to this script
    I=iff14.read(path); ids={f['id']:f['meta']['name'] for f in I['files']}
    return I,ids
def parse(b,ids):
    n=struct.unpack_from('<I',b,4)[0]; rec=[]
    for i in range(n):
        o=24+24*i; a,_,m,_,X,Y=struct.unpack_from('<6I',b,o)
        rec.append(dict(i=i,o=o,name=ids.get(a,hex(a)),X=X,Y=Y,grp=X>>27,to=((X<<6)&0xffffffff)>>19 if not (X<<6)&0x80000000 else (((X<<6)&0xffffffff)-2**32)>>19, frm=X&0x1fff))
    to=8+struct.unpack_from('<I',b,8)[0]-1; nn=struct.unpack_from('<I',b,12)[0]; nodes=[]
    for k in range(nn):
        e=to+16*k; w=struct.unpack_from('<4I',b,e)
        lp=e+8+w[2]-1 if w[2] else None; cnt=w[3]&0x7fffffff
        lst=list(struct.unpack_from('<%dH'%cnt,b,lp)) if lp is not None and cnt else []
        nodes.append(dict(k=k,w=w,list=lst))
    return rec,nodes
if __name__=='__main__':
    I,ids=load(); 
    for nm in ('crowdren1',):
        f=[f for f in I['files'] if f['meta']['name']==nm][0]; b=bytes(I['secs'][0]['data'][f['offs'][0]:f['offs'][0]+f['meta']['size']])
        rec,nodes=parse(b,ids)
        print(len(rec),len(nodes)); print([ (r['name'],r['frm'],r['to']) for r in rec[:6]]); print(nodes[:3])
        print('max to',max(r['to'] for r in rec),'list idx max',max(max(n['list']) if n['list'] else 0 for n in nodes))
