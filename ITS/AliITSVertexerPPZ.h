#ifndef ALIITSVERTEXERPPZ_H
#define ALIITSVERTEXERPPZ_H

#include <AliITSVertexer.h>

/////////////////////////////////////////////////////////////////////
//                                                                 //
// Class for primary vertex Z coordinate reconstruction            //
// Optimized for p-p events (in general: low multiplicity events)  //
//                                                                 //
/////////////////////////////////////////////////////////////////////

class AliITS;
class TH1F; 
class TArrayF;

class AliITSVertexerPPZ : public AliITSVertexer {

 public:
  AliITSVertexerPPZ();
  AliITSVertexerPPZ(TFile *infile, TFile *outfile, Float_t x0=0., Float_t y0=0.);  // standard constructor 
  virtual ~AliITSVertexerPPZ(); // destructor
  virtual AliITSVertex* FindVertexForCurrentEvent(Int_t event);
  virtual void FindVertices();
  virtual Float_t GetZFound() const {return fZFound;}
  virtual Float_t GetZsig() const {return fZsig;}
  virtual void PrintStatus() const;
  virtual void SetDiffPhiMax(Float_t pm = 0.01){fDiffPhiMax = pm;}
  virtual void SetFirstLayerModules(Int_t m1 = 0, Int_t m2 = 79){fFirstL1 = m1; fLastL1 = m2;}
  virtual void SetSecondLayerModules(Int_t m1 = 80, Int_t m2 = 239){fFirstL2 = m1; fLastL2 = m2;}

 private:
  void EvalZ(TH1F *hist,Int_t sepa, Int_t ncoinc, TArrayF *zval);

 protected:
  Int_t fFirstL1;          // first module of the first pixel layer
  Int_t fLastL1;           // last module of the first pixel layer
  Int_t fFirstL2;          // first module of the second pixel layer
  Int_t fLastL2;           // last module of the second pixel layer
  Float_t fDiffPhiMax;     // Maximum delta phi allowed among corr. pixels
  Float_t fX0;             // Nominal x coordinate of the vertex
  Float_t fY0;             // Nominal y coordinate of the vertex
  AliITS *fITS;            //! pointer to the AliITS object
  Float_t fZFound;         //! found value for the current event
  Float_t fZsig;           //! RMS of Z


  ClassDef(AliITSVertexerPPZ,1);
};

#endif
