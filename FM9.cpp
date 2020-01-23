/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vim: et sta sw=4:

    skUG - SuperCollider UGen Library
    Copyright (c) 2005-2008 Stefan Kersten. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
    USA
*/

#include <SC_PlugIn.h>
#include <math.h>

static InterfaceTable *ft;

struct FM9Op
{
    int32 m_phase;
    float m_freq;
    float m_phasemod;
    float m_amp;
    float m_mem;
};

struct FM9 : public Unit
{
    static const int kNumOps  = 9;
    static const int kNumOps2 = kNumOps * kNumOps;

    enum {
        kFreq = 0,
        kPhase,
        kAmp,
        kNumControls
    };

    double m_cpstoinc;
    double m_radtoinc;
    int32  m_lomask;
    FM9Op  m_ops[kNumOps];
    float  m_mod[kNumOps2];
};

extern "C"
{
    void FM9_Ctor(FM9 *unit);
    void FM9_next_kk(FM9 *unit, int inNumSamples);
    void FM9_next_ki(FM9 *unit, int inNumSamples);
};

#define FM9_OP_INDEX(i)			((i) * FM9::kNumControls)
#define FM9_FREQ_INDEX(i)		(FM9_OP_INDEX(i) + FM9::kFreq)
#define FM9_PHASE_INDEX(i)		(FM9_OP_INDEX(i) + FM9::kPhase)
#define FM9_AMP_INDEX(i)		(FM9_OP_INDEX(i) + FM9::kAmp)
#define FM9_MOD_BASE			(FM9::kNumControls * FM9::kNumOps)

#define FM9_DECLARE(i) \
		int32 phase##i; \
        float phasemodx##i; \
		float freq##i; \
		float freqinc##i; \
        float phasemod##i; \
        float phasemodinc##i; \
		float amp##i; \
		float ampinc##i; \
		float mem##i; \
		float *zout##i

#define FM9_IMPORT(ops, i) \
		{ \
		FM9Op *op           = (ops) + i; \
		phase##i            = op->m_phase; \
		freq##i             = op->m_freq; \
		float nextfreq      = ZIN0(FM9_FREQ_INDEX(i)); \
		freqinc##i          = CALCSLOPE(nextfreq, freq##i); \
        phasemod##i         = op->m_phasemod; \
		float nextphasemod  = ZIN0(FM9_PHASE_INDEX(i)); \
        phasemodinc##i      = CALCSLOPE(nextphasemod, phasemod##i); \
		amp##i              = op->m_amp; \
		float nextamp       = ZIN0(FM9_AMP_INDEX(i)); \
		ampinc##i           = CALCSLOPE(nextamp, amp##i); \
		mem##i              = op->m_mem; \
		zout##i             = ZOUT(i); \
		}

#define FM9_EXPORT(ops, i) \
		{ \
		FM9Op *op           = (ops) + i; \
		op->m_phase         = phase##i; \
		op->m_freq          = freq##i; \
        op->m_phasemod      = phasemod##i; \
		op->m_amp           = amp##i; \
		op->m_mem           = mem##i; \
		}

#define FM9_PHASE_MOD(mod, j) \
		((mod)[j] * mem##j)

#define FM9_MOD_NEXT(i, mod) \
		phasemodx##i = phasemod##i + \
		               FM9_PHASE_MOD(mod, 0) + FM9_PHASE_MOD(mod, 1) + \
		               FM9_PHASE_MOD(mod, 2) + FM9_PHASE_MOD(mod, 3) + \
		               FM9_PHASE_MOD(mod, 4) + FM9_PHASE_MOD(mod, 5) + \
		               FM9_PHASE_MOD(mod, 6) + FM9_PHASE_MOD(mod, 7) + \
		               FM9_PHASE_MOD(mod, 8)

#define FM9_OP_NEXT(i, t0, t1, tmask, radtoinc, cpstoinc) \
     mem##i = amp##i * \
              lookupi1((t0), (t1), \
                       phase##i + (int32)(phasemodx##i * (radtoinc)), \
                       (tmask)); \
     ZXP(zout##i) = mem##i; \
     phase##i    += (int32)(freq##i * (cpstoinc)); \
     freq##i     += freqinc##i; \
     phasemod##i += phasemodinc##i; \
     amp##i      += ampinc##i;

void FM9_next_kk(FM9 *unit, int inNumSamples)
{
    const int32 lomask = unit->m_lomask;
    const float cpstoinc = unit->m_cpstoinc;
    const float radtoinc = unit->m_radtoinc;

    FM9Op *ops = unit->m_ops;

    float *table0 = ft->mSineWavetable;
    float *table1 = table0 + 1;

    float* mod0 = unit->m_mod;
    float* mod1 = mod0 + FM9::kNumOps;
    float* mod2 = mod1 + FM9::kNumOps;
    float* mod3 = mod2 + FM9::kNumOps;
    float* mod4 = mod3 + FM9::kNumOps;
    float* mod5 = mod4 + FM9::kNumOps;
    float* mod6 = mod5 + FM9::kNumOps;
    float* mod7 = mod6 + FM9::kNumOps;
    float* mod8 = mod7 + FM9::kNumOps;
    
    float modinc[FM9::kNumOps2];

    FM9_DECLARE(0);
    FM9_IMPORT(ops, 0);

    FM9_DECLARE(1);
    FM9_IMPORT(ops, 1);

    FM9_DECLARE(2);
    FM9_IMPORT(ops, 2);

    FM9_DECLARE(3);
    FM9_IMPORT(ops, 3);

    FM9_DECLARE(4);
    FM9_IMPORT(ops, 4);

    FM9_DECLARE(5);
    FM9_IMPORT(ops, 5);

    FM9_DECLARE(6);
    FM9_IMPORT(ops, 6);

    FM9_DECLARE(7);
    FM9_IMPORT(ops, 7);

    FM9_DECLARE(8);
    FM9_IMPORT(ops, 8);

    for (int i=0, mi = FM9_MOD_BASE; i < FM9::kNumOps2; i++, mi++) {
        float next = ZIN0(mi);
        modinc[i] = CALCSLOPE(next, mod0[i]);
    }

    LooP(inNumSamples) {
        FM9_MOD_NEXT(0, mod0);
        FM9_MOD_NEXT(1, mod1);
        FM9_MOD_NEXT(2, mod2);
        FM9_MOD_NEXT(3, mod3);
        FM9_MOD_NEXT(4, mod4);
        FM9_MOD_NEXT(5, mod5);
        FM9_MOD_NEXT(6, mod6);
        FM9_MOD_NEXT(7, mod7);
        FM9_MOD_NEXT(8, mod8);

        FM9_OP_NEXT(0, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(1, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(2, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(3, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(4, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(5, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(6, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(7, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(8, table0, table1, lomask, radtoinc, cpstoinc);

        for (int i=0; i < FM9::kNumOps2; i++) mod0[i] += modinc[i];
    }

    FM9_EXPORT(ops, 0);
    FM9_EXPORT(ops, 1);
    FM9_EXPORT(ops, 2);
    FM9_EXPORT(ops, 3);
    FM9_EXPORT(ops, 4);
    FM9_EXPORT(ops, 5);
    FM9_EXPORT(ops, 6);
    FM9_EXPORT(ops, 7);
    FM9_EXPORT(ops, 8);
}

void FM9_next_ki(FM9 *unit, int inNumSamples)
{
    const int32 lomask = unit->m_lomask;
    const float cpstoinc = unit->m_cpstoinc;
    const float radtoinc = unit->m_radtoinc;
	
    FM9Op *ops = unit->m_ops;

    float *table0 = ft->mSineWavetable;
    float *table1 = table0 + 1;

    float *mod0 = unit->m_mod;
    float* mod1 = mod0 + FM9::kNumOps;
    float* mod2 = mod1 + FM9::kNumOps;
    float* mod3 = mod2 + FM9::kNumOps;
    float* mod4 = mod3 + FM9::kNumOps;
    float* mod5 = mod4 + FM9::kNumOps;
    float* mod6 = mod5 + FM9::kNumOps;
    float* mod7 = mod6 + FM9::kNumOps;
    float* mod8 = mod7 + FM9::kNumOps;

    
    FM9_DECLARE(0);
    FM9_IMPORT(ops, 0);

    FM9_DECLARE(1);
    FM9_IMPORT(ops, 1);

    FM9_DECLARE(2);
    FM9_IMPORT(ops, 2);

    FM9_DECLARE(3);
    FM9_IMPORT(ops, 3);

    FM9_DECLARE(4);
    FM9_IMPORT(ops, 4);

    FM9_DECLARE(5);
    FM9_IMPORT(ops, 5);

    FM9_DECLARE(6);
    FM9_IMPORT(ops, 6);

    FM9_DECLARE(7);
    FM9_IMPORT(ops, 7);

    FM9_DECLARE(8);
    FM9_IMPORT(ops, 8);

    LooP(inNumSamples) {
        FM9_MOD_NEXT(0, mod0);
        FM9_MOD_NEXT(1, mod1);
        FM9_MOD_NEXT(2, mod2);
        FM9_MOD_NEXT(3, mod3);
        FM9_MOD_NEXT(4, mod4);
        FM9_MOD_NEXT(5, mod5);
        FM9_MOD_NEXT(6, mod6);
        FM9_MOD_NEXT(7, mod7);
        FM9_MOD_NEXT(8, mod8);

        FM9_OP_NEXT(0, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(1, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(2, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(3, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(4, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(5, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(6, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(7, table0, table1, lomask, radtoinc, cpstoinc);
        FM9_OP_NEXT(8, table0, table1, lomask, radtoinc, cpstoinc);
    }

    FM9_EXPORT(ops, 0);
    FM9_EXPORT(ops, 1);
    FM9_EXPORT(ops, 2);
    FM9_EXPORT(ops, 3);
    FM9_EXPORT(ops, 4);
    FM9_EXPORT(ops, 5);
    FM9_EXPORT(ops, 6);
    FM9_EXPORT(ops, 7);
    FM9_EXPORT(ops, 8);
}

void FM9_Ctor(FM9 *unit)
{
    int modRate = calc_ScalarRate;

    for (int i=0, mi=FM9_MOD_BASE; i < FM9::kNumOps2; i++, mi++) {
        if (INRATE(mi) != calc_ScalarRate) {
            modRate = calc_BufRate;
            break;
        }
    }

    if (modRate == calc_ScalarRate) {
        SETCALC(FM9_next_ki);
    } else {
        SETCALC(FM9_next_kk);
    }

    // Initialize fixed point table lookup arithmetic
    int tableSize = ft->mSineSize;
    unit->m_cpstoinc = tableSize * SAMPLEDUR * 65536.;
    unit->m_radtoinc = tableSize * rtwopi    * 65536.;
    unit->m_lomask   = (tableSize - 1) << 3;

    FM9Op *ops = unit->m_ops;
    float *mod = unit->m_mod;

    for (int i=0, mi=FM9_MOD_BASE; i < FM9::kNumOps; i++) {
        FM9Op *op = ops + i;

        op->m_phase     = 0;
        // op->m_phase     = (int32)(ZIN0(FM9_PHASE_INDEX(i)) * unit->m_radtoinc);
        op->m_freq      = ZIN0(FM9_FREQ_INDEX(i));
        op->m_phasemod  = ZIN0(FM9_PHASE_INDEX(i));
        op->m_amp       = ZIN0(FM9_AMP_INDEX(i));
        op->m_mem       = 0.f;

        for (int j=0; j < FM9::kNumOps; j++, mi++) {
            *mod++ = ZIN0(mi);
        }
    }

    FM9_next_ki(unit, 1);
}

PluginLoad(FM9)
{
    ft = inTable;
    DefineSimpleUnit(FM9);
}

// EOF