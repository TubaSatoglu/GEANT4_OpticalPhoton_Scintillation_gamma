#ifndef PMSENSITIVEDETECTOR_HH
#define PMSENSITIVEDETECTOR_HH 1

#include "G4VSensitiveDetector.hh"
#include "G4ThreeVector.hh"
#include "G4EventManager.hh"

class G4Step;
class G4HCofThisEvent;

class PMSensitiveDetector : public G4VSensitiveDetector {
public:
    PMSensitiveDetector(const G4String& name);
    virtual ~PMSensitiveDetector();

    virtual void Initialize(G4HCofThisEvent*);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory*);
    void EndOfEvent(G4HCofThisEvent*) override;

private:
    G4int fTotalOpticalPhotons;     // ✅ Total optical photons generated
    G4int fPhotonsAtScintillator;   // ✅ Optical photons created inside scintillator
    G4int fPhotonsAtAluminum;       // ✅ Optical photons detected at Aluminum barrier
    G4int fPrimaryOpticalPhotons; 
    G4double fTotalEnergyDeposited;
};

#endif