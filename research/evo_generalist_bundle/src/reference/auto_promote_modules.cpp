
#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

static constexpr int D=64, O=10, SH=32, HN=16;
static constexpr int MODS=8, MN=4;
static constexpr int PRIM_OPS=15, CALL_OP=15, OPS=16, LIM=1000000;

enum Op:uint8_t {
    PASS,CONSTV,ADD,SUB,MULQ,XORV,ANDV,ORV,NOTV,ABSV,MINV,MAXV,NEG,SELECTP,AVG,CALLM
};

struct Node{uint8_t op,a,b,c;};
struct Module{Node n[MN];uint8_t out;};
struct Genome{
    Node shared[SH];
    Node head[O][HN];
    uint8_t out[O];
    Module mod[MODS];
};
struct Ex{int32_t x[D];int32_t t[O];uint8_t label;};

struct RNG{
    uint64_t s; RNG(uint64_t z):s(z?z:1){}
    inline uint64_t next(){uint64_t x=s;x^=x>>12;x^=x<<25;x^=x>>27;s=x;return x*2685821657736338717ULL;}
    inline uint32_t u32(){return (uint32_t)next();}
    inline int range(int n){return (int)(next()%n);}
};
static inline int32_t clamp64(int64_t v){return v>LIM?LIM:(v<-LIM?-LIM:(int32_t)v);}

static vector<Ex> readcsv(const string&p){
    ifstream f(p); vector<Ex>a; string line;
    while(getline(f,line)){
        if(line.empty()) continue;
        stringstream ss(line); string z; vector<int>v;
        while(getline(ss,z,',')) v.push_back(stoi(z));
        if((int)v.size()!=D+O+1) continue;
        Ex e{}; for(int i=0;i<D;i++)e.x[i]=v[i];
        for(int o=0;o<O;o++)e.t[o]=v[D+o];
        e.label=(uint8_t)v[D+O]; a.push_back(e);
    }
    return a;
}

static inline int32_t prim_eval(uint8_t op,int32_t A,int32_t B,uint8_t c){
    switch(op%PRIM_OPS){
        case PASS:return A; case CONSTV:return (int8_t)c;
        case ADD:return clamp64((int64_t)A+B); case SUB:return clamp64((int64_t)A-B);
        case MULQ:return clamp64(((int64_t)A*B)>>5);
        case XORV:return A^B; case ANDV:return A&B; case ORV:return A|B; case NOTV:return ~A;
        case ABSV:return A==INT32_MIN?INT32_MAX:abs(A); case MINV:return min(A,B); case MAXV:return max(A,B);
        case NEG:return clamp64(-(int64_t)A); case SELECTP:return A>=0?B:(int8_t)c;
        case AVG:return (int32_t)(((int64_t)A+B)/2);
    }
    return 0;
}

static inline int32_t eval_module(const Module&m,int32_t x0,int32_t x1){
    int32_t v[MN];
    for(int i=0;i<MN;i++){
        const Node&n=m.n[i];
        auto get=[&](uint8_t s)->int32_t{
            if(s==0) return x0;
            if(s==1) return x1;
            int j=(int)s-2;
            return (j>=0&&j<i)?v[j]:0;
        };
        v[i]=prim_eval(n.op,get(n.a),get(n.b),n.c);
    }
    return v[m.out%MN];
}

static inline uint8_t sh_src(RNG&r,int i){return (uint8_t)r.range(D+i);}
static inline uint8_t h_src(RNG&r,int i){return (uint8_t)r.range(D+SH+i);}
static inline uint8_t m_src(RNG&r,int i){return (uint8_t)r.range(2+i);}

struct Active{
    bool sh[SH]{};
    bool h[O][HN]{};
    bool mod[MODS]{};
    int nsh=0,nh=0,nmod=0;
};

static Active compute_active(const Genome&g){
    Active a{};
    // Active head nodes.
    for(int o=0;o<O;o++){
        int st[HN],sp=0; st[sp++]=g.out[o]%HN;
        while(sp){
            int i=st[--sp]; if(i<0||i>=HN||a.h[o][i])continue;
            a.h[o][i]=true; a.nh++;
            const Node&n=g.head[o][i]; int op=n.op%OPS;
            if(op==CALL_OP) a.mod[n.c%MODS]=true;
            auto push=[&](uint8_t s){
                if(s>=D+SH){int j=(int)s-(D+SH);if(j>=0&&j<i&&!a.h[o][j])st[sp++]=j;}
            };
            if(op!=CONSTV){
                push(n.a);
                if(op!=PASS&&op!=NOTV&&op!=ABSV&&op!=NEG) push(n.b);
            }
        }
    }
    // Shared roots referenced from heads.
    int st[SH*O*2],sp=0;
    for(int o=0;o<O;o++)for(int i=0;i<HN;i++)if(a.h[o][i]){
        const Node&n=g.head[o][i]; int op=n.op%OPS;
        auto add=[&](uint8_t s){
            if(s>=D&&s<D+SH){int j=(int)s-D;if(!a.sh[j])st[sp++]=j;}
        };
        if(op!=CONSTV){add(n.a);if(op!=PASS&&op!=NOTV&&op!=ABSV&&op!=NEG)add(n.b);}
    }
    while(sp){
        int i=st[--sp]; if(i<0||i>=SH||a.sh[i])continue;
        a.sh[i]=true; a.nsh++;
        const Node&n=g.shared[i]; int op=n.op%OPS;
        if(op==CALL_OP) a.mod[n.c%MODS]=true;
        auto push=[&](uint8_t s){
            if(s>=D){int j=(int)s-D;if(j>=0&&j<i&&!a.sh[j])st[sp++]=j;}
        };
        if(op!=CONSTV){push(n.a);if(op!=PASS&&op!=NOTV&&op!=ABSV&&op!=NEG)push(n.b);}
    }
    for(int k=0;k<MODS;k++) if(a.mod[k]) a.nmod += MN;
    return a;
}

static inline int32_t main_eval(const Genome&g,const Node&n,int32_t A,int32_t B){
    if((n.op%OPS)==CALL_OP) return eval_module(g.mod[n.c%MODS],A,B);
    return prim_eval(n.op,A,B,n.c);
}

static inline void outputs(const Genome&g,const Active&a,const Ex&e,int32_t out[O]){
    int32_t sv[SH],hv[HN];
    for(int i=0;i<SH;i++) if(a.sh[i]){
        const Node&n=g.shared[i];
        auto get=[&](uint8_t s)->int32_t{
            if(s<D)return e.x[s];
            int j=(int)s-D;return (j>=0&&j<i)?sv[j]:0;
        };
        sv[i]=main_eval(g,n,get(n.a),get(n.b));
    }
    for(int o=0;o<O;o++){
        for(int i=0;i<HN;i++) if(a.h[o][i]){
            const Node&n=g.head[o][i];
            auto get=[&](uint8_t s)->int32_t{
                if(s<D)return e.x[s];
                if(s<D+SH)return sv[(int)s-D];
                int j=(int)s-(D+SH);return (j>=0&&j<i)?hv[j]:0;
            };
            hv[i]=main_eval(g,n,get(n.a),get(n.b));
        }
        out[o]=hv[g.out[o]%HN];
    }
}
static inline int argmax10(const int32_t a[O]){int p=0;for(int o=1;o<O;o++)if(a[o]>a[p])p=o;return p;}

struct Fit{
    int64_t loss=0;
    int agreement=0;
    int active=0;
    int64_t head_err[O]{};
};

static Fit evaluate(const Genome&g,const vector<Ex>&set){
    Active a=compute_active(g);
    Fit f; f.active=a.nsh+a.nh+a.nmod;
    for(auto&e:set){
        int32_t out[O]; outputs(g,a,e,out);
        int tp=0;for(int o=1;o<O;o++)if(e.t[o]>e.t[tp])tp=o;
        int sp=argmax10(out);
        f.agreement+=(sp==tp);
        int64_t l1=0;
        for(int o=0;o<O;o++){
            int64_t er=llabs((long long)out[o]-e.t[o]);
            f.head_err[o]+=er; l1+=er;
        }
        f.loss += l1 + (sp!=tp?3000:0);
    }
    return f;
}
static inline bool better(const Fit&a,const Fit&b){
    if(a.loss!=b.loss)return a.loss<b.loss;
    if(a.agreement!=b.agreement)return a.agreement>b.agreement;
    return a.active<b.active;
}

static Genome random_genome(RNG&r){
    Genome g{};
    for(int i=0;i<SH;i++){g.shared[i].op=r.range(OPS);g.shared[i].a=sh_src(r,i);g.shared[i].b=sh_src(r,i);g.shared[i].c=r.u32();}
    for(int o=0;o<O;o++)for(int i=0;i<HN;i++){g.head[o][i].op=r.range(OPS);g.head[o][i].a=h_src(r,i);g.head[o][i].b=h_src(r,i);g.head[o][i].c=r.u32();}
    for(int o=0;o<O;o++)g.out[o]=r.range(HN);
    for(int k=0;k<MODS;k++){
        for(int i=0;i<MN;i++){g.mod[k].n[i].op=r.range(PRIM_OPS);g.mod[k].n[i].a=m_src(r,i);g.mod[k].n[i].b=m_src(r,i);g.mod[k].n[i].c=r.u32();}
        g.mod[k].out=r.range(MN);
    }
    return g;
}

struct Delta{uint16_t off;uint8_t old;};

static inline void mutate_main_node(Node&n,int ni,bool shared,RNG&r,Delta&d,uint16_t baseoff){
    int q=r.range(100);
    if(q<30){
        d={baseoff,n.op};uint8_t z=n.op;while(z==n.op)z=r.range(OPS);n.op=z;
    }else if(q<62){
        d={(uint16_t)(baseoff+1),n.a};uint8_t z=n.a;while(z==n.a)z=shared?sh_src(r,ni):h_src(r,ni);n.a=z;
    }else if(q<94){
        d={(uint16_t)(baseoff+2),n.b};uint8_t z=n.b;while(z==n.b)z=shared?sh_src(r,ni):h_src(r,ni);n.b=z;
    }else{
        d={(uint16_t)(baseoff+3),n.c};uint8_t z=n.c;while(z==n.c)z=r.u32();n.c=z;
    }
}
static inline void mutate_module_gene(Module&m,int ni,RNG&r,Delta&d,uint16_t baseoff){
    int q=r.range(100);
    if(q<28){
        d={baseoff,m.n[ni].op};uint8_t z=m.n[ni].op;while(z==m.n[ni].op)z=r.range(PRIM_OPS);m.n[ni].op=z;
    }else if(q<62){
        d={(uint16_t)(baseoff+1),m.n[ni].a};uint8_t z=m.n[ni].a;while(z==m.n[ni].a)z=m_src(r,ni);m.n[ni].a=z;
    }else if(q<96){
        d={(uint16_t)(baseoff+2),m.n[ni].b};uint8_t z=m.n[ni].b;while(z==m.n[ni].b)z=m_src(r,ni);m.n[ni].b=z;
    }else{
        d={(uint16_t)(baseoff+3),m.n[ni].c};uint8_t z=m.n[ni].c;while(z==m.n[ni].c)z=r.u32();m.n[ni].c=z;
    }
}

static inline int choosek(uint64_t stall,RNG&r){
    int q=r.range(10000);
    if(stall<20000){if(q<9000)return 1;if(q<9900)return 2;return 4;}
    if(stall<100000){if(q<7600)return 1;if(q<9200)return 2;if(q<9900)return 4;return 8;}
    if(stall<300000){if(q<6000)return 1;if(q<8000)return 2;if(q<9400)return 4;if(q<9900)return 8;return 16;}
    if(q<4500)return 1;if(q<7000)return 2;if(q<8800)return 4;if(q<9700)return 8;return 16;
}
static int sample_head(const Fit&f,RNG&r){
    long double total=0;for(int o=0;o<O;o++)total+=(long double)f.head_err[o]+1;
    long double u=(long double)(r.next()>>11)/(long double)(1ULL<<53)*total,c=0;
    for(int o=0;o<O;o++){c+=(long double)f.head_err[o]+1;if(u<=c)return o;}return O-1;
}
static inline int pick_sh(const Active&a,RNG&r){int ids[SH],n=0;for(int i=0;i<SH;i++)if(a.sh[i])ids[n++]=i;return n?ids[r.range(n)]:r.range(SH);}
static inline int pick_h(const Active&a,int o,RNG&r){int ids[HN],n=0;for(int i=0;i<HN;i++)if(a.h[o][i])ids[n++]=i;return n?ids[r.range(n)]:r.range(HN);}
static inline int pick_active_mod(const Active&a,RNG&r){int ids[MODS],n=0;for(int k=0;k<MODS;k++)if(a.mod[k])ids[n++]=k;return n?ids[r.range(n)]:-1;}
static inline int pick_inactive_mod(const Active&a,RNG&r){int ids[MODS],n=0;for(int k=0;k<MODS;k++)if(!a.mod[k])ids[n++]=k;return n?ids[r.range(n)]:-1;}


static inline bool op_uses_b(int op){
    return !(op==CONSTV || op==PASS || op==NOTV || op==ABSV || op==NEG);
}
static inline bool op_uses_a(int op){ return op!=CONSTV; }

// Automatically abstract a useful active subgraph into a 2-argument module.
// The replacement is accepted only if it is exactly function-preserving on the
// current training set. This is a generic graph compression step: it knows
// nothing about images/classes.
static bool try_promote_module(Genome &g, const vector<Ex>&train, RNG &r){
    Active aa=compute_active(g);
    int freeMods[MODS], nf=0;
    for(int k=0;k<MODS;k++) if(!aa.mod[k]) freeMods[nf++]=k;
    if(!nf) return false;

    Fit before=evaluate(g,train);

    for(int attempt=0;attempt<32;attempt++){
        bool useShared = (r.range(100)<35);
        int ho=-1, root=-1;

        if(useShared){
            int ids[SH],n=0;
            for(int i=1;i<SH;i++) if(aa.sh[i] && (g.shared[i].op%OPS)!=CALL_OP) ids[n++]=i;
            if(!n) continue;
            root=ids[r.range(n)];
        }else{
            ho=r.range(O);
            int ids[HN],n=0;
            for(int i=1;i<HN;i++) if(aa.h[ho][i] && (g.head[ho][i].op%OPS)!=CALL_OP) ids[n++]=i;
            if(!n) continue;
            root=ids[r.range(n)];
        }

        auto getnode = [&](int i)->const Node&{
            return useShared ? g.shared[i] : g.head[ho][i];
        };
        auto dep_from_src = [&](uint8_t s,int i)->int{
            if(useShared){
                if(s>=D){int j=(int)s-D; if(j>=0&&j<i) return j;}
            }else{
                if(s>=D+SH){int j=(int)s-(D+SH); if(j>=0&&j<i) return j;}
            }
            return -1;
        };

        // Grow a small backwards cone (max 4 nodes).
        vector<int> internal; internal.push_back(root);
        for(size_t q=0;q<internal.size() && internal.size()<MN;q++){
            int i=internal[q]; const Node&n=getnode(i); int op=n.op%OPS;
            if(op==CALL_OP) continue;
            int da=op_uses_a(op)?dep_from_src(n.a,i):-1;
            int db=op_uses_b(op)?dep_from_src(n.b,i):-1;
            int deps[2]={da,db};
            for(int z=0;z<2 && internal.size()<MN;z++){
                int d=deps[z];
                if(d<0) continue;
                if((getnode(d).op%OPS)==CALL_OP) continue;
                if(find(internal.begin(),internal.end(),d)==internal.end())
                    internal.push_back(d);
            }
        }
        if(internal.size()<2) continue;
        sort(internal.begin(),internal.end());

        auto is_internal=[&](int j){return find(internal.begin(),internal.end(),j)!=internal.end();};

        vector<int> ext;
        auto add_external=[&](uint8_t s,int i){
            int d=dep_from_src(s,i);
            if(d>=0 && is_internal(d)) return;
            int key=(int)s;
            if(find(ext.begin(),ext.end(),key)==ext.end()) ext.push_back(key);
        };

        bool bad=false;
        for(int i:internal){
            const Node&n=getnode(i); int op=n.op%OPS;
            if(op==CALL_OP){bad=true;break;}
            if(op_uses_a(op)) add_external(n.a,i);
            if(op_uses_b(op)) add_external(n.b,i);
            if(ext.size()>2){bad=true;break;}
        }
        if(bad || ext.empty() || ext.size()>2) continue;
        if(ext.size()==1) ext.push_back(ext[0]);

        int mk=freeMods[r.range(nf)];
        Genome old=g;

        // Translate selected subgraph into formal module inputs 0/1.
        Module nm{};
        for(int p=0;p<MN;p++){ nm.n[p].op=PASS; nm.n[p].a=0; nm.n[p].b=0; nm.n[p].c=0; }
        for(int p=0;p<(int)internal.size();p++){
            int oi=internal[p]; const Node&on=getnode(oi);
            Node nn=on;
            auto translate=[&](uint8_t s)->uint8_t{
                int d=dep_from_src(s,oi);
                if(d>=0 && is_internal(d)){
                    int pos=find(internal.begin(),internal.end(),d)-internal.begin();
                    return (uint8_t)(2+pos);
                }
                return (s==(uint8_t)ext[0])?0:1;
            };
            if(op_uses_a(nn.op%OPS)) nn.a=translate(on.a); else nn.a=0;
            if(op_uses_b(nn.op%OPS)) nn.b=translate(on.b); else nn.b=0;
            nn.op%=PRIM_OPS;
            nm.n[p]=nn;
        }
        int rootpos=find(internal.begin(),internal.end(),root)-internal.begin();
        nm.out=(uint8_t)rootpos;
        g.mod[mk]=nm;

        Node repl{};
        repl.op=CALL_OP; repl.a=(uint8_t)ext[0]; repl.b=(uint8_t)ext[1]; repl.c=(uint8_t)mk;
        if(useShared) g.shared[root]=repl; else g.head[ho][root]=repl;

        Fit after=evaluate(g,train);
        // exact same phenotype on the active training set
        if(after.loss==before.loss && after.agreement==before.agreement){
            return true;
        }
        g=old;
    }
    return false;
}

static void mine(const Genome&g,vector<Ex>&train,const vector<Ex>&chal){
    if(chal.empty())return;Active a=compute_active(g);
    auto sloss=[&](const Ex&e){
        int32_t out[O];outputs(g,a,e,out);
        int tp=0;for(int o=1;o<O;o++)if(e.t[o]>e.t[tp])tp=o;
        int sp=argmax10(out);int64_t l=sp!=tp?3000:0;
        for(int o=0;o<O;o++)l+=llabs((long long)out[o]-e.t[o]);return l;
    };
    vector<pair<int64_t,int>>cw,tw;
    for(int i=0;i<(int)chal.size();i++)cw.push_back({sloss(chal[i]),i});
    for(int i=0;i<(int)train.size();i++)tw.push_back({sloss(train[i]),i});
    int k=min(12,min((int)train.size(),(int)chal.size()));
    partial_sort(cw.begin(),cw.begin()+k,cw.end(),[](auto&a,auto&b){return a.first>b.first;});
    partial_sort(tw.begin(),tw.begin()+k,tw.end(),[](auto&a,auto&b){return a.first<b.first;});
    for(int i=0;i<k;i++)train[tw[i].second]=chal[cw[i].second];
}

struct Metrics{double agree,acc,mae;};
static Metrics metrics(const Genome&g,const vector<Ex>&set){
    Active a=compute_active(g);int ag=0,ac=0;double ae=0;
    for(auto&e:set){
        int32_t out[O];outputs(g,a,e,out);int sp=argmax10(out),tp=0;
        for(int o=1;o<O;o++)if(e.t[o]>e.t[tp])tp=o;
        ag+=(sp==tp);ac+=(sp==e.label);
        for(int o=0;o<O;o++)ae+=abs((double)out[o]-e.t[o]);
    }
    return {(double)ag/set.size(),(double)ac/set.size(),ae/(set.size()*O)};
}

struct Res{int seed;double initAcc,agree,acc,mae;int active,activeMods;uint64_t evals;double sec;};

static Res runone(int seed,uint64_t budget,const vector<Ex>&pool,const vector<Ex>&test){
    RNG r(0xC6BC279692B5CC83ULL^(uint64_t)seed*0x9E3779B97F4A7C15ULL);
    vector<Ex>p=pool;mt19937 sh(seed);shuffle(p.begin(),p.end(),sh);

    int nsel=min(256,(int)p.size()/4);
    vector<Ex>select(p.end()-nsel,p.end());p.resize(p.size()-nsel);
    int nt=min(128,(int)p.size());
    vector<Ex>train(p.begin(),p.begin()+nt),chal(p.begin()+nt,p.end());

    Genome g=random_genome(r),champ=g,genchamp=g;
    Fit f=evaluate(g,train),cf=f;
    Metrics gm=metrics(genchamp,select);
    double init=metrics(g,test).acc;
    uint64_t stall=0,ev=1;double t0=omp_get_wtime();

    auto sel_better=[&](const Genome&cand,const Metrics&cm,const Genome&old,const Metrics&om){
        if(cm.agree!=om.agree)return cm.agree>om.agree;
        if(cm.mae!=om.mae)return cm.mae<om.mae;
        return compute_active(cand).nsh+compute_active(cand).nh+compute_active(cand).nmod <
               compute_active(old).nsh+compute_active(old).nh+compute_active(old).nmod;
    };

    while(ev<budget){
        Active a=compute_active(g);

        // Free drift inside an unused module: cannot affect phenotype until CALL enters active graph.
        if((r.u32()&15u)==0){
            int mk=pick_inactive_mod(a,r);
            if(mk>=0){
                int ni=r.range(MN);Delta dd;
                uint16_t base=offsetof(Genome,mod)+mk*sizeof(Module)+ni*sizeof(Node);
                mutate_module_gene(g.mod[mk],ni,r,dd,base);
                if(r.range(12)==0)g.mod[mk].out=r.range(MN);
            }
        }

        int k=choosek(stall,r);Delta ds[40];int nd=0;
        for(int m=0;m<k;m++){
            a=compute_active(g);int mode=r.range(100);

            if(mode<7){
                int o=sample_head(f,r);
                uint16_t off=offsetof(Genome,out)+o;
                ds[nd]={off,g.out[o]};uint8_t z=g.out[o];while(z==g.out[o])z=r.range(HN);g.out[o]=z;nd++;
            }else if(mode<24){
                int ni=pick_sh(a,r);
                uint16_t off=offsetof(Genome,shared)+ni*sizeof(Node);
                mutate_main_node(g.shared[ni],ni,true,r,ds[nd++],off);
            }else if(mode<82){
                int o=sample_head(f,r),ni=pick_h(a,o,r);
                uint16_t off=offsetof(Genome,head)+(o*HN+ni)*sizeof(Node);
                mutate_main_node(g.head[o][ni],ni,false,r,ds[nd++],off);
            }else{
                int mk=pick_active_mod(a,r);
                if(mk<0){
                    int o=sample_head(f,r),ni=pick_h(a,o,r);
                    uint16_t off=offsetof(Genome,head)+(o*HN+ni)*sizeof(Node);
                    mutate_main_node(g.head[o][ni],ni,false,r,ds[nd++],off);
                }else{
                    int ni=r.range(MN);
                    uint16_t off=offsetof(Genome,mod)+mk*sizeof(Module)+ni*sizeof(Node);
                    mutate_module_gene(g.mod[mk],ni,r,ds[nd++],off);
                }
            }
        }

        Fit nf=evaluate(g,train);ev++;
        if(better(nf,f)||(nf.loss==f.loss&&nf.active<=f.active&&r.range(4)==0)){
            bool strict=better(nf,f);f=nf;if(strict)stall=0;else stall++;
            if(better(f,cf)){cf=f;champ=g;}
        }else{
            uint8_t*pb=(uint8_t*)&g;for(int j=nd-1;j>=0;j--)pb[ds[j].off]=ds[j].old;stall++;
        }

        if(ev%25000==0){
            // Promote one function-preserving active subgraph when possible.
            // This creates reusable functions from structures the run already discovered.
            if(try_promote_module(g,train,r)){
                f=evaluate(g,train);
                if(better(f,cf)){cf=f;champ=g;}
            }
            Metrics cm=metrics(champ,select);
            if(sel_better(champ,cm,genchamp,gm)){genchamp=champ;gm=cm;}
            if(!chal.empty()){mine(champ,train,chal);f=evaluate(g,train);cf=evaluate(champ,train);stall=0;}
        }
    }

    Metrics cm=metrics(champ,select);
    if(sel_better(champ,cm,genchamp,gm)){genchamp=champ;gm=cm;}

    auto mm=metrics(genchamp,test);Active aa=compute_active(genchamp);
    int am=0;for(int k=0;k<MODS;k++)am+=aa.mod[k];
    return {seed,init,mm.agree,mm.acc,mm.mae,aa.nsh+aa.nh+aa.nmod,am,ev,omp_get_wtime()-t0};
}

int main(int argc,char**argv){
    if(argc<5)return 2;
    auto pool=readcsv(argv[1]),test=readcsv(argv[2]);
    int runs=atoi(argv[3]);uint64_t budget=strtoull(argv[4],0,10);
    vector<Res>rr(runs);
    #pragma omp parallel for schedule(dynamic)
    for(int i=0;i<runs;i++)rr[i]=runone(50000+i,budget,pool,test);

    cout<<"seed,initial_acc,teacher_agreement,true_acc,score_mae,active,active_modules,evals,seconds\n";
    for(auto&r:rr)cout<<r.seed<<","<<r.initAcc<<","<<r.agree<<","<<r.acc<<","<<r.mae<<","<<r.active<<","<<r.activeMods<<","<<r.evals<<","<<fixed<<setprecision(6)<<r.sec<<"\n";
}
