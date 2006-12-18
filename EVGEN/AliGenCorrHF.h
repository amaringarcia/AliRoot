#ifndef ALIGENCORRHF_H
#define ALIGENCORRHF_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

//
// Class to generate correlated Heavy Flavor hadron pairs using paramtrized
// kinematics of quark pairs from some generator and quark fragmentation functions.
// Is a generalisation of AliGenParam class for correlated pairs of hadrons.
// Author: S. Grigoryan, LPC Clermont-Fd & YerPhI, Smbat.Grigoryan@cern.ch
//

#include "AliGenMC.h"

class AliPythia;
class TParticle;
class TRandom;
class TFile;
class TString;

//-------------------------------------------------------------
class AliGenCorrHF : public AliGenMC
{
 public:
    AliGenCorrHF();
    AliGenCorrHF(Int_t npart, Int_t param);
    AliGenCorrHF(char* tname, Int_t npart, Int_t param);
    AliGenCorrHF(const AliGenCorrHF &CorrHF);
     
    virtual ~AliGenCorrHF();
    virtual void Generate();
    virtual void Init();
    // force decay type
    virtual void SetDecayer(AliDecayer* decayer) {fDecayer = decayer;}

    AliGenCorrHF & operator=(const AliGenCorrHF & rhs);

    // Particle type parametrisation functions, needed by GetHadronPair
    static Int_t IpCharm(TRandom* ran);
    static Int_t IpBeauty(TRandom* ran);

    // Computation of cumulative sums of cell wght-s, needed by GetQuarkPair
    static Double_t ComputeIntegral(TFile* fG);

    // fG - input file w QQbar kinematical grid (TTree) and fragm. functions (24 TH2-s)
    static void GetQuarkPair(TFile* fG, Double_t* fInt, Double_t &y1, Double_t &y2, Double_t &pt1, Double_t &pt2, Double_t &dphi);              
    static void GetHadronPair(TFile* fG, Int_t idq, Double_t y1, Double_t y2, Double_t pt1, Double_t pt2, Int_t &id3, Int_t &id4, Double_t &pz3, Double_t &pz4, Double_t &pt3, Double_t &pt4); 

 protected:
    TString     fFileName;    // Name of the input file
    TFile*      fFile;        //! Pointer to input file
    Int_t       fQuark;       // Quark type 
    Float_t     fBias;        // Biasing factor
    Int_t       fTrials;      // Number of trials
    AliDecayer* fDecayer;     //! Pointer to pythia object for decays

 private:
    static Double_t* fgIntegral; //! Pointer to array of cumulative sums of wght-s
    static Int_t  fgnptbins;             // =12 Number of bins for the fragm. 
                                         //   function dependence on quark pt
    // Number of the grid bins in deltaphi, y and pt:  18, 30 and 32
    static Double_t fgdph[19];           // deltaphi bin coordinates  
    static Double_t fgy[31];             // y bin coordinates
    static Double_t fgpt[33];            // pt bin coordinates
    static Double_t fgptbmin[12];        // min & max coordinates of pt bins for
    static Double_t fgptbmax[12];        // the fragm. function
    
    ClassDef(AliGenCorrHF,1)  // Generator using parameterized QQbar & fragm. functions
};
#endif










