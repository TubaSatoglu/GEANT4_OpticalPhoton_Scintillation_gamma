#ifndef PMActionInitialization_h
#define PMActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "globals.hh"

class PMPrimaryGenerator;

class PMActionInitialization : public G4VUserActionInitialization {
public:
    PMActionInitialization(G4double energy);
    virtual ~PMActionInitialization();
    
    virtual void BuildForMaster() const;
    virtual void Build() const;
    
    // Getter metodları
    PMPrimaryGenerator* GetPrimaryGenerator() const { return fPrimaryGen; }
    void SetEnergy(G4double energy) { fEnergy = energy; }

private:
    G4double fEnergy;
    mutable PMPrimaryGenerator* fPrimaryGen = nullptr;  };

#endif