#ifndef PMPHYSICSLIST_HH
#define PMPHYSICSLIST_HH

#include "G4VModularPhysicsList.hh"

class PMPhysicsList : public G4VModularPhysicsList {
public:
    PMPhysicsList();
    virtual ~PMPhysicsList();

    void ConstructParticle() override;
    void ConstructProcess() override;
    void SetCuts() override;

    void DefineParticles();
    void DefineCuts();
};

#endif
