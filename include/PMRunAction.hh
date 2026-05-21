#ifndef PMRUNACTION_HH
#define PMRUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

class PMRunAction : public G4UserRunAction {
public:
    explicit PMRunAction(G4double energy);
    ~PMRunAction() override;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

private:
    G4double fEnergy;  
};
#endif 
