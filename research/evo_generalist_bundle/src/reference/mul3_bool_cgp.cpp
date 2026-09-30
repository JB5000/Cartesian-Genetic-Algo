#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <cstring>

static constexpr int INPUTS=6;
static constexpr int CONSTS=2;
static constexpr int TERMS=INPUTS+CONSTS; // A0 A1 A2 B0 B1 B2 0 1
static constexpr int NODES=64;
static constexpr int OUTS=6;
static constexpr int PRIMS=4;
static constexpr uint64_t ALL=~0ULL;

enum : uint8_t { XOR_=0, AND_=1, OR_=2, NOT_=3 };
#pragma pack(push,1)
struct Node { uint8_t op,a,b,c; };
struct Genome { Node n[NODES]; uint8_t out[OUTS]; };
#pragma pack(pop)

struct RNG { uint64_t s; explicit RNG(uint64_t x):s(x?x:1){}; inline uint64_t next(){uint64_t x=s;x^=x>>12;x^=x<<25;x^=x>>27;s=x;return x*0x2545F4914F6CDD1DULL;} inline uint32_t u(){return (uint32_t)(next()>>32);} inline uint32_t n(uint32_t m){return m?(uint32_t)(((uint64_t)u()*m)>>32):0;} };

struct Score { uint16_t biterr; uint8_t exact; };
static inline bool better(const Score&a,const Score&b){return a.biterr!=b.biterr ? a.biterr<b.biterr : a.exact>b.exact;}
static inline bool equalS(const Score&a,const Score&b){return a.biterr==b.biterr && a.exact==b.exact;}

static std::array<uint64_t,TERMS> term;
static std::array<uint64_t,OUTS> target;
static void init_truth(){
    for(auto &x:term)x=0; for(auto &x:target)x=0;
    for(int idx=0;idx<64;idx++){
        int A=idx>>3, B=idx&7; uint64_t m=1ULL<<idx;
        for(int k=0;k<3;k++){ if((A>>k)&1) term[k]|=m; if((B>>k)&1) term[3+k]|=m; }
        int y=A*B; for(int k=0;k<6;k++) if((y>>k)&1) target[k]|=m;
    }
    term[6]=0; term[7]=ALL;
}

struct Active { uint64_t mask=0; uint8_t ids[NODES]; uint8_t n=0; };
static inline int arity(uint8_t op){return op==NOT_?1:2;}
static Active active(const Genome&g){
    Active a; uint8_t st[NODES*2]; int sp=0;
    for(int o=0;o<OUTS;o++) if(g.out[o]>=TERMS){int r=g.out[o]-TERMS; if(r<NODES && !(a.mask&(1ULL<<r))){a.mask|=1ULL<<r; st[sp++]=r;}}
    while(sp){int i=st[--sp]; const Node&n=g.n[i]; uint8_t src[2]={n.a,n.b}; for(int k=0;k<arity(n.op);k++) if(src[k]>=TERMS){int j=src[k]-TERMS; if(j<i && !(a.mask&(1ULL<<j))){a.mask|=1ULL<<j;st[sp++]=j;}}}
    uint64_t m=a.mask; while(m){unsigned i=std::countr_zero(m);a.ids[a.n++]=(uint8_t)i;m&=m-1;} return a;
}

static inline uint64_t gate(uint8_t op,uint64_t a,uint64_t b){switch(op){case XOR_:return a^b;case AND_:return a&b;case OR_:return a|b;case NOT_:return ~a;default:return a;}}
static inline Score eval(const Genome&g){
    Active ac=active(g); uint64_t v[TERMS+NODES]; for(int i=0;i<TERMS;i++)v[i]=term[i];
    for(int k=0;k<ac.n;k++){int i=ac.ids[k];const Node&n=g.n[i];v[TERMS+i]=gate(n.op,v[n.a],v[n.b]);}
    uint16_t err=0; uint64_t wrong=0;
    for(int o=0;o<OUTS;o++){uint64_t d=v[g.out[o]]^target[o];err += std::popcount(d);wrong|=d;}
    return {err,(uint8_t)(64-std::popcount(wrong))};
}

static Genome randomg(RNG&r){Genome g{};for(int i=0;i<NODES;i++){g.n[i].op=r.n(PRIMS);uint32_t lim=TERMS+i;g.n[i].a=r.n(lim);g.n[i].b=r.n(lim);g.n[i].c=0;}for(int o=0;o<OUTS;o++)g.out[o]=r.n(TERMS+NODES);return g;}

struct Undo { uint16_t key; uint8_t old; };
// key 0..NODES*3-1 => op,a,b ; then outputs
static inline uint8_t* gene_ptr(Genome&g,uint16_t key){
    if(key<NODES*3){int i=key/3,f=key%3;return f==0?&g.n[i].op:(f==1?&g.n[i].a:&g.n[i].b);} return &g.out[key-NODES*3];
}
static inline uint8_t rand_new(Genome&g,uint16_t key,RNG&r){
    if(key<NODES*3){int i=key/3,f=key%3;if(f==0)return r.n(PRIMS);return r.n(TERMS+i);}return r.n(TERMS+NODES);
}
static inline void mutate_key(Genome&g,uint16_t key,RNG&r,Undo&u){
    uint8_t*p=gene_ptr(g,key);u={key,*p};uint8_t x=*p;for(int z=0;z<8&&x==*p;z++)x=rand_new(g,key,r);if(x==*p){uint32_t lim=(key<NODES*3?(key%3==0?PRIMS:TERMS+key/3):TERMS+NODES);x=(uint8_t)((*p+1)%lim);}*p=x;
}

static inline uint16_t active_gene(const Active&a,RNG&r){
    if(a.n==0) return NODES*3+r.n(OUTS);
    int ni=a.ids[r.n(a.n)]; uint32_t z=r.n(100);
    int f = z<33 ? 0 : (z<67 ? 1 : 2);
    return (uint16_t)(ni*3+f);
}

static inline int pick_k(uint64_t stall,RNG&r){
    uint32_t x=r.n(1000);
    if(stall<2000){ if(x<900)return 1;if(x<990)return 2;return 4; }
    if(stall<10000){ if(x<750)return 1;if(x<900)return 2;if(x<980)return 4;return 8; }
    if(stall<50000){ if(x<600)return 1;if(x<800)return 2;if(x<950)return 4;if(x<990)return 8;return 16; }
    if(x<450)return 1;if(x<700)return 2;if(x<880)return 4;if(x<970)return 8;return 16;
}

static inline void neutral_drift(Genome&g,const Active&a,RNG&r){
    if(a.n>=NODES)return; // choose an inactive node, mutate one field, no eval needed
    for(int tries=0;tries<8;tries++){
        int ni=r.n(NODES);if(!(a.mask&(1ULL<<ni))){uint16_t key=(uint16_t)(ni*3+r.n(3));Undo u;mutate_key(g,key,r,u);return;}
    }
}

struct Result{bool solved;uint64_t evals;uint16_t err;uint8_t exact,active_n;double sec;Genome g;};
static Result run(uint64_t seed,uint64_t budget){
    RNG r(seed);Genome g=randomg(r);Score s=eval(g);uint64_t ev=1,stall=0;auto t0=std::chrono::steady_clock::now();
    while(ev<budget && s.biterr){
        Active ac=active(g);
        // Neutral reservoir mutation survives regardless of candidate acceptance.
        if((r.u()&3)==0) neutral_drift(g,ac,r);
        ac=active(g);
        int K=pick_k(stall,r); Undo undo[16]; int un=0;
        for(int e=0;e<K;e++){
            uint16_t key;
            if(r.n(100)<10) key=NODES*3+r.n(OUTS); // root/output mutation
            else key=active_gene(ac,r);
            // avoid duplicate undo key so rollback is correct/simple
            bool dup=false;for(int q=0;q<un;q++)if(undo[q].key==key){dup=true;break;}if(dup){e--;continue;}
            mutate_key(g,key,r,undo[un++]);
        }
        Score ns=eval(g);ev++;
        if(better(ns,s)){s=ns;stall=0;}
        else if(equalS(ns,s)){
            // neutral walk accepted; still counts as stalled search
            s=ns;stall++;
        } else {
            for(int q=un-1;q>=0;q--)*gene_ptr(g,undo[q].key)=undo[q].old;
            stall++;
        }
    }
    double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();auto ac=active(g);return{s.biterr==0,ev,s.biterr,s.exact,ac.n,sec,g};
}

static void dump_genome(const Genome&g,const char*path){std::ofstream f(path);auto ac=active(g);f<<"active_nodes="<<(int)ac.n<<"\noutputs:";for(int o=0;o<OUTS;o++)f<<" "<<(int)g.out[o];f<<"\n";for(int k=0;k<ac.n;k++){int i=ac.ids[k];auto n=g.n[i];f<<"N"<<i<<" op="<<(int)n.op<<" a="<<(int)n.a<<" b="<<(int)n.b<<"\n";}}

int main(int argc,char**argv){
    init_truth();int trials=argc>1?atoi(argv[1]):32;uint64_t budget=argc>2?strtoull(argv[2],0,10):5000000ULL;int threads=argc>3?atoi(argv[3]):5;const char*csv=argc>4?argv[4]:"/mnt/data/mul3_bool_results.csv";
    std::vector<Result> res(trials);std::atomic<int> next{0};auto wall0=std::chrono::steady_clock::now();
    auto worker=[&](){for(;;){int i=next.fetch_add(1);if(i>=trials)break;res[i]=run(900000+i,budget);}};
    std::vector<std::thread> pool;for(int t=0;t<threads;t++)pool.emplace_back(worker);for(auto&t:pool)t.join();
    double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-wall0).count();
    std::ofstream f(csv);f<<"seed,solved,evals,error_bits,exact_cases,active_nodes,seconds\n";
    int solved=0;uint16_t besterr=999;int bi=-1;uint64_t bestev=~0ULL;
    for(int i=0;i<trials;i++){auto&r=res[i];f<<900000+i<<","<<r.solved<<","<<r.evals<<","<<r.err<<","<<(int)r.exact<<","<<(int)r.active_n<<","<<r.sec<<"\n";if(r.solved)solved++;if(r.err<besterr||(r.err==besterr&&r.evals<bestev)){besterr=r.err;bestev=r.evals;bi=i;}}
    if(bi>=0)dump_genome(res[bi].g,"/mnt/data/mul3_bool_best_genome.txt");
    std::printf("Genome=%zuB trials=%d budget=%llu threads=%d solved=%d best_err=%u best_exact=%u/64 best_active=%u best_evals=%llu wall=%.3fs\n",sizeof(Genome),trials,(unsigned long long)budget,threads,solved,besterr,(unsigned)res[bi].exact,(unsigned)res[bi].active_n,(unsigned long long)res[bi].evals,wall);
    if(solved){uint64_t minE=~0ULL;int mi=-1;for(int i=0;i<trials;i++)if(res[i].solved&&res[i].evals<minE){minE=res[i].evals;mi=i;}dump_genome(res[mi].g,"/mnt/data/mul3_bool_solved_genome.txt");std::printf("fastest_solved_seed=%d evals=%llu active=%u sec=%.4f\n",900000+mi,(unsigned long long)res[mi].evals,(unsigned)res[mi].active_n,res[mi].sec);}
}
