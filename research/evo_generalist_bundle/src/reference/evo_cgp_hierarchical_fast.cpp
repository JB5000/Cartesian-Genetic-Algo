#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>

static constexpr int NODES=64, TERMS=4, MAX_MODULES=12, MAX_MOD_NODES=64, MAX_WORDS=32;
static constexpr int PRIMS=7, CALL_BASE=PRIMS, CALL_MUT_PCT=1;
#ifndef STALL_GATE
#define STALL_GATE 32
#endif
#pragma pack(push,1)
struct Node{uint8_t op,a,b,c;}; struct Genome{Node n[NODES];uint8_t out;};
#pragma pack(pop)
static_assert(sizeof(Genome)==257);
enum:uint8_t{PASS=0,XOR_=1,AND_=2,OR_=3,NOT_=4,SHL1=5,SHR1=6};
struct Rng{uint64_t s;explicit Rng(uint64_t x):s(x?x:1){}inline uint64_t next(){uint64_t x=s;x^=x>>12;x^=x<<25;x^=x>>27;s=x;return x*0x2545F4914F6CDD1DULL;}inline uint32_t u(){return next()>>32;}inline uint32_t n(uint32_t m){return m?(uint32_t)((uint64_t)u()*m>>32):0;}};
struct Active{std::array<uint8_t,NODES>id{};uint8_t n=0;};
struct Module{uint8_t n=0,out=0,in_bits=0,out_bits=0;std::array<Node,MAX_MOD_NODES> node{};};
struct Library{uint8_t n=0;std::array<Module,MAX_MODULES>m{};};
static inline int arity(uint8_t op){return op>=CALL_BASE?2:((op==PASS||op==NOT_||op==SHL1||op==SHR1)?1:2);}
static Active active(const Genome&g){Active a;uint64_t mask=0;uint8_t st[NODES];int sp=0;if(g.out>=TERMS){int r=g.out-TERMS;if(r<NODES){mask|=1ULL<<r;st[sp++]=r;}}while(sp){int i=st[--sp];auto&n=g.n[i];uint8_t ss[2]={n.a,n.b};for(int k=0;k<arity(n.op);k++)if(ss[k]>=TERMS){int j=ss[k]-TERMS;if(j<i&&!(mask&(1ULL<<j))){mask|=1ULL<<j;st[sp++]=j;}}}while(mask){unsigned i=std::countr_zero(mask);a.id[a.n++]=i;mask&=mask-1;}return a;}
static inline uint64_t rep(uint64_t x,int l){return l==4?x*0x1111111111111111ULL:x*0x0101010101010101ULL;}
struct Data{int bits,outbits,cases,lane,lanes,words;uint64_t lsb,top,outmask,last;std::array<uint64_t,MAX_WORDS>A{},B{},Y{};};
static Data data(int bits){Data d{};d.bits=bits;d.outbits=bits+1;d.cases=1<<(2*bits);d.lane=d.outbits<=4?4:8;d.lanes=64/d.lane;d.words=(d.cases+d.lanes-1)/d.lanes;if(d.words>MAX_WORDS)abort();d.lsb=rep(1,d.lane);d.top=rep(1ULL<<(d.lane-1),d.lane);d.outmask=rep((1ULL<<d.outbits)-1,d.lane);int side=1<<bits,idx=0;for(int w=0;w<d.words;w++){int n=std::min(d.lanes,d.cases-idx);for(int j=0;j<n;j++){int z=idx+j,aa=z/side,bb=z%side,sh=j*d.lane;d.A[w]|=(uint64_t)aa<<sh;d.B[w]|=(uint64_t)bb<<sh;d.Y[w]|=(uint64_t)(aa+bb)<<sh;}idx+=n;}int rem=d.cases%d.lanes;d.last=rem?0:d.lsb;if(rem)for(int j=0;j<rem;j++)d.last|=1ULL<<(j*d.lane);return d;}
static inline uint64_t prim(uint8_t op,uint64_t a,uint64_t b,const Data&d){switch(op){case PASS:return a;case XOR_:return a^b;case AND_:return a&b;case OR_:return a|b;case NOT_:return ~a;case SHL1:return(a<<1)&~d.lsb;case SHR1:return(a>>1)&~d.top;default:return a;}}
static uint64_t call(const Library&,uint8_t,uint64_t,uint64_t,const Data&);
static uint64_t call(const Library&lib,uint8_t mid,uint64_t A,uint64_t B,const Data&d){const Module&m=lib.m[mid];uint64_t v[TERMS+MAX_MOD_NODES];uint64_t im=rep((1ULL<<m.in_bits)-1,d.lane),om=rep((1ULL<<m.out_bits)-1,d.lane);v[0]=A&im;v[1]=B&im;v[2]=0;v[3]=rep(1,d.lane);for(int i=0;i<m.n;i++){auto&n=m.node[i];uint64_t a=v[n.a],b=v[n.b];v[TERMS+i]=n.op<CALL_BASE?prim(n.op,a,b,d):call(lib,n.op-CALL_BASE,a,b,d);}return v[m.out]&om;}
struct Score{uint32_t err=0,exact=0;};
static inline Score score(const Genome&g,const Library&lib,const Data&d){Score s{};Active ac=active(g);uint64_t one=rep(1,d.lane);for(int w=0;w<d.words;w++){uint64_t v[TERMS+NODES];v[0]=d.A[w];v[1]=d.B[w];v[2]=0;v[3]=one;for(int k=0;k<ac.n;k++){int i=ac.id[k];auto&n=g.n[i];uint64_t a=v[n.a],b=v[n.b];v[TERMS+i]=n.op<CALL_BASE?prim(n.op,a,b,d):call(lib,n.op-CALL_BASE,a,b,d);}uint64_t o=v[g.out],df=(o^d.Y[w])&d.outmask;s.err+=std::popcount(df);uint64_t nz=df;nz|=nz>>1;nz|=nz>>2;if(d.lane==8)nz|=nz>>4;uint64_t wrong=nz&d.lsb;if(w==d.words-1)wrong&=d.last;int valid=(w==d.words-1&&d.cases%d.lanes)?d.cases%d.lanes:d.lanes;s.exact+=valid-std::popcount(wrong);}return s;}
static Genome randomg(Rng&r){Genome g{};for(int i=0;i<NODES;i++){g.n[i].op=r.n(PRIMS);uint32_t l=TERMS+i;g.n[i].a=r.n(l);g.n[i].b=r.n(l);g.n[i].c=0;}g.out=r.n(TERMS+NODES);return g;}
static inline void field(Genome&g,Rng&r,uint32_t q){if(q==NODES*3){g.out=r.n(TERMS+NODES);return;}int i=q/3,f=q%3;uint32_t l=TERMS+i;if(f==0){uint8_t x,o=g.n[i].op;do{x=r.n(PRIMS);}while(x==o);g.n[i].op=x;}else if(f==1){uint8_t x,o=g.n[i].a;do{x=r.n(l);}while(l>1&&x==o);g.n[i].a=x;}else{uint8_t x,o=g.n[i].b;do{x=r.n(l);}while(l>1&&x==o);g.n[i].b=x;}}
static inline bool isactive(const Active&a,int ni){for(int k=0;k<a.n;k++)if(a.id[k]==ni)return true;return false;}
struct Mut{bool changed;};
static inline Mut mutate(Genome&g,const Active&a,Rng&r,const Library&lib,bool allow_call){if(allow_call&&lib.n&&a.n&&r.n(100)<CALL_MUT_PCT){int ni=a.id[r.n(a.n)],l=TERMS+ni;g.n[ni].op=CALL_BASE+r.n(lib.n);g.n[ni].a=r.n(l);g.n[ni].b=r.n(l);return{true};}int edits=1+(r.n(100)<35)+(r.n(100)<8);bool ch=false;for(int e=0;e<edits;e++){uint32_t q;if(e==0&&r.n(100)<88){if(a.n&&r.n(100)<90){int ni=a.id[r.n(a.n)];q=ni*3+r.n(3);}else q=NODES*3;}else q=r.n(NODES*3+1);if(q==NODES*3||isactive(a,q/3))ch=true;field(g,r,q);}return{ch};}
static inline bool better(const Score&a,const Score&b){return a.err!=b.err?a.err<b.err:a.exact>b.exact;}static inline bool eq(const Score&a,const Score&b){return a.err==b.err&&a.exact==b.exact;}
struct Run{Genome g{};Score s{};uint64_t mut=0,ev=0;double sec=0;bool ok=false;};
static Run evolve(int bits,uint64_t seed,uint64_t budget,const Library&lib,int lambda){
    Data d=data(bits);Rng r(seed);Genome p=randomg(r);Score ps=score(p,lib,d);uint64_t mu=0,ev=1;int stall=0;
    auto t=std::chrono::steady_clock::now();
    while(mu<budget&&ps.err){
        Active ac=active(p);Genome bg=p;Score bs=ps;bool take=false;bool strict=false;
        const bool allow_call = stall >= STALL_GATE;
        for(int j=0;j<lambda&&mu<budget;j++){
            Genome c=p;Mut m=mutate(c,ac,r,lib,allow_call);mu++;Score cs=m.changed?score(c,lib,d):ps;if(m.changed)ev++;
            if(better(cs,bs)||(eq(cs,bs)&&(r.u()&1))){if(better(cs,ps))strict=true;bg=c;bs=cs;take=true;}
        }
        if(take){p=bg;ps=bs;} if(strict)stall=0; else stall++;
    }
    double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();return{p,ps,mu,ev,sec,ps.err==0};
}
static bool promote(const Genome&g,int ib,int ob,Library&lib){if(lib.n>=MAX_MODULES)return false;Active ac=active(g);Module m{};m.n=ac.n;m.in_bits=ib;m.out_bits=ob;int8_t mp[NODES];std::fill(std::begin(mp),std::end(mp),(int8_t)-1);for(int k=0;k<ac.n;k++)mp[ac.id[k]]=k;auto M=[&](uint8_t s)->uint8_t{return s<TERMS?s:TERMS+mp[s-TERMS];};for(int k=0;k<ac.n;k++){Node x=g.n[ac.id[k]];x.a=M(x.a);x.b=M(x.b);m.node[k]=x;}m.out=M(g.out);lib.m[lib.n++]=m;return true;}
static uint64_t med(std::vector<uint64_t>v){std::sort(v.begin(),v.end());return v.empty()?0:v[v.size()/2];}
int main(int argc,char**argv){
    int trials=argc>1?atoi(argv[1]):128;
    uint64_t bud=argc>2?strtoull(argv[2],0,10):500000;
    std::string pref=argc>3?argv[3]:"/mnt/data/evo_hier_fast";
    int lambda=argc>4?atoi(argv[4]):8;
    int threads=argc>5?atoi(argv[5]):1;
    if(threads<1)threads=1;

    Library empty{}; Run best{}; bool have=false;
    for(int i=0;i<32;i++){
        Run r=evolve(1,10000+i,100000,empty,lambda);
        if(r.ok&&(!have||active(r.g).n<active(best.g).n)){best=r;have=true;}
    }
    if(!have){std::fprintf(stderr,"failed to learn base module\n");return 2;}
    Library lib{}; promote(best.g,1,2,lib);

    struct Pair{Run flat,hier;};
    std::vector<Pair> result(trials);
    std::atomic<int> next{0};
    auto wall0=std::chrono::steady_clock::now();
    auto worker=[&](){
        for(;;){
            int i=next.fetch_add(1,std::memory_order_relaxed); if(i>=trials)break;
            uint64_t seed=40000+i;
            result[i].flat=evolve(2,seed,bud,empty,lambda);
            result[i].hier=evolve(2,seed,bud,lib,lambda);
        }
    };
    std::vector<std::thread> pool; pool.reserve(threads);
    for(int t=0;t<threads;t++)pool.emplace_back(worker);
    for(auto&th:pool)th.join();
    double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-wall0).count();

    std::ofstream f(pref+"_summary.csv");
    f<<"trial,mode,solved,mutations,evals,seconds,mutations_per_s,evals_per_s\n";
    std::vector<uint64_t>vf,vh;int sf=0,sh=0;double tf=0,th=0;uint64_t ef=0,eh=0;
    for(int i=0;i<trials;i++){
        const Run&a=result[i].flat; const Run&h=result[i].hier;
        f<<i<<",flat,"<<a.ok<<","<<a.mut<<","<<a.ev<<","<<a.sec<<","<<a.mut/a.sec<<","<<a.ev/a.sec<<"\n";
        f<<i<<",hier,"<<h.ok<<","<<h.mut<<","<<h.ev<<","<<h.sec<<","<<h.mut/h.sec<<","<<h.ev/h.sec<<"\n";
        if(a.ok){sf++;vf.push_back(a.mut);}if(h.ok){sh++;vh.push_back(h.mut);}
        tf+=a.sec;th+=h.sec;ef+=a.ev;eh+=h.ev;
    }
    std::printf("Genome=%zu B Node=%zu B | M0 active=%u | threads=%d\n",sizeof(Genome),sizeof(Node),active(best.g).n,threads);
    std::printf("SUMMARY flat %d/%d med=%llu aggregate=%.4fs eval/s=%.2fM | hier %d/%d med=%llu aggregate=%.4fs eval/s=%.2fM | wall=%.4fs\n",
        sf,trials,(unsigned long long)med(vf),tf,ef/tf/1e6,
        sh,trials,(unsigned long long)med(vh),th,eh/th/1e6,wall);
    return 0;
}
