
#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

static constexpr int NODES=64, TRAIN=64, CHAL=384, OPS=15;
static constexpr int LIM=1000000;

enum Op:uint8_t {PASS,CONSTV,ADD,SUB,MUL,XORV,ANDV,ORV,NOTV,SHL,SHR,ABSV,MINV,MAXV,SELECTV};
struct Node{uint8_t op,a,b,c;};
struct Genome{Node n[NODES]; uint8_t out;};
struct Ex{int32_t x,y,t;};

struct RNG{
    uint64_t s;
    RNG(uint64_t x):s(x?x:1){}
    inline uint64_t next(){uint64_t x=s;x^=x>>12;x^=x<<25;x^=x>>27;s=x;return x*2685821657736338717ULL;}
    inline uint32_t u32(){return (uint32_t)next();}
    inline int range(int n){return (int)(next()%n);}
};

static inline int32_t clamp64(int64_t v){ if(v>LIM)return LIM; if(v<-LIM)return -LIM; return (int32_t)v; }
static inline int32_t target(int32_t x,int32_t y){
    int64_t a=(int64_t)(x+y)*(x-2*y)+3LL*x-5;
    int64_t b=x-y+2;
    int64_t c=llabs((long long)x+3LL*y);
    int64_t d=((uint32_t)x & 7u)<<2;
    return clamp64(a*b+c-d);
}
static inline int32_t shr_arith(int32_t a,int s){ return a >> (s&7); }
static inline int32_t shl_safe(int32_t a,int s){ return clamp64((int64_t)a << (s&7)); }

static inline uint8_t src_rand(RNG&r,int i){ return (uint8_t)r.range(i+2); } // 0=x,1=y, 2..i+1 prev nodes
static inline int32_t getsrc(uint8_t s,int i,int32_t x,int32_t y,const int32_t *v){
    if(s==0)return x; if(s==1)return y;
    int j=(int)s-2; return (j>=0 && j<i)?v[j]:0;
}
static inline int32_t execnode(const Node&n,int i,int32_t x,int32_t y,const int32_t*v){
    int32_t A=getsrc(n.a,i,x,y,v), B=getsrc(n.b,i,x,y,v);
    switch(n.op%OPS){
        case PASS:return A;
        case CONSTV:return (int8_t)n.c;
        case ADD:return clamp64((int64_t)A+B);
        case SUB:return clamp64((int64_t)A-B);
        case MUL:return clamp64((int64_t)A*B);
        case XORV:return A^B;
        case ANDV:return A&B;
        case ORV:return A|B;
        case NOTV:return ~A;
        case SHL:return shl_safe(A,B);
        case SHR:return shr_arith(A,B);
        case ABSV:return A==INT32_MIN?INT32_MAX:abs(A);
        case MINV:return min(A,B);
        case MAXV:return max(A,B);
        case SELECTV:return (A>=0)?B:(int8_t)n.c;
    }
    return 0;
}
static inline void active_mask(const Genome&g,bool act[NODES]){
    memset(act,0,NODES);
    int st[NODES],sp=0,root=g.out%NODES; st[sp++]=root;
    while(sp){
        int i=st[--sp]; if(i<0||i>=NODES||act[i])continue; act[i]=true;
        const Node&n=g.n[i]; int op=n.op%OPS;
        auto push=[&](uint8_t s){if(s>=2){int j=(int)s-2;if(j>=0&&j<i&&!act[j])st[sp++]=j;}};
        if(op!=CONSTV){push(n.a); if(op!=PASS && op!=NOTV && op!=ABSV) push(n.b);}
    }
}
static inline int active_count(const Genome&g){bool a[NODES];active_mask(g,a);int n=0;for(bool z:a)n+=z;return n;}

struct Fit{int64_t abs; int exact; int active;};
static inline bool better(const Fit&a,const Fit&b){
    if(a.abs!=b.abs)return a.abs<b.abs;
    if(a.exact!=b.exact)return a.exact>b.exact;
    return a.active<b.active;
}
static inline int32_t pred(const Genome&g,const Ex&e){
    bool act[NODES];active_mask(g,act); int32_t v[NODES]; int root=g.out%NODES;
    for(int i=0;i<NODES;i++)if(act[i])v[i]=execnode(g.n[i],i,e.x,e.y,v);
    return v[root];
}
static inline Fit evaluate(const Genome&g,const vector<Ex>&train){
    int64_t ae=0; int exact=0;
    bool act[NODES];active_mask(g,act); int ac=0;for(int i=0;i<NODES;i++)ac+=act[i];
    int root=g.out%NODES; int32_t v[NODES];
    for(const auto&e:train){
        for(int i=0;i<NODES;i++)if(act[i])v[i]=execnode(g.n[i],i,e.x,e.y,v);
        int32_t p=v[root]; ae+=llabs((long long)p-e.t); exact+=(p==e.t);
    }
    return {ae,exact,ac};
}
static inline int64_t eval_abs(const Genome&g,const vector<Ex>&set){
    int64_t ae=0; bool act[NODES];active_mask(g,act); int root=g.out%NODES; int32_t v[NODES];
    for(const auto&e:set){
        for(int i=0;i<NODES;i++)if(act[i])v[i]=execnode(g.n[i],i,e.x,e.y,v);
        ae+=llabs((long long)v[root]-e.t);
    }
    return ae;
}
static inline int eval_exact(const Genome&g,const vector<Ex>&set){
    int ex=0; bool act[NODES];active_mask(g,act); int root=g.out%NODES; int32_t v[NODES];
    for(const auto&e:set){
        for(int i=0;i<NODES;i++)if(act[i])v[i]=execnode(g.n[i],i,e.x,e.y,v);
        ex+=(v[root]==e.t);
    }
    return ex;
}

static Genome random_genome(RNG&r){
    Genome g;
    for(int i=0;i<NODES;i++){g.n[i].op=r.range(OPS);g.n[i].a=src_rand(r,i);g.n[i].b=src_rand(r,i);g.n[i].c=r.u32();}
    g.out=r.range(NODES); return g;
}
struct Delta{uint16_t off; uint8_t old;};
static inline int pick_active(const bool act[NODES],RNG&r){
    int ids[NODES],n=0;for(int i=0;i<NODES;i++)if(act[i])ids[n++]=i;return n?ids[r.range(n)]:r.range(NODES);
}
static inline int pick_inactive(const bool act[NODES],RNG&r){
    int ids[NODES],n=0;for(int i=0;i<NODES;i++)if(!act[i])ids[n++]=i;return n?ids[r.range(n)]:-1;
}
static inline void mutate_gene(Genome&g,int ni,RNG&r,Delta&d){
    Node&n=g.n[ni]; int q=r.range(100),off=ni*4;
    if(q<28){d={(uint16_t)off,n.op};uint8_t z=n.op;while(z==n.op)z=r.range(OPS);n.op=z;}
    else if(q<62){d={(uint16_t)(off+1),n.a};uint8_t z=n.a;while(z==n.a)z=src_rand(r,ni);n.a=z;}
    else if(q<96){d={(uint16_t)(off+2),n.b};uint8_t z=n.b;while(z==n.b)z=src_rand(r,ni);n.b=z;}
    else {d={(uint16_t)(off+3),n.c};uint8_t z=n.c;while(z==n.c)z=r.u32();n.c=z;}
}
static inline int k_baseline(uint64_t stall,RNG&r){
    int q=r.range(10000);
    if(stall<20000){if(q<9000)return 1;if(q<9900)return 2;return 4;}
    if(stall<100000){if(q<7600)return 1;if(q<9200)return 2;if(q<9900)return 4;return 8;}
    if(stall<500000){if(q<6000)return 1;if(q<8000)return 2;if(q<9400)return 4;if(q<9900)return 8;return 16;}
    if(q<4500)return 1;if(q<7000)return 2;if(q<8800)return 4;if(q<9700)return 8;return 16;
}
static inline int k_relative(int activeNodes,uint64_t stall,RNG&r){
    int activeGenes=max(1,activeNodes*3);
    int scale=max(2,(int)llround(sqrt((double)activeGenes)));
    int q=r.range(10000),k;
    if(stall<20000){
        if(q<7000)k=1; else if(q<9000)k=2; else if(q<9800)k=scale; else k=2*scale;
    } else if(stall<100000){
        if(q<5500)k=1; else if(q<7500)k=2; else if(q<9000)k=scale; else k=2*scale;
    } else {
        if(q<4000)k=1; else if(q<6000)k=2; else if(q<8000)k=scale; else if(q<9500)k=2*scale; else k=4*scale;
    }
    return min(k,24);
}


static inline int k_prop_jump(int activeNodes, RNG&r, int permille){
    int activeGenes=max(1,activeNodes*3);
    int k=0;
    for(int i=0;i<activeGenes;i++) if((int)(r.u32()%1000)<permille) k++;
    return max(1,min(k,24));
}
static void mine_counterexamples(const Genome&champ, vector<Ex>&train, const vector<Ex>&chal){
    struct E{int64_t err;int idx;};
    vector<E> ce; ce.reserve(chal.size());
    for(int i=0;i<(int)chal.size();i++) ce.push_back({llabs((long long)pred(champ,chal[i])-chal[i].t),i});
    partial_sort(ce.begin(),ce.begin()+8,ce.end(),[](auto&a,auto&b){return a.err>b.err;});
    vector<E> tr; tr.reserve(train.size());
    for(int i=0;i<(int)train.size();i++) tr.push_back({llabs((long long)pred(champ,train[i])-train[i].t),i});
    partial_sort(tr.begin(),tr.begin()+8,tr.end(),[](auto&a,auto&b){return a.err<b.err;});
    for(int k=0;k<8;k++) train[tr[k].idx]=chal[ce[k].idx];
}

struct Res{int config,seed;int solved;uint64_t evals;int64_t trainAbs,valAbs;int trainExact,valExact,active;double sec;};

static Res runone(int config,int seed,uint64_t budget,const vector<Ex>&baseTrain,const vector<Ex>&chal,const vector<Ex>&val){
    // 0 baseline; 1 counterexample mining; 2 final hybrid:
    // baseline local search + mining + rare proportional mutation jumps on stalls.
    bool mining=(config==1||config==2);
    vector<Ex> train=baseTrain;
    RNG r(0xA0761D6478BD642FULL ^ (uint64_t)seed*0xE7037ED1A0B428DBULL);
    Genome g=random_genome(r),champ=g; Fit f=evaluate(g,train),cf=f;
    uint64_t stall=0,evals=1;
    double t0=omp_get_wtime();

    while(evals<budget && cf.abs){
        bool act[NODES];active_mask(g,act);
        if((r.u32()&31u)==0){int ni=pick_inactive(act,r);if(ni>=0){Delta d;mutate_gene(g,ni,r,d);}}
        int k=k_baseline(stall,r);
        if(config==2 && stall>=20000 && r.range(100)<15){
            int pm = (stall<100000)?20:40; // 2% or 4% per active gene during rare escape jumps
            k=max(k,k_prop_jump(f.active,r,pm));
        }
        Delta ds[32];int nd=0;
        for(int m=0;m<k;m++){
            active_mask(g,act);
            if(r.range(100)<10){
                ds[nd]={(uint16_t)(NODES*4),g.out};uint8_t z=g.out;while(z==g.out)z=r.range(NODES);g.out=z;nd++;
            } else {
                int ni=pick_active(act,r);mutate_gene(g,ni,r,ds[nd++]);
            }
        }
        Fit nf=evaluate(g,train);evals++;
        if(better(nf,f) || (nf.abs==f.abs && nf.active<=f.active && r.range(4)==0)){
            bool strict=better(nf,f);f=nf;if(strict)stall=0;else stall++;
            if(better(f,cf)){cf=f;champ=g;}
        } else {
            uint8_t*p=(uint8_t*)&g;for(int j=nd-1;j>=0;j--)p[ds[j].off]=ds[j].old;stall++;
        }
        if(mining && evals%25000==0){
            mine_counterexamples(champ,train,chal);
            // re-score current/champion on the changed training set
            f=evaluate(g,train); cf=evaluate(champ,train);
            stall=0;
        }
    }
    double sec=omp_get_wtime()-t0;
    int64_t ta=eval_abs(champ,baseTrain), va=eval_abs(champ,val);
    int te=eval_exact(champ,baseTrain), ve=eval_exact(champ,val);
    return {config,seed,(va==0),evals,ta,va,te,ve,active_count(champ),sec};
}

int main(int argc,char**argv){
    uint64_t budget=300000; if(argc>1)budget=strtoull(argv[1],nullptr,10);
    int seeds=6;if(argc>2)seeds=atoi(argv[2]);

    vector<Ex> all;
    for(int x=-20;x<=20;x++)for(int y=-20;y<=20;y++)all.push_back({x,y,target(x,y)});
    // deterministic split
    std::mt19937 sh(1234567); shuffle(all.begin(),all.end(),sh);
    vector<Ex> train(all.begin(),all.begin()+TRAIN);
    vector<Ex> chal(all.begin()+TRAIN,all.begin()+TRAIN+CHAL);
    vector<Ex> val(all.begin()+TRAIN+CHAL,all.end());

    vector<Res> rr(3*seeds);
    #pragma omp parallel for schedule(dynamic)
    for(int z=0;z<3*seeds;z++){
        int c=z/seeds,s=z%seeds;
        rr[z]=runone(c,9000+s,budget,train,chal,val);
    }
    cout<<"config,seed,solved,evals,train_abs,val_abs,train_exact,val_exact,active,seconds\n";
    for(auto&r:rr)cout<<r.config<<","<<r.seed<<","<<r.solved<<","<<r.evals<<","<<r.trainAbs<<","<<r.valAbs<<","<<r.trainExact<<","<<r.valExact<<","<<r.active<<","<<fixed<<setprecision(6)<<r.sec<<"\n";
}
