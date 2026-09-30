#pragma once
#include <stdint.h>

// Minimal runtime for CH32V203-class MCUs.
// Same 4-byte node / 257-byte genome layout as the host trainer.
// Put Genome/Module objects in flash/const memory when possible.

#define CGP_NODES 64
#define CGP_TERMS 4
#define CGP_MAX_MODULE_NODES 64
#define CGP_CALL_BASE 7

typedef struct __attribute__((packed)) {
    uint8_t op, a, b, c;   // c reserved for 3rd arg / parameter in future modules
} CgpNode;

typedef struct __attribute__((packed)) {
    CgpNode n[CGP_NODES];
    uint8_t out;
} CgpGenome;

typedef struct {
    uint8_t n, out, in_bits, out_bits;
    CgpNode node[CGP_MAX_MODULE_NODES];
} CgpModule;

typedef struct {
    uint8_t n;
    const CgpModule *m;
} CgpLibrary;

enum {
    CGP_PASS=0, CGP_XOR=1, CGP_AND=2, CGP_OR=3,
    CGP_NOT=4, CGP_SHL1=5, CGP_SHR1=6
};

static inline uint32_t cgp_prim(uint8_t op, uint32_t a, uint32_t b) {
    switch(op) {
        case CGP_PASS: return a;
        case CGP_XOR:  return a ^ b;
        case CGP_AND:  return a & b;
        case CGP_OR:   return a | b;
        case CGP_NOT:  return ~a;
        case CGP_SHL1: return a << 1;
        case CGP_SHR1: return a >> 1;
        default:       return a;
    }
}

static uint32_t cgp_eval_module(const CgpLibrary *lib, uint8_t mid,
                                uint32_t A, uint32_t B) {
    const CgpModule *m = &lib->m[mid];
    uint32_t v[CGP_TERMS + CGP_MAX_MODULE_NODES];
    const uint32_t im = (m->in_bits >= 32) ? 0xffffffffu : ((1u << m->in_bits) - 1u);
    const uint32_t om = (m->out_bits >= 32) ? 0xffffffffu : ((1u << m->out_bits) - 1u);
    v[0] = A & im; v[1] = B & im; v[2] = 0; v[3] = 1;
    for(uint8_t i=0;i<m->n;i++) {
        const CgpNode *n=&m->node[i];
        const uint32_t a=v[n->a], b=v[n->b];
        v[CGP_TERMS+i] = (n->op < CGP_CALL_BASE)
            ? cgp_prim(n->op,a,b)
            : cgp_eval_module(lib,(uint8_t)(n->op-CGP_CALL_BASE),a,b);
    }
    return v[m->out] & om;
}

static inline uint32_t cgp_eval(const CgpGenome *g, const CgpLibrary *lib,
                                uint32_t A, uint32_t B) {
    uint32_t v[CGP_TERMS + CGP_NODES];
    v[0]=A; v[1]=B; v[2]=0; v[3]=1;
    // Simple MCU runtime evaluates all 64 nodes: fixed time, no allocations.
    // A flash-side compiled active-node list can reduce this further.
    for(uint8_t i=0;i<CGP_NODES;i++) {
        const CgpNode *n=&g->n[i];
        const uint32_t a=v[n->a], b=v[n->b];
        v[CGP_TERMS+i] = (n->op < CGP_CALL_BASE)
            ? cgp_prim(n->op,a,b)
            : cgp_eval_module(lib,(uint8_t)(n->op-CGP_CALL_BASE),a,b);
    }
    return v[g->out];
}

#if defined(__cplusplus)
static_assert(sizeof(CgpNode)==4, "CgpNode must stay 4 bytes");
static_assert(sizeof(CgpGenome)==257, "CgpGenome must stay 257 bytes");
#endif
