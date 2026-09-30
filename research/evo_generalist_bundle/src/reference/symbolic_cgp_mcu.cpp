#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

static constexpr int MAX_NODES = 64;
static constexpr int TERMINALS = 16;
static constexpr int LAMBDA = 4;
static constexpr int MAX_STEPS = 250000;

#pragma pack(push,1)
struct Node {
    uint8_t op;
    uint8_t a;
    uint8_t b;
    uint8_t c;
};
struct Genome {
    Node nodes[MAX_NODES];
    uint8_t out;
};
#pragma pack(pop)

static_assert(sizeof(Node) == 4, "Node must be 4 bytes");
static_assert(sizeof(Genome) == MAX_NODES*4 + 1, "Unexpected genome size");

enum Op : uint8_t {
    PASS=0, ADD, SUB, MUL, XOR_, AND_, OR_, MOD_, EQ_, LT_, GT_,
    IS_DIGIT, IS_LETTER, SELECT_, MIN_, MAX_, NOPS
};

struct Example { std::string in, out; };

static inline int clampv(int x) {
    if (x < -1024) return -1024;
    if (x > 1024) return 1024;
    return x;
}

static inline int src_value(uint8_t gene, int node_i,
                            const std::array<int, TERMINALS>& t,
                            const std::array<int, MAX_NODES>& vals) {
    int avail = TERMINALS + node_i;
    int s = gene % avail;
    if (s < TERMINALS) return t[s];
    return vals[s - TERMINALS];
}

static int eval_pos(const Genome& g, const std::string& in, int pos, int prev_out) {
    std::array<int, TERMINALS> t{};
    t[0] = (pos < (int)in.size()) ? (unsigned char)in[pos] : 0; // current input char
    t[1] = pos;
    t[2] = (int)in.size();
    t[3] = prev_out;
    t[4] = 0;
    t[5] = 1;
    t[6] = '0';
    t[7] = '9';
    for (int k=0;k<8;k++) t[8+k] = (k < (int)in.size()) ? (unsigned char)in[k] : 0;

    std::array<int, MAX_NODES> v{};
    for (int i=0;i<MAX_NODES;i++) {
        const Node &n = g.nodes[i];
        int a = src_value(n.a, i, t, v);
        int b = src_value(n.b, i, t, v);
        int c = src_value(n.c, i, t, v);
        int r = 0;
        switch (n.op % NOPS) {
            case PASS: r=a; break;
            case ADD: r=a+b; break;
            case SUB: r=a-b; break;
            case MUL: r=a*b; break;
            case XOR_: r=a^b; break;
            case AND_: r=a&b; break;
            case OR_: r=a|b; break;
            case MOD_: r=(b==0)?a:(a%b); break;
            case EQ_: r=(a==b); break;
            case LT_: r=(a<b); break;
            case GT_: r=(a>b); break;
            case IS_DIGIT: r=(a>='0' && a<='9'); break;
            case IS_LETTER: r=((a>='A'&&a<='Z')||(a>='a'&&a<='z')); break;
            case SELECT_: r=(a!=0)?b:c; break;
            case MIN_: r=std::min(a,b); break;
            case MAX_: r=std::max(a,b); break;
        }
        v[i] = clampv(r);
    }
    int total = TERMINALS + MAX_NODES;
    int s = g.out % total;
    if (s < TERMINALS) return t[s];
    return v[s-TERMINALS];
}

static std::string run_genome(const Genome& g, const std::string& in, int out_len) {
    std::string s;
    s.reserve(out_len);
    int prev=0;
    for (int p=0;p<out_len;p++) {
        int x = eval_pos(g,in,p,prev);
        x = std::clamp(x, 0, 127);
        s.push_back((char)x);
        prev=x;
    }
    return s;
}

// Lower is better. Exact match = 0.
static int fitness(const Genome& g, const std::vector<Example>& ex) {
    int f=0;
    for (const auto& e: ex) {
        int prev=0;
        for (int p=0;p<(int)e.out.size();p++) {
            int got = eval_pos(g,e.in,p,prev);
            got = std::clamp(got,0,127);
            int want = (unsigned char)e.out[p];
            if (got != want) {
                f += 100 + std::min(99, std::abs(got-want));
            }
            prev=got;
        }
    }
    return f;
}

static int arity(uint8_t op) {
    switch(op % NOPS) {
        case PASS: case IS_DIGIT: case IS_LETTER: return 1;
        case SELECT_: return 3;
        default: return 2;
    }
}

static void mark_source(uint8_t gene, int node_i, const Genome& g, std::array<uint8_t,MAX_NODES>& used);
static void mark_node(int ni, const Genome& g, std::array<uint8_t,MAX_NODES>& used) {
    if (ni < 0 || ni >= MAX_NODES || used[ni]) return;
    used[ni]=1;
    const Node& n=g.nodes[ni];
    int a=arity(n.op);
    mark_source(n.a,ni,g,used);
    if(a>=2) mark_source(n.b,ni,g,used);
    if(a>=3) mark_source(n.c,ni,g,used);
}
static void mark_source(uint8_t gene, int node_i, const Genome& g, std::array<uint8_t,MAX_NODES>& used) {
    int s = gene % (TERMINALS + node_i);
    if (s >= TERMINALS) mark_node(s-TERMINALS,g,used);
}
static int active_nodes(const Genome& g) {
    std::array<uint8_t,MAX_NODES> used{};
    int s=g.out % (TERMINALS+MAX_NODES);
    if (s>=TERMINALS) mark_node(s-TERMINALS,g,used);
    int c=0; for(auto x:used)c+=x;
    return c;
}

static Genome random_genome(std::mt19937& rng) {
    Genome g{};
    std::uniform_int_distribution<int> byte(0,255);
    for(auto& n:g.nodes){ n.op=byte(rng); n.a=byte(rng); n.b=byte(rng); n.c=byte(rng); }
    g.out=byte(rng);
    return g;
}

static void mutate(Genome& g, std::mt19937& rng, int mutations) {
    std::uniform_int_distribution<int> gene(0, MAX_NODES*4); // last is output gene
    std::uniform_int_distribution<int> byte(0,255);
    uint8_t* raw = reinterpret_cast<uint8_t*>(&g);
    for(int k=0;k<mutations;k++) raw[gene(rng)] = (uint8_t)byte(rng);
}

struct Result { Genome g; int fit; int gen; };

static Result evolve(const std::vector<Example>& train, uint32_t seed, int max_steps=MAX_STEPS) {
    std::mt19937 rng(seed);
    Genome parent=random_genome(rng);
    int pf=fitness(parent,train);
    std::uniform_int_distribution<int> mutcount(1,4);

    for(int gen=1;gen<=max_steps;gen++) {
        Genome best=parent;
        int bf=pf;
        bool child_won=false;
        for(int k=0;k<LAMBDA;k++) {
            Genome c=parent;
            mutate(c,rng,mutcount(rng));
            int cf=fitness(c,train);
            if (cf < bf || (cf==bf && (rng()&1))) {
                best=c; bf=cf; child_won=true;
            }
        }
        if (child_won || bf<=pf) { parent=best; pf=bf; }
        if (pf==0) return {parent,pf,gen};
    }
    return {parent,pf,max_steps};
}

static const char* opname(uint8_t op){
    static const char* n[] = {"PASS","ADD","SUB","MUL","XOR","AND","OR","MOD","EQ","LT","GT","IS_DIGIT","IS_LETTER","SELECT","MIN","MAX"};
    return n[op%NOPS];
}

static void print_active(const Genome& g) {
    std::array<uint8_t,MAX_NODES> used{};
    int os=g.out%(TERMINALS+MAX_NODES);
    if(os>=TERMINALS) mark_node(os-TERMINALS,g,used);
    std::printf("Active nodes: %d / %d\n", active_nodes(g), MAX_NODES);
    std::printf("Output source: %d\n", os);
    for(int i=0;i<MAX_NODES;i++) if(used[i]) {
        const Node& n=g.nodes[i];
        int avail=TERMINALS+i;
        std::printf("  N%02d %-9s src=(%d,%d,%d)\n", i, opname(n.op), n.a%avail, n.b%avail, n.c%avail);
    }
}

static void run_task(const char* name, const std::vector<Example>& train, const std::vector<Example>& test, int seeds, int max_steps) {
    std::printf("\n=== %s ===\n",name);
    int solved=0, generalized=0;
    long long gens=0;
    Result best{}; best.fit=1e9;
    bool have=false;
    for(int s=1;s<=seeds;s++) {
        auto r=evolve(train, 1000+s, max_steps);
        if(r.fit==0) {
            solved++; gens+=r.gen;
            bool ok=true;
            for(const auto& e:test) if(run_genome(r.g,e.in,(int)e.out.size())!=e.out) ok=false;
            if(ok) generalized++;
        }
        if(!have || r.fit<best.fit){best=r;have=true;}
    }
    std::printf("Genome bytes: %zu (nodes=%zu + output=1)\n", sizeof(Genome), sizeof(Node)*MAX_NODES);
    std::printf("Solved training: %d/%d\n",solved,seeds);
    std::printf("Generalized held-out: %d/%d seeds (%d/%d among solved)\n", generalized,seeds,generalized,solved);
    if(solved) std::printf("Mean generations to exact train solution: %.1f\n", (double)gens/solved);

    // show first solution that generalizes, else best from a deterministic rerun search
    Result show{}; bool found=false;
    for(int s=1;s<=seeds && !found;s++) {
        auto r=evolve(train,1000+s,max_steps);
        if(r.fit==0){
            bool ok=true; for(const auto& e:test) if(run_genome(r.g,e.in,(int)e.out.size())!=e.out) ok=false;
            if(ok){show=r;found=true;}
        }
    }
    if(!found) show=best;
    std::printf("Example evolved program: fitness=%d gen=%d\n",show.fit,show.gen);
    for(const auto& e:train) std::printf("  train %-8s -> %-8s target=%s\n", e.in.c_str(), run_genome(show.g,e.in,(int)e.out.size()).c_str(), e.out.c_str());
    for(const auto& e:test)  std::printf("  test  %-8s -> %-8s target=%s\n", e.in.c_str(), run_genome(show.g,e.in,(int)e.out.size()).c_str(), e.out.c_str());
    print_active(show.g);
}

int main(){
    std::printf("Symbolic CGP MCU prototype\n");
    std::printf("sizeof(Node)=%zu bytes, sizeof(Genome)=%zu bytes\n",sizeof(Node),sizeof(Genome));

    run_task("Task 1: increment one ASCII digit",
        {{"0","1"},{"1","2"},{"2","3"},{"3","4"},{"4","5"},{"5","6"},{"6","7"},{"7","8"}},
        {{"8","9"}}, 20, 120000);

    std::vector<Example> t2train={{"A1","A2"},{"B3","B4"},{"C5","C6"},{"D7","D8"}};
    std::vector<Example> t2test;
    for(char L='A';L<='Z';++L) for(char d='0';d<='8';++d){
        std::string in; in.push_back(L); in.push_back(d);
        std::string out; out.push_back(L); out.push_back(char(d+1));
        bool seen=false; for(auto &e:t2train) if(e.in==in) seen=true;
        if(!seen) t2test.push_back({in,out});
    }
    run_task("Task 2: copy letter, increment digit (230 held-out cases)",
        t2train, t2test, 20, 250000);

    std::vector<Example> t3train={{"1+1=","2"},{"2+3=","5"},{"4+2=","6"},{"5+3=","8"}};
    std::vector<Example> t3test;
    for(int a=0;a<=9;a++) for(int b=0;b<=9;b++) if(a+b<=9){
        std::string in=std::to_string(a)+"+"+std::to_string(b)+"=";
        std::string out=std::to_string(a+b);
        bool seen=false; for(auto &e:t3train) if(e.in==in) seen=true;
        if(!seen) t3test.push_back({in,out});
    }
    run_task("Task 3: single-digit addition a+b= (all unseen sums < 10)",
        t3train, t3test, 20, 250000);
}
